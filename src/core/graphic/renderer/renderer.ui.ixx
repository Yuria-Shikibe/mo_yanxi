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

import mo_yanxi.vk.vma;

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

	constexpr math::mat3 uniform{{2, 0, 0}, {0, 2, 0}, {-1, -1, 1}};
	constexpr math::mat3 inv_uniform = math::mat3{uniform}.inv();

	 struct scissor_raw{
		math::frect rect{};
		float distance{};
		// math::frect shrunk_rect{};

		void uniform(const math::mat3& mat) noexcept{
	 		if(rect.area() < 0.05f){
	 			rect = {-5, -5, 0, 0};
	 			return;
	 		}

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

		float inv_scale{};

		std::uint32_t cap1;
		std::uint32_t cap2;
		std::uint32_t cap3;

		constexpr friend bool operator==(const ui_fragment_uniform& lhs, const ui_fragment_uniform& rhs) noexcept = default;
	};

	struct ui_blit_info{
		math::usize2 offset{};
		math::usize2 cap{};
	};

	
	struct grid_drawer : batch_layer{

	public:
		[[nodiscard]] grid_drawer() = default;

		[[nodiscard]] grid_drawer(
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

	struct indirect_dispatcher{
		using value_type = VkDispatchIndirectCommand;

	private:
		vk::buffer buffer{};
		value_type current{};

	public:
		[[nodiscard]] indirect_dispatcher() = default;

		[[nodiscard]] indirect_dispatcher(vk::allocator& allocator, const std::size_t chunk_count)
			: buffer(allocator, VkBufferCreateInfo{
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = sizeof(value_type) * chunk_count,
				.usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
			}, {
				.usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
			})
		{}

		void set(math::u32size2 extent, math::u32size2 group_extent = post_processor::compute_group_unit_size2) noexcept{
			extent.add(group_extent.copy().sub(1u, 1u)).div(group_extent);

			current = value_type{
				.x = extent.x,
				.y = extent.y,
				.z = 1
			};
		}

		void update(std::size_t index){
			assert(index < buffer.get_size() / sizeof(value_type));

			(void)vk::buffer_mapper{buffer}.load(
					current, sizeof(value_type) * index
				);
		}

		[[nodiscard]] VkDeviceSize offset_at(std::size_t index) const noexcept{
			assert(index < buffer.get_size() / sizeof(value_type));

			return index * sizeof(value_type);
		}

		explicit(false) operator VkBuffer() const noexcept{
			return buffer;
		}
	};

	
	export struct renderer_ui;

	export
	struct ui_batch_proxy : batch_proxy{
	private:
		friend renderer_ui;
		friend batch_proxy;

		vk::color_attachment color_base{};
		vk::color_attachment color_light{};
		vk::color_attachment color_background{};

		vk::storage_image blit_base{};
		vk::storage_image blit_light{};

	private:
		vk::descriptor_layout descriptor_layout_proj{};
		vk::descriptor_buffer descriptor_buffer_proj{};
	public:
		batch_layer_data<ui_vertex_uniform> vert_data{};
		batch_layer_data<ui_fragment_uniform> frag_data{};

	private:
		grid_drawer main_layer{};

		static constexpr std::size_t blit_chunk_count = 3;
		std::size_t current_blit_chunk_index{};
		vk::command_chunk blit_command_chunk{};

		vk::descriptor_layout blit_descriptor_layout{};
		vk::descriptor_buffer blit_descriptor_buffer{};
		vk::pipeline_layout blit_pipeline_layout{};
		vk::pipeline blit_pipeline{};

		// vk::command_buffer blit_command{};
		indirect_dispatcher blit_dispatcher{};
		batch_layer_data<ui_blit_info> blit_data{};

		// math::mat3 current_proj{math::mat3_idt};
		math::mat3 current_viewport_transform{math::mat3_idt};

		std::vector<math::mat3> projections{};

		std::vector<math::frect> viewports{};
		std::vector<scissor_raw> scissors{};

		[[nodiscard]] auto get_public_sets() const noexcept{
			return std::array<VkDescriptorSetLayout, 2>{descriptor_layout_proj.get(), batch.descriptor_layout()};
		}

	public:
		[[nodiscard]] ui_batch_proxy() = default;

		[[nodiscard]] explicit ui_batch_proxy(vk::context& context) :
			batch_proxy(vk::batch{
					context, assets::graphic::buffers::indices_buffer, assets::graphic::samplers::ui_sampler, 4,
					sizeof(vk::vertices::vertex_ui), 512
				}),
			color_base{context.get_allocator(), context.get_extent(), VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT},
			color_light{context.get_allocator(), context.get_extent(), VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT},
			color_background{context.get_allocator(), context.get_extent(), VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT},
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

			blit_command_chunk{context.get_device(), context.get_compute_command_pool(), blit_chunk_count},

			blit_descriptor_layout{
				context.get_device(), [](vk::descriptor_layout_builder& builder){
					builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
					builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
					builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
					builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
					builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
					builder.push_seq(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
				}
			},
			blit_descriptor_buffer(
				context.get_allocator(),
				blit_descriptor_layout,
				blit_descriptor_layout.binding_count(), blit_command_chunk.size()),
			blit_pipeline_layout{context.get_device(), 0, {blit_descriptor_layout}},

			blit_pipeline{
				context.get_device(), blit_pipeline_layout,
				VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
				assets::graphic::shaders::comp::ui_blit.get_create_info()},
			blit_dispatcher(context.get_allocator(), blit_command_chunk.size()),
			blit_data(context, blit_command_chunk.size())
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

		void blit_viewport(math::frect region){
			const auto v0 = region.get_src();
			const auto v1 = region.get_end();
			math::frect transed_region{current_viewport_transform * v0, current_viewport_transform * v1};

			blit(transed_region.round<int>());
		}

	private:
		void blit(const math::irect region){
			const auto clamped = region.intersection_with({tags::unchecked,
				(this->region.offset.x),
				(this->region.offset.y),
				static_cast<int>(this->region.extent.width),
				static_cast<int>(this->region.extent.height)
			}).as<std::uint32_t>();

			blit_data.current.offset = clamped.src;
			blit_dispatcher.set(clamped.size());


			auto& cur = blit_command_chunk[current_blit_chunk_index];
			cur.wait(context().get_device());

			blit_data.update<true>(current_blit_chunk_index);
			blit_dispatcher.update(current_blit_chunk_index);
			cur.submit(context().compute_queue(), VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

			current_blit_chunk_index = (current_blit_chunk_index + 1) % blit_command_chunk.size();
		}
	public:

		void push_scissor(scissor_raw scissor){
			auto back = scissors.back();
			if(back != scissor){
				batch.consume_all();
			}

			auto proj = get_cur_full_proj();
			scissor.uniform(proj);
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

		void push_projection(const math::frect viewport){
			batch.consume_all();

			projections.push_back(math::mat3{}.set_orthogonal(viewport.src, viewport.size()));
		}

		[[nodiscard]] math::mat3 get_cur_proj() const{
			assert(!projections.empty());
			return current_viewport_transform * projections.back();
		}

		[[nodiscard]] const scissor_raw& get_cur_scissor() const{
			assert(!scissors.empty());
			return scissors.back();
		}

		void push_projection(const math::mat3& proj){
			batch.consume_all();

			projections.push_back(proj);
		}

		void pop_projection() noexcept{
			batch.consume_all();

			assert(!projections.empty());
			projections.pop_back();
		}

		void push_viewport(const math::frect viewport){
			batch.consume_all();
			math::frect last_viewport = get_last_viewport();

			math::mat3 transform;
			auto scale = viewport.size() / last_viewport.size();
			transform.from_scaling(scale);
			transform.set_translation(scale - math::vec2{1, 1} + viewport.src / last_viewport.size() * 2);

			if(viewports.empty()){
				current_viewport_transform = transform;
			}else{
				current_viewport_transform *= transform;
			}

			viewports.push_back(viewport);
		}

		void pop_viewport(){
			batch.consume_all();
			assert(!viewports.empty());

			auto viewport = viewports.back();
			viewports.pop_back();

			math::frect last_viewport = get_last_viewport();

			math::mat3 transform;
			const auto scale = viewport.size() / last_viewport.size();
			transform.from_scaling(scale);
			transform.set_translation(scale - math::vec2{1, 1} + viewport.src / last_viewport.size() * 2);

			if(viewports.empty()){
				current_viewport_transform = transform;
			}else{
				current_viewport_transform *= transform.inv();
			}

		}

		batch_layer& get_current_batch_layer() noexcept{
			if(current_layer_){
				return *current_layer_;
			}

			return main_layer;
		}

	private:
		[[nodiscard]] math::frect get_last_viewport() const noexcept{
			math::frect last_viewport;
			if(!viewports.empty()){
				last_viewport = viewports.back();
			}else{
				last_viewport = math::irect{
					region.offset.x, region.offset.y,
					static_cast<int>(region.extent.width),
					static_cast<int>(region.extent.height)
				}.as<float>();
			}
			return last_viewport;
		}

		void on_resize(const VkExtent2D extent){
			region.extent = extent;

			init_image_layout(context().get_transient_graphic_command_buffer(), context().get_transient_compute_command_buffer());
			update_layer_commands(main_layer);

			for (auto& layer : layers_ | std::views::values){
				update_layer_commands(*layer);
			}

			create_blit_command();

			vert_data.current = {{math::mat3{}.set_orthogonal({}, math::vector2{extent.width, extent.height}.as<float>())}};
			scissors.assign(1, {
				.rect = math::frect{-1, -1, 2, 2},
				.distance = 0.f,
			});

			// blit_dispatcher.set({extent.width / 2, extent.height / 2});
		}

		void update_layer_commands(batch_layer& batch_layer){
			vk::dynamic_rendering dynamic_rendering{
					{color_base.get_image_view(), color_light.get_image_view(), color_background.get_image_view()}
				};

			const auto h = batch_layer.create_command(dynamic_rendering, batch, region);
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
			vk::descriptor_mapper blit_binder{blit_descriptor_buffer};

			for(int i = 0; i < blit_command_chunk.size(); ++i){
				blit_binder
					.set_storage_image(0, color_base.get_image_view(), VK_IMAGE_LAYOUT_GENERAL, i)
					.set_storage_image(1, color_light.get_image_view(), VK_IMAGE_LAYOUT_GENERAL, i)
					.set_storage_image(2, color_background.get_image_view(), VK_IMAGE_LAYOUT_GENERAL, i)
					.set_storage_image(3, blit_base.get_image_view(), VK_IMAGE_LAYOUT_GENERAL, i)
					.set_storage_image(4, blit_light.get_image_view(), VK_IMAGE_LAYOUT_GENERAL, i)
				;
			}

			blit_data.bind_to(blit_binder, 5);
		}

		[[nodiscard]] math::mat3 get_cur_full_proj() const noexcept{
			assert(!projections.empty());

			const auto currentProj = projections.back();
			math::mat3 proj = current_viewport_transform * currentProj;
			return proj;
		}

		VkCommandBuffer submit(const std::size_t index){
			assert(!projections.empty());
			assert(!viewports.empty());

			const auto currentProj = projections.back();
			math::mat3 proj = current_viewport_transform * currentProj;
			vert_data.current.view = proj;
			vert_data.update<true>(index);

			auto csr = scissors.back();

			frag_data.current.scissor = csr;
			frag_data.current.viewport_extent = viewports.back().size();

			math::vec2 def_scale = math::vector2{region.extent.width, region.extent.height}.as<float>();
			math::vec2 cur_scale = ~currentProj.get_scale() * 2;

			auto multis = std::sqrt(cur_scale.area() / def_scale.area());
			frag_data.current.inv_scale = multis;
			frag_data.update<true>(index);

			auto& layer = get_current_batch_layer();
			layer.try_post_data(index);
			return layer.submit(context(), index);
		}

		void create_blit_command(){
			for (auto && [idx, unit] : blit_command_chunk | std::views::enumerate){
				const vk::scoped_recorder scoped_recorder{unit, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT};
				std::array attachments{&color_base, &color_light, &color_background};

				vk::cmd::dependency_gen dependency_gen{};

				for(const auto attachment : attachments){
					dependency_gen.push(
						attachment->get_image(),
						VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
						VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
						VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
						VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
						VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
						VK_IMAGE_LAYOUT_GENERAL,
						vk::image::default_image_subrange
					);
				}

				dependency_gen.apply(scoped_recorder);

				blit_pipeline.bind(scoped_recorder, VK_PIPELINE_BIND_POINT_COMPUTE);
				blit_descriptor_buffer.bind_chunk_to(scoped_recorder, VK_PIPELINE_BIND_POINT_COMPUTE, blit_pipeline_layout, 0, idx);

				vkCmdDispatchIndirect(scoped_recorder, blit_dispatcher, blit_dispatcher.offset_at(idx));

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
		}

	};

	struct renderer_ui : renderer{
	public:
		ui_batch_proxy batch{};
		vk::storage_image merge_result{};

		bloom_post_processor bloom{};

		[[nodiscard]] renderer_ui() = default;

		[[nodiscard]] renderer_ui(
			vk::context& context,
			renderer_export& export_target) :
			renderer(context, export_target, "renderer.ui"),
			batch(context),
			merge_result{
			context.get_allocator(), context.get_extent(), 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT
			},

			bloom{
				context, std::bit_cast<math::usize2>(context.get_extent()),
				assets::graphic::shaders::comp::bloom,
				assets::graphic::samplers::blit_sampler,
				6, 1.f
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
			clean_command(context.get_compute_command_pool().obtain()){

			bloom.set_strength(.8f, .8f);

			init_layout(context.get_transient_compute_command_buffer());
			create_clear_commands();
			set_merger_state();
			set_bloom_state();


			set_export(batch.blit_base);
			set_export(batch.blit_light, "light");
			set_export(bloom.get_output(), "bloom");
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

			create_clear_commands();
			set_merger_state();
			set_bloom_state();

			set_export(batch.blit_base);
			set_export(batch.blit_light, "light");
			set_export(bloom.get_output(), "bloom");
		}

		void post_process() const{
			vk::cmd::submit_command(
				context().compute_queue(),
				std::array{
					bloom.get_main_command_buffer(),
					// clean_command.get()
				});
		}

		void clear(){
			vk::cmd::submit_command(
				context().compute_queue(),
				std::array{
					clean_command.get(),
					// clean_command.get()
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
		vk::command_buffer clean_command{};

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
				.set_storage_image(0, merge_result.get_image_view())
				.set_storage_image(1, batch.blit_base.get_image_view())
				.set_storage_image(2, batch.blit_light.get_image_view())
				.set_storage_image(3, bloom.get_output().view);
		}

		void create_clear_commands(){
			std::array attachments{&batch.blit_base, &batch.blit_light};

			vk::scoped_recorder scoped_recorder{clean_command, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT};

			for (const auto & attachment : attachments){
				vk::cmd::clear_color(
					scoped_recorder, attachment->get_image(), {},

					vk::image::default_image_subrange,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
					VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
					VK_IMAGE_LAYOUT_GENERAL
				);
			}
		}

		void init_layout(VkCommandBuffer compute_command_buffer) const{
			merge_result.init_layout_general(compute_command_buffer);
		}
	};
}