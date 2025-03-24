module;

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <cassert>

export module mo_yanxi.graphic.renderer.ui;

export import mo_yanxi.graphic.batch_proxy;
export import mo_yanxi.vk.vertex_info;
export import mo_yanxi.assets.graphic;
export import mo_yanxi.graphic.renderer;
export import mo_yanxi.vk.util.cmd.render;
export import mo_yanxi.vk.ext;
export import mo_yanxi.vk.util.uniform;

export import mo_yanxi.graphic.post_processor.resolve;
export import mo_yanxi.graphic.post_processor.bloom;
export import mo_yanxi.graphic.post_processor.ssao;

import std;

namespace mo_yanxi::graphic{
	struct ui_vertex_uniform{
		vk::padded_mat3 view;

		constexpr friend bool operator==(const ui_vertex_uniform& lhs, const ui_vertex_uniform& rhs) noexcept = default;
	};

	struct scissor{
		math::vec2 src{};
		math::vec2 dst{};
		float distance{};
		constexpr friend bool operator==(const scissor& lhs, const scissor& rhs) noexcept = default;

	};

	 struct scissor_raw{
		math::frect rect{};
		float distance{};

	 	void uniform(const math::mat3& mat) noexcept{
	 		auto src = mat * rect.get_src();
	 		auto dst = mat * rect.get_end();
	 		rect = {src, dst};
	 	}

		constexpr friend bool operator==(const scissor_raw& lhs, const scissor_raw& rhs) noexcept = default;

		constexpr explicit(false) operator scissor() const noexcept{
	 		return scissor{rect.get_src(), rect.get_end(), distance};
	 	}
	};


	struct ui_fragment_uniform{
		// std::uint32_t enable_depth{};
		// float camera_scale{};
		// std::uint32_t cap1{};
		// std::uint32_t cap2{};

		scissor scissor{};

		float global_time{};

		math::vec2 viewport_extent{};

		constexpr friend bool operator==(const ui_fragment_uniform& lhs, const ui_fragment_uniform& rhs) noexcept = default;
	};

	
	struct ui_layer_default : batch_layer{

	public:
		[[nodiscard]] ui_layer_default() = default;

		[[nodiscard]] ui_layer_default(
			vk::context& context,
			const std::size_t chunk_count,
			const std::initializer_list<VkDescriptorSetLayout> public_sets)
			: batch_layer(context, chunk_count, public_sets){

			vk::graphic_pipeline_template pipeline_template{};
			pipeline_template.set_shaders({
				assets::graphic::shaders::vert::ui,
				assets::graphic::shaders::frag::ui});
			pipeline_template.set_vertex_info(
				vk::vertices::vertex_ui_info.get_default_bind_desc(),
				vk::vertices::vertex_ui_info.get_default_attr_desc()
			);
			pipeline_template.push_color_attachment_format(VK_FORMAT_R8G8B8A8_UNORM);
			pipeline_template.push_color_attachment_format(VK_FORMAT_R8G8B8A8_UNORM);
			pipeline_template.push_color_attachment_format(VK_FORMAT_R8G8B8A8_UNORM);

			create_pipeline(context, pipeline_template, public_sets);
		}

		command_buffer_modifier create_command(
			const vk::dynamic_rendering& rendering,
			vk::batch& batch
		) override{
			auto region = batch.get_context()->get_screen_area();
			for (auto && [idx, vk_command_buffer] : command_buffers | std::views::enumerate){
				vk::scoped_recorder scoped_recorder{vk_command_buffer, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT};

				rendering.begin_rendering(scoped_recorder, region);

				pipeline.bind(scoped_recorder, VK_PIPELINE_BIND_POINT_GRAPHICS);

				co_yield scoped_recorder.get();

				vk::cmd::set_viewport(scoped_recorder, region);
				vk::cmd::set_scissor(scoped_recorder, region);

				batch.bind_resources(scoped_recorder, pipeline_layout, idx);

				vkCmdEndRendering(scoped_recorder);
			}
		}
	};

	
	export struct renderer_ui;

	export
	struct ui_batch_proxy : batch_proxy{
	private:
		friend renderer_ui;

		vk::color_attachment color_base{};
		vk::color_attachment color_light{};
		vk::color_attachment color_background{};
		
		vk::texel_image blit_base{};
		vk::texel_image blit_light{};
		
	private:
		vk::descriptor_layout descriptor_layout_proj{};
		vk::descriptor_buffer descriptor_buffer_proj{};
	public:
		batch_layer_data<ui_vertex_uniform> vert_data{};
		batch_layer_data<ui_fragment_uniform> frag_data{};

	private:
		ui_layer_default main_layer{};

		vk::descriptor_layout blit_descriptor_layout{};
		vk::descriptor_buffer blit_descriptor_buffer{};
		vk::pipeline_layout blit_pipeline_layout{};
		vk::pipeline blit_pipeline{};
		vk::command_buffer blit_command{};

		std::vector<math::mat3> transforms{};
		std::vector<scissor_raw> scissors{};


	public:
		[[nodiscard]] ui_batch_proxy() = default;

		[[nodiscard]] explicit ui_batch_proxy(vk::context& context) :
			batch_proxy(vk::batch{
					context, assets::graphic::buffers::indices_buffer, assets::graphic::samplers::ui_sampler, 4,
					sizeof(vk::vertices::vertex_ui), 512
				}),
			color_base{context.get_allocator(), context.get_extent(), VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT},
			color_light{context.get_allocator(), context.get_extent(), VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT},
			color_background{context.get_allocator(), context.get_extent(), VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT},
			blit_base{context.get_allocator(), context.get_extent(), 1},
			blit_light{context.get_allocator(), context.get_extent(), 1},

			descriptor_layout_proj{
				context.get_device(), [](vk::descriptor_layout_builder& builder){
					builder.push_seq(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
					builder.push_seq(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
				}
			},
			descriptor_buffer_proj(
				context.get_allocator(),
				descriptor_layout_proj,
				descriptor_layout_proj.binding_count(), batch.get_chunk_count()),

			vert_data{context, batch.get_chunk_count()},
			frag_data{context, batch.get_chunk_count()},
			main_layer(context, batch.get_chunk_count(), {descriptor_layout_proj, batch.descriptor_layout()}),

			blit_descriptor_layout{
				context.get_device(), [](vk::descriptor_layout_builder& builder){
					builder.push_seq(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);
					builder.push_seq(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);
					builder.push_seq(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);
					builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
					builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
				}
			},
			blit_descriptor_buffer(
				context.get_allocator(),
				blit_descriptor_layout,
				blit_descriptor_layout.binding_count()),
			blit_pipeline_layout{context.get_device(), 0, {blit_descriptor_layout}},
			blit_pipeline{
				context.get_device(), blit_pipeline_layout,
				VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
				assets::graphic::shaders::comp::ui_blit.get_create_info()},
			blit_command(context.get_compute_command_pool().obtain())
		{

			batch.set_consumer([this](std::size_t indices){
				return submit(indices);
			});

			const vk::descriptor_mapper mapper{descriptor_buffer_proj};
			vert_data.bind_to(mapper, 0);
			frag_data.bind_to(mapper, 1);

			on_resize(context.get_extent());
		}

		void resize(const VkExtent2D extent){
			color_base.resize(extent);
			color_light.resize(extent);
			color_background.resize(extent);
			blit_base.resize(extent);
			blit_light.resize(extent);

			on_resize(extent);
		}

		void blit() const{
			vk::cmd::submit_command(context().compute_queue(), blit_command);
		}

		void push_scissor(scissor_raw scissor){
			const auto back = scissors.back();
			if(back != scissor){
				batch.consume_all();
			}

			scissor.rect = scissor.rect.intersection_with(back.rect);

			scissors.push_back(scissor);
		}

		void pop_scissor(){
			if(scissors.size() >= 2 && scissors.back() == scissors.crbegin()[1]){

			}else{
				batch.consume_all();
			}

			scissors.pop_back();
			assert(scissors.size() > 0);
		}

	private:
		void on_resize(const VkExtent2D extent){
			init_image_layout(context().get_transient_graphic_command_buffer(), context().get_transient_compute_command_buffer());
			update_layer_commands(main_layer);
			create_blit_command();

			vert_data.current = {{math::mat3{}.set_orthogonal({}, math::vector2{extent.width, extent.height}.as<float>())}};
			scissors.assign(1, {
				.rect = math::frect{
					math::vector2{extent.width, extent.height}.as<float>()
				},
				.distance = 0.f
			});
		}

		void update_layer_commands(batch_layer& batch_layer){
			vk::dynamic_rendering dynamic_rendering{
					{color_base.get_image_view(), color_light.get_image_view(), color_background.get_image_view()}
				};

			const auto h = batch_layer.create_command(dynamic_rendering, batch);
			for(std::size_t i = 0; i < batch.get_chunk_count(); ++i){

				vk::cmd::bind_descriptors(
					h.current_command(),
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					batch_layer.get_pipeline_layout(), 0,
					{
						descriptor_buffer_proj, batch.descriptor_buffer()},
					{
						descriptor_buffer_proj.get_chunk_offset(i),
						batch.descriptor_buffer().get_chunk_offset(i)});

				h.submit();
			}
		}

		void init_image_layout(VkCommandBuffer graphic_command_buffer, VkCommandBuffer compute_command_buffer){
			color_base.init_layout(graphic_command_buffer);
			color_light.init_layout(graphic_command_buffer);
			color_background.init_layout(graphic_command_buffer);
			blit_base.init_layout_general(compute_command_buffer);
			blit_light.init_layout_general(compute_command_buffer);

			VkSampler sampler = assets::graphic::samplers::blit_sampler;
			vk::descriptor_mapper{blit_descriptor_buffer}
				.set_image(0, color_base.get_image_view(), 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				.set_image(1, color_light.get_image_view(), 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				.set_image(2, color_background.get_image_view(), 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				.set_image(3, blit_base.get_image_view(), 0, VK_IMAGE_LAYOUT_GENERAL, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
				.set_image(4, blit_light.get_image_view(), 0, VK_IMAGE_LAYOUT_GENERAL, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)

			;

		}

		VkCommandBuffer submit(const std::size_t index){
			vert_data.update<true>(index);
			auto csr = scissors.back();
			const math::mat3 proj = vert_data.current.view;
			csr.uniform(proj);
			frag_data.current.scissor = csr;
			frag_data.current.viewport_extent = ~proj.get_ortho_scale() * 2;
			frag_data.update<true>(index);

			return main_layer.submit(context(), index);
		}

		void create_blit_command(){
			const vk::scoped_recorder scoped_recorder{blit_command, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT};
			std::array attachments{&color_base, &color_light, &color_background};

			for(const auto attachment : attachments){
				attachment->set_layout_to_read_optimal(
					scoped_recorder,
					context().graphic_family(),
					context().compute_family()
				);
			}


			blit_pipeline.bind(scoped_recorder, VK_PIPELINE_BIND_POINT_COMPUTE);
			blit_descriptor_buffer.bind_to(scoped_recorder, VK_PIPELINE_BIND_POINT_COMPUTE, blit_pipeline_layout, 0);

			auto group = post_processor::get_work_group_size(std::bit_cast<math::u32size2>(blit_base.get_image().get_extent2()));
			vkCmdDispatch(scoped_recorder, group.x, group.y, 1);

			for(const auto attachment : attachments){
				vk::cmd::clear_color(
					scoped_recorder, attachment->get_image(), {},

					vk::image::default_image_subrange,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					context().compute_family(),
					context().compute_family(),
					context().graphic_family()
				);
			}
		}
	};

	export
	struct renderer_ui : renderer{
	public:
		ui_batch_proxy batch{};
		vk::texel_image merge_result{};

		bloom_post_processor bloom{};

		[[nodiscard]] renderer_ui() = default;

		[[nodiscard]] renderer_ui(
			vk::context& context,
			renderer_export& export_target) :
			renderer(context, export_target, "renderer.ui"),
			batch(context),
			merge_result{
			context.get_allocator(), context.get_extent()
			},

			bloom{
				context,
				assets::graphic::shaders::comp::bloom,
				assets::graphic::samplers::blit_sampler,
				6, 0.35f
			},
			merge_descriptor_layout(context.get_device(), [](vk::descriptor_layout_builder& builder){
				builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
				builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
				builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
				builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
			}),

			merge_descriptor_buffer{
				context.get_allocator(), merge_descriptor_layout, merge_descriptor_layout.binding_count()
			},
			merge_pipeline_layout(context.get_device(), 0, {merge_descriptor_layout}),
			merge_pipeline(
				context.get_device(), merge_pipeline_layout,
				VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
				assets::graphic::shaders::comp::ui_merge.get_create_info()),
			merge_command(context.get_compute_command_pool().obtain()){

			bloom.set_strength(.75f, .75f);

			init_layout(context.get_transient_compute_command_buffer());
			create_merge_commands();
			set_merger_state();
			set_bloom_state();

			set_export(merge_result);
		}

		void resize(const VkExtent2D extent){
			batch.resize(extent);

			merge_result.resize(extent);

			{
				const auto cmd = context().get_transient_compute_command_buffer();
				bloom.resize(
					cmd,
					{extent.width, extent.height},
					false);

				init_layout(cmd);
			}

			create_merge_commands();
			set_merger_state();
			set_bloom_state();

			set_export(merge_result);
		}

		void post_process() const{
			vk::cmd::submit_command(
				context().compute_queue(),
				std::array{
					// batch.blit_command.get(),
					bloom.get_main_command_buffer(),
					merge_command.get()
				});
		}

		void set_time(float time_in_sec){
			batch.frag_data.current.global_time = std::fmod(time_in_sec, 1 << 16);
		}

	private:
		vk::descriptor_layout merge_descriptor_layout{};
		vk::descriptor_buffer merge_descriptor_buffer{};
		vk::pipeline_layout merge_pipeline_layout{};
		vk::pipeline merge_pipeline{};
		vk::command_buffer merge_command{};

		void set_bloom_state(){
			bloom.set_input({
				graphic::post_process_socket{
					.image = batch.blit_light.get_image(),
					.view = batch.blit_light.get_image_view(),
					.layout = VK_IMAGE_LAYOUT_GENERAL,
					.queue_family_index = context().compute_family(),
				}
			});
		}

		void set_merger_state(){
			vk::descriptor_mapper{merge_descriptor_buffer}
				.set_image(0, merge_result.get_image_view(), 0, VK_IMAGE_LAYOUT_GENERAL, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
				.set_image(1, batch.blit_base.get_image_view(), 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
				.set_image(2, batch.blit_light.get_image_view(), 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
				.set_image(3, bloom.get_output().view, 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		}

		void create_merge_commands(){
			std::array attachments{&batch.blit_base, &batch.blit_light};

			vk::scoped_recorder scoped_recorder{merge_command, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT};

			vk::cmd::dependency_gen dependency{};

			for (const auto & attachment : attachments){
				dependency.push(
					attachment->get_image(),
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					VK_ACCESS_2_SHADER_READ_BIT,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					VK_ACCESS_2_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_GENERAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					vk::image::default_image_subrange
				);
			}

			dependency.push(
				merge_result.get_image(),
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_READ_BIT,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_LAYOUT_GENERAL,
				vk::image::default_image_subrange
			);

			dependency.apply(scoped_recorder);

			merge_pipeline.bind(scoped_recorder, VK_PIPELINE_BIND_POINT_COMPUTE);
			merge_descriptor_buffer.bind_to(scoped_recorder, VK_PIPELINE_BIND_POINT_COMPUTE, merge_pipeline_layout, 0);

			auto group = post_processor::get_work_group_size(std::bit_cast<math::u32size2>(merge_result.get_image().get_extent2()));
			vkCmdDispatch(scoped_recorder, group.x, group.y, 1);

			dependency.push(
				merge_result.get_image(),
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				vk::image::default_image_subrange
			);

			dependency.apply(scoped_recorder);

			for (const auto & attachment : attachments){
				vk::cmd::clear_color(
					scoped_recorder, attachment->get_image(), {},

					vk::image::default_image_subrange,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
					VK_IMAGE_LAYOUT_GENERAL
				);
			}
		}

		void init_layout(VkCommandBuffer compute_command_buffer) const{
			merge_result.get_image().init_layout(
				compute_command_buffer,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
		}
	};
}