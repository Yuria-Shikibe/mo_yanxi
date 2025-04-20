module;

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

export module mo_yanxi.graphic.renderer.world;

export import mo_yanxi.graphic.renderer;
export import mo_yanxi.graphic.batch_proxy;
export import mo_yanxi.vk.vertex_info;
export import mo_yanxi.graphic.camera;

import mo_yanxi.assets.graphic;
import mo_yanxi.vk.util.cmd.render;
import mo_yanxi.vk.ext;
import mo_yanxi.vk.util.uniform;

import mo_yanxi.graphic.post_processor.bloom;
import mo_yanxi.graphic.post_processor.ssao;
import mo_yanxi.graphic.post_processor.oit_blender;

import std;

namespace mo_yanxi::graphic{
	// static constexpr std::uint32_t MultiSampleTimes = 0;
	// static constexpr VkSampleCountFlagBits SampleCount = []{
	// 	switch(MultiSampleTimes){
	// 		case 0: return VK_SAMPLE_COUNT_1_BIT;
	// 		case 1: return VK_SAMPLE_COUNT_2_BIT;
	// 		case 2: return VK_SAMPLE_COUNT_4_BIT;
	// 		case 3: return VK_SAMPLE_COUNT_8_BIT;
	// 		case 4: return VK_SAMPLE_COUNT_16_BIT;
	// 		case 5: return VK_SAMPLE_COUNT_32_BIT;
	// 		case 6: return VK_SAMPLE_COUNT_64_BIT;
	// 		default: return VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;
	// 	}
	// }();


	struct world_vertex_uniform{
		vk::padded_mat3 view{math::mat3_idt};
		vk::padded_mat3 proj{math::mat3_idt};
	};

	struct world_fragment_uniform{
		std::uint32_t cap0{};
		float camera_scale{};
		std::uint32_t cap1{};
		std::uint32_t cap2{};

		constexpr friend bool operator==(const world_fragment_uniform& lhs, const world_fragment_uniform& rhs) noexcept = default;
	};

	struct world_layer_default : batch_layer{

	public:
		[[nodiscard]] world_layer_default() = default;

		[[nodiscard]] world_layer_default(
			vk::context& context,
			const std::size_t chunk_count,
			const std::initializer_list<VkDescriptorSetLayout> public_sets,
			bool enable_depth_write = true
		)
			: batch_layer(context, chunk_count, public_sets){

			vk::graphic_pipeline_template pipeline_template{};
			pipeline_template.set_shaders({
				assets::graphic::shaders::vert::world,
				assets::graphic::shaders::frag::world});
			pipeline_template.set_vertex_info(
				vk::vertices::vertex_world_info.get_default_bind_desc(),
				vk::vertices::vertex_world_info.get_default_attr_desc()
			);
			pipeline_template.push_color_attachment_format(VK_FORMAT_R8G8B8A8_UNORM, vk::blending::overwrite);
			pipeline_template.push_color_attachment_format(VK_FORMAT_R8G8B8A8_UNORM, vk::blending::overwrite);
			// if(MultiSampleTimes > 0)pipeline_template.set_multisample(SampleCount, 1.f, true);
			pipeline_template.set_depth_format(VK_FORMAT_D32_SFLOAT);
			pipeline_template.set_depth_state(true, enable_depth_write);

			create_pipeline(context, pipeline_template, public_sets);
		}

		command_buffer_modifier create_command(
			const vk::dynamic_rendering& rendering,
			vk::batch& batch, VkRect2D region
		) override{
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

	struct world_batch_attachments{
		static constexpr std::size_t size = 3;
	private:
		static constexpr VkImageUsageFlags get_usage(VkSampleCountFlagBits samples){
			auto usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			// if(samples == VK_SAMPLE_COUNT_1_BIT){
				return usage | VK_IMAGE_USAGE_STORAGE_BIT;
			// }else{
			// 	return usage;
			// }
		}

	public:
		vk::color_attachment color_base{};
		vk::color_attachment color_light{};
		vk::color_attachment color_normal{};

		[[nodiscard]] world_batch_attachments() = default;

		[[nodiscard]] world_batch_attachments(vk::context& context, VkSampleCountFlagBits samples) :
				color_base(
				  context.get_allocator(), context.get_extent(),
				  get_usage(samples), samples
			  ), color_light(
				  context.get_allocator(), context.get_extent(),
				  get_usage(samples), samples
			  ), color_normal(
				  context.get_allocator(), context.get_extent(),
				  get_usage(samples), samples
			  ){
		}

		void resize(VkExtent2D extent){
			color_base.resize(extent);
			color_normal.resize(extent);
			color_light.resize(extent);
		}

		void init_layout(VkCommandBuffer command_buffer) const{
			color_base.init_layout(command_buffer);
			color_light.init_layout(command_buffer);
			color_normal.init_layout(command_buffer);
		}

		void init_layout(VkCommandBuffer command_buffer, VkImageLayout layout) const{
			color_base.get_image().init_layout(
				command_buffer,
				VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
				VK_ACCESS_2_NONE,
				layout
			);
			color_light.get_image().init_layout(
				command_buffer,
				VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
				VK_ACCESS_2_NONE,
				layout
			);
			color_normal.get_image().init_layout(
				command_buffer,
				VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
				VK_ACCESS_2_NONE,
				layout
			);
		}

		vk::color_attachment& operator [](const std::size_t i) noexcept{
			switch(i){
			case 0 : return color_base;
			case 1 : return color_light;
			case 2 : return color_normal;
			default : std::unreachable();
			}
		}
	};


	export struct renderer_world;

	export
	struct world_batch_proxy : batch_proxy{
	private:
		friend renderer_world;

		world_batch_attachments draw_attachments{};
		world_batch_attachments mid_attachments{};

		vk::image depth{};
		vk::image_view depth_view_d32f{};

		vk::storage_buffer oit_buffer{};
		vk::storage_image oit_image_head{};

		vk::descriptor_layout descriptor_layout{};
		vk::descriptor_buffer descriptor_buffer{};

		// world_vertex_uniform proj{};
		// vk::uniform_buffer proj_ubo{};

		std::vector<math::mat3> projs{};

	public:
		batch_layer_data<world_fragment_uniform> frag_data{};
		batch_layer_data<world_vertex_uniform> vert_data{};

	private:
		world_layer_default main_layer{};
		oit_blender oit_blender{};

	public:
		[[nodiscard]] world_batch_proxy() = default;

		[[nodiscard]] explicit world_batch_proxy(vk::context& context) :
			batch_proxy(vk::batch{
					context, assets::graphic::buffers::indices_buffer, assets::graphic::samplers::texture_sampler, 4,
					sizeof(vk::vertices::vertex_world), 512
				}),
			draw_attachments{context, VK_SAMPLE_COUNT_1_BIT},
			mid_attachments{context, VK_SAMPLE_COUNT_1_BIT},

			oit_buffer{context.get_allocator(), get_oit_buffer_size(context.get_extent()),
				VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
			},
			oit_image_head{context.get_allocator(), context.get_extent(), 1, VK_FORMAT_R32_UINT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT},
			descriptor_layout{
				context.get_device(), [](vk::descriptor_layout_builder& builder){
					builder.push_seq(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
					builder.push_seq(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
					builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
					builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
					builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT);
				}
			},

			descriptor_buffer(
				context.get_allocator(),
				descriptor_layout,
				descriptor_layout.binding_count(), batch.get_chunk_count()),

			frag_data{context, batch.get_chunk_count()},
			vert_data{context, batch.get_chunk_count()},

			main_layer(context, batch.get_chunk_count(), {descriptor_layout, batch.descriptor_layout()}),
			oit_blender{
				context, std::bit_cast<math::usize2>(context.get_extent()),
				assets::graphic::samplers::blit_sampler, assets::graphic::shaders::comp::oit_blend.get_create_info()}{

			batch.set_consumer([this](std::size_t indices){
				return submit(indices);
			});

			for(std::size_t i = 0; i < batch.get_chunk_count(); ++i){
				(void)vk::descriptor_mapper{descriptor_buffer}
				.set_uniform_buffer(0, vert_data.ubo.get_address() + decltype(vert_data)::chunk_size * i, decltype(vert_data)::chunk_size, i)
				.set_uniform_buffer(1, frag_data.ubo.get_address() + decltype(frag_data)::chunk_size * i, decltype(frag_data)::chunk_size, i);
			}

			on_resize(context.get_extent());
		}

		void resize(const VkExtent2D extent){
			oit_buffer.resize(get_oit_buffer_size(extent));
			oit_image_head.resize(extent);
			mid_attachments.resize(extent);
			draw_attachments.resize(extent);
			on_resize(extent);
		}

		void push_proj(const math::mat3& proj){
			batch.consume_all();

			if(projs.empty()){
				vert_data.current.proj = proj;
				projs.push_back(proj);
			}else{
				vert_data.current.proj = proj * vert_data.current.proj;
				projs.push_back(proj);
			}
		}

		void pop_proj(){
			if(projs.empty()){
				throw std::out_of_range("projs is empty");
			}

			batch.consume_all();

			if(projs.size() == 1){
				vert_data.current.proj = math::mat3_idt;
				projs.pop_back();
			}else{
				vert_data.current.proj = projs.back().inv() * vert_data.current.proj;
				projs.pop_back();
			}
		}

		void blit() {
			oit_blender.run(nullptr, nullptr, nullptr);
		}

	private:
		void on_resize(const VkExtent2D extent){
			region.extent = extent;


			for(std::size_t i = 0; i < batch.get_chunk_count(); ++i){
				(void)vk::descriptor_mapper{descriptor_buffer}
					.set_storage_buffer(2, oit_buffer.get_address(), sizeof(oit_statistic), i)
					.set_storage_buffer(3, oit_buffer.get_address() + sizeof(oit_statistic), oit_buffer.get_size() - sizeof(oit_statistic), i)
					.set_storage_image(4, oit_image_head.get_image_view(), VK_IMAGE_LAYOUT_GENERAL, i);
			}


			create_attachments(extent);

			{
				const auto cmd = context().get_transient_graphic_command_buffer();
				const oit_statistic data{get_oit_buffer_count(extent), std::numeric_limits<uint32_t>::max()};
				init_image_layout(cmd, context().get_transient_compute_command_buffer());
				vk::cmd::update(cmd, oit_buffer, 0, data);
			}

			update_layer_commands(main_layer);

			oit_blender.update(
				std::bit_cast<math::usize2>(context().get_extent()),
				oit_buffer.borrow(), {
					post_process_socket{
						.image = oit_image_head.get_image(),
						.view = oit_image_head.get_image_view(),
						.layout = VK_IMAGE_LAYOUT_GENERAL,
						.queue_family_index = context().graphic_family()
					},
					post_process_socket{
						.image = depth,
						.view = depth_view_d32f,
						.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
						.queue_family_index = context().graphic_family()
					},
					post_process_socket{
						.image = draw_attachments.color_base.get_image(),
						.view = draw_attachments.color_base.get_image_view(),
						.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
						.queue_family_index = context().graphic_family()
					},
					post_process_socket{
						.image = draw_attachments.color_light.get_image(),
						.view = draw_attachments.color_light.get_image_view(),
						.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
						.queue_family_index = context().graphic_family()
					},
				}, {
					post_process_socket{
						.image = mid_attachments.color_base.get_image(),
						.view = mid_attachments.color_base.get_image_view(),
						.layout = VK_IMAGE_LAYOUT_GENERAL,
						.queue_family_index = context().compute_family()
					},
					post_process_socket{
						.image = mid_attachments.color_light.get_image(),
						.view = mid_attachments.color_light.get_image_view(),
						.layout = VK_IMAGE_LAYOUT_GENERAL,
						.queue_family_index = context().compute_family()
					},
				});
		}

		void update_layer_commands(batch_layer& batch_layer){
			vk::dynamic_rendering dynamic_rendering{
					{draw_attachments.color_base.get_image_view(), draw_attachments.color_light.get_image_view()},
				depth_view_d32f
				};

			const auto h = batch_layer.create_command(dynamic_rendering, batch, region);
			for(std::size_t i = 0; i < batch.get_chunk_count(); ++i){

				vk::cmd::bind_descriptors(
					h.current_command(),
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					batch_layer.get_pipeline_layout(), 0,
					{descriptor_buffer, batch.descriptor_buffer()},
					{descriptor_buffer.get_chunk_offset(i),
				batch.descriptor_buffer().get_chunk_offset(i)});

				h.submit();
			}

		}

		void init_image_layout(VkCommandBuffer graphic_command_buffer, VkCommandBuffer compute_command_buffer){
			mid_attachments.init_layout(compute_command_buffer, VK_IMAGE_LAYOUT_GENERAL);
			draw_attachments.init_layout(graphic_command_buffer);

			oit_image_head.init_layout_general(graphic_command_buffer);

			vk::cmd::memory_barrier(
				graphic_command_buffer, depth,
				VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
				VK_ACCESS_2_NONE,
				VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
				VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				vk::image::depth_image_subrange
			);
		}

		void create_attachments(VkExtent2D extent){
			depth = {
					context().get_allocator(), VkImageCreateInfo{
						.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
						.imageType = VK_IMAGE_TYPE_2D,
						.format = VK_FORMAT_D32_SFLOAT,
						.extent = {extent.width, extent.height, 1},
						.mipLevels = 1,
						.arrayLayers = 1,
						.samples = VK_SAMPLE_COUNT_1_BIT,
						.tiling = VK_IMAGE_TILING_OPTIMAL,
						.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
						VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
					},
					VmaAllocationCreateInfo{
						.usage = VMA_MEMORY_USAGE_GPU_ONLY
					}
				};

			depth_view_d32f = {
					context().get_device(), VkImageViewCreateInfo{
						.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
						.image = depth,
						.viewType = VK_IMAGE_VIEW_TYPE_2D,
						.format = VK_FORMAT_D32_SFLOAT,
						.components = {},
						.subresourceRange = vk::image::depth_image_subrange
					}
				};
		}

		VkCommandBuffer submit(std::size_t index){

			frag_data.update<true>(index);
			vert_data.update<true>(index);

			return main_layer.submit(context(), index);
		}

		void append_clean_command(VkCommandBuffer scoped_recorder) const{
			vk::cmd::clear_color(
					scoped_recorder, mid_attachments.color_base.get_image(), {},
					vk::image::default_image_subrange,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
					VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
					VK_IMAGE_LAYOUT_GENERAL
				);

			vk::cmd::clear_color(
				scoped_recorder, mid_attachments.color_light.get_image(), {},
				 vk::image::default_image_subrange,
				 VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				 VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				 VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
				 VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
				 VK_IMAGE_LAYOUT_GENERAL
			);

			vk::cmd::clear_depth_stencil(
				scoped_recorder, depth, {1}, vk::image::depth_image_subrange,
				VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
				VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
				VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			);

		}
	};

	struct renderer_world : renderer{
	public:
		camera2 camera{};
		world_batch_proxy batch{};
		//
		// //Post Process
		vk::storage_image merge_result{};
		bloom_post_processor bloom{};
		ssao_post_processor ssao{};

		[[nodiscard]] renderer_world() = default;

		[[nodiscard]] renderer_world(
			vk::context& context,
			renderer_export& export_target) :
			renderer(context, export_target, "renderer.world"),
			batch(context),
			merge_result{
				context.get_allocator(), context.get_extent(), 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
			},

			bloom{
				context, std::bit_cast<math::usize2>(context.get_extent()),
				assets::graphic::shaders::comp::bloom,
				assets::graphic::samplers::blit_sampler,
				7, 1.6f
			},
			ssao{
				context, std::bit_cast<math::usize2>(context.get_extent()),
				assets::graphic::shaders::comp::ssao,
				assets::graphic::samplers::blit_sampler
			},
			merge_descriptor_layout(context.get_device(), [](vk::descriptor_layout_builder& builder){
				builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
				builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
				builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
				builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
				builder.push_seq(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);
			}),

			merge_descriptor_buffer{
				context.get_allocator(), merge_descriptor_layout, merge_descriptor_layout.binding_count()
			},
			merge_pipeline_layout(context.get_device(), 0, {merge_descriptor_layout}),
			merge_pipeline(
				context.get_device(), merge_pipeline_layout,
				VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
				assets::graphic::shaders::comp::world_merge.get_create_info()),
			merge_command(context.get_compute_command_pool().obtain()){
			bloom.set_strength(1.1f, 0.95f);
			init_layout(context.get_transient_compute_command_buffer());


			camera.resize_screen(context.get_extent().width, context.get_extent().height);
			create_merge_commands();
			set_merger_state();
			set_bloom_state();
			set_ssao_state();

			set_export(merge_result);
		}

		void update(float delta_in_tick){
			camera.update(delta_in_tick, true);
			if(camera.check_changed()){
				batch.vert_data.current.view = {camera.get_world_to_uniformed()};
			}

			ssao.set_scale(camera.map_scale(0.15f, 2.5f) * 1.5f);
			bloom.set_scale(camera.map_scale(0.195f, 1.75f));
			batch.frag_data.current.camera_scale = camera.get_scale();
		}

		void resize(const VkExtent2D extent){
			camera.resize_screen(extent.width, extent.height);
			batch.resize(extent);

			merge_result.resize(extent);

			{
				const auto cmd = context().get_transient_compute_command_buffer();
				bloom.resize(
					cmd,
					{extent.width, extent.height},
					false);

				ssao.resize(
					cmd,
					{extent.width, extent.height},
					false);

				init_layout(cmd);
			}

			create_merge_commands();
			set_merger_state();
			set_bloom_state();
			set_ssao_state();

			set_export(merge_result);
		}

		void post_process() const{
			vk::cmd::submit_command(
				context().compute_queue(),
				std::array{
					batch.oit_blender.get_main_command_buffer(),
					ssao.get_main_command_buffer(),
					bloom.get_main_command_buffer(),
					merge_command.get()
				});
		}

		renderer_world(const renderer_world& other) = delete;
		renderer_world(renderer_world&& other) noexcept = default;
		renderer_world& operator=(const renderer_world& other) = delete;
		renderer_world& operator=(renderer_world&& other) noexcept = default;

	private:
		vk::descriptor_layout merge_descriptor_layout{};
		vk::descriptor_buffer merge_descriptor_buffer{};
		vk::pipeline_layout merge_pipeline_layout{};
		vk::pipeline merge_pipeline{};
		vk::command_buffer merge_command{};

		void set_bloom_state(){
			bloom.set_input({
				graphic::post_process_socket{
					.image = batch.mid_attachments.color_light.get_image(),
					.view = batch.mid_attachments.color_light.get_image_view(),
					.layout = VK_IMAGE_LAYOUT_GENERAL,
					.queue_family_index = context().compute_family(),
				}
			});
		}

		void set_merger_state(){
			vk::descriptor_mapper{merge_descriptor_buffer}
			.set_image(0, merge_result.get_image_view(), 0, VK_IMAGE_LAYOUT_GENERAL, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
			.set_storage_image(1, batch.mid_attachments.color_base.get_image_view())
			.set_storage_image(2, batch.mid_attachments.color_light.get_image_view())
			.set_storage_image(3, bloom.get_output().view)
			.set_image(4, ssao.get_output().view, 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, assets::graphic::samplers::blit_sampler);

		}

		void set_ssao_state(){
			ssao.set_input({
				post_process_socket{
					.image = batch.depth,
					.view = batch.depth_view_d32f,
					.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
					.queue_family_index = context().graphic_family()
				}});
		}

		void create_merge_commands(){
			vk::scoped_recorder scoped_recorder{merge_command, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT};

			vk::cmd::dependency_gen dependency{};

			// for(std::size_t i = 0; i < world_batch_attachments::size - 1; ++i){
			// 	dependency.push(
			// 		batch.mid_attachments[i].get_image(),
			// 		VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			// 		VK_ACCESS_2_SHADER_READ_BIT,
			// 		VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			// 		VK_ACCESS_2_SHADER_READ_BIT,
			// 		VK_IMAGE_LAYOUT_GENERAL,
			// 		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			// 		vk::image::default_image_subrange
			// 	);
			// }

			dependency.apply(scoped_recorder);

			merge_pipeline.bind(scoped_recorder, VK_PIPELINE_BIND_POINT_COMPUTE);
			merge_descriptor_buffer.bind_to(scoped_recorder, VK_PIPELINE_BIND_POINT_COMPUTE, merge_pipeline_layout, 0);

			auto group = graphic::post_processor::get_work_group_size(std::bit_cast<math::u32size2>(context().get_extent()));
			vkCmdDispatch(scoped_recorder, group.x, group.y, 1);

			dependency.apply(scoped_recorder);

			batch.append_clean_command(scoped_recorder);
		}

		void init_layout(VkCommandBuffer compute_command_buffer) const{
			merge_result.init_layout_general(compute_command_buffer);
		}
	};
}
