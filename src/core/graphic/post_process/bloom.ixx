module;

#include <vulkan/vulkan.h>

export module mo_yanxi.graphic.post_processor.bloom;

export import mo_yanxi.graphic.post_processor.base;
import mo_yanxi.vk.image_derives;
import mo_yanxi.vk.uniform_buffer;
import mo_yanxi.vk.shader;
import mo_yanxi.vk.ext;
import mo_yanxi.math;
import std;

namespace mo_yanxi::graphic{
	export
	struct bloom_post_processor final : post_processor{
		constexpr std::uint32_t reverse_after(std::uint32_t value, std::uint32_t ceil) noexcept{
			if(value < ceil)return value;
			return (ceil * 2 - value - 1);
		}

		constexpr std::uint32_t reverse_after_bit(std::uint32_t value, std::uint32_t ceil) noexcept{
			if(value < ceil)return value;
			return (ceil * 2 - value - 1) | ~(~0u >> 1);
		}

		// constexpr std::uint32_t b = reverse_after(7, 5);

	protected:
		// static constexpr std::uint32_t mipmap_levels = 6;

		struct bloom_uniform_block{
			std::uint32_t current_layer;
			std::uint32_t up_scaling;
			std::uint32_t total_layer;
			float scale;

			float strength_src;
			float strength_dst;
			float c2;
			float c3;
		};

		VkSampler sampler_{};
		std::uint32_t max_mipmap_levels{};
		float bloom_scale{};
		float strength_src{1.};
		float strength_dst{1.};

		vk::texel_image mipmap_tree{};
		vk::texel_image output_image{};

		std::vector<vk::image_view> down_mipmap_image_views{};
		std::vector<vk::image_view> up_mipmap_image_views{};

		vk::uniform_buffer info_uniform_buffer{};

		void record_commands() override{
			using namespace mo_yanxi::vk;

			scoped_recorder scoped_recorder{main_command_buffer, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT};


			const auto& input = inputs[0];
			const auto extent = std::bit_cast<math::u32size2>(output_image.get_image().get_extent2());
			const bool from_graphic = input.queue_family_index == context_->graphic_family();

			pipeline.bind(scoped_recorder, VK_PIPELINE_BIND_POINT_COMPUTE);

			cmd::memory_barrier(
				scoped_recorder,
				input.image,
			    from_graphic ? VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT : VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			    from_graphic ? VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT : VK_ACCESS_2_SHADER_WRITE_BIT,
			    VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			    VK_ACCESS_2_SHADER_READ_BIT,
			    input.layout,
			    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			    image::default_image_subrange,
			    input.queue_family_index,
			    context().compute_family()
			);



			for(std::uint32_t i = 0; i < get_real_mip_level() * 2; ++i){
				auto current_mipmap_index = reverse_after(i, get_real_mip_level());
				auto div = 1 << (current_mipmap_index + 1 - (i >= get_real_mip_level()));
				math::u32size2 current_ext{extent / div};

				if(i > 0){
					if(i <= get_real_mip_level()){
						//mipmap access barrier
						cmd::memory_barrier(
							scoped_recorder,
							mipmap_tree.get_image(),
							VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
							VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
							VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
							VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
							VK_IMAGE_LAYOUT_GENERAL,
							VK_IMAGE_LAYOUT_GENERAL,

							VkImageSubresourceRange{
								VK_IMAGE_ASPECT_COLOR_BIT, reverse_after(i - 1, get_real_mip_level()), 1, 0, 1
							}
						);
					}else{
						cmd::memory_barrier(
							scoped_recorder,
							output_image.get_image(),
							VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
							VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
							VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
							VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
							VK_IMAGE_LAYOUT_GENERAL,
							VK_IMAGE_LAYOUT_GENERAL,

							VkImageSubresourceRange{
								VK_IMAGE_ASPECT_COLOR_BIT, reverse_after(i, get_real_mip_level()) + 1, 1, 0, 1
							}
						);
					}
				}

				if(current_mipmap_index == 0 && i != 0){
					//Final, set output image layout to general
					cmd::memory_barrier(
						scoped_recorder,
						output_image.get_image(),
						VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
						VK_ACCESS_2_SHADER_READ_BIT,
						VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
						VK_ACCESS_2_SHADER_WRITE_BIT,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						VK_IMAGE_LAYOUT_GENERAL
					);
				}

				cmd::bind_descriptors(scoped_recorder, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, {descriptor_buffer}, {descriptor_buffer.get_chunk_offset(i)});

				// ext::cmdBindThenSetDescriptorBuffers(scoped_recorder, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, arr);

				auto groups = get_work_group_size(current_ext);
				vkCmdDispatch(scoped_recorder, groups.x, groups.y, 1);
			}

			cmd::memory_barrier(
				scoped_recorder,
				output_image.get_image(),
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);

			cmd::memory_barrier(
				scoped_recorder,
				input.image,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_READ_BIT,
				from_graphic ? VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT : VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				from_graphic ? VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT : VK_ACCESS_2_SHADER_WRITE_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				input.layout,
				image::default_image_subrange,
				context().compute_family(),
				input.queue_family_index
			);
		}

		[[nodiscard]] std::uint32_t get_real_mip_level() const noexcept{
			return std::min(max_mipmap_levels, vk::get_recommended_mip_level(context().get_extent()));
		}

	public:
		[[nodiscard]] const auto& get_output() const noexcept{
			return outputs.front();
		}

		[[nodiscard]] bloom_post_processor() = default;

		void set_input(std::initializer_list<post_process_socket> sockets) override{
			if(sockets.size() != 1){
				throw post_processor_socket_unmatch_error{1};
			}
			inputs = sockets;

			vk::descriptor_mapper mapper{descriptor_buffer};

			for(std::uint32_t i = 0; i < get_real_mip_level() * 2; ++i){
				mapper.set_image(0, inputs[0].view, i, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler_);
			}

			record_commands();
		}

		[[nodiscard]] explicit bloom_post_processor(
			vk::context& context,
			const vk::shader_module& bloom_shader,
			VkSampler sampler,
			const std::uint32_t max_mip_levels = 5,
			float scale = 1.5f
		) :
			post_processor(context),
			sampler_(sampler),
			max_mipmap_levels(max_mip_levels),
			bloom_scale{scale},
			mipmap_tree{
				context.get_allocator(),
				{context.get_extent().width / 2, context.get_extent().height / 2},
				get_real_mip_level()
			},
			output_image(
				context.get_allocator(),
				context.get_extent(),
				get_real_mip_level()
			){
			descriptor_layout = vk::descriptor_layout(context.get_device(), [](vk::descriptor_layout_builder& builder){
				builder.push_seq(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);
				builder.push_seq(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);
				builder.push_seq(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);

				builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
				builder.push_seq(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
			});

			default_init_pipeline(bloom_shader.get_create_info());

			auto cmd = context.get_transient_compute_command_buffer();
			init_image_state(cmd);
		}

		void resize(VkCommandBuffer command_buffer, const math::usize2 size, bool record_command) override{
			mipmap_tree.resize({size.x / 2, size.y / 2});
			output_image.resize({size.x, size.y});

			init_image_state(command_buffer);
			if(record_command)record_commands();
		}

		void set_scale(const float scale) const{
			vk::buffer_mapper ubo_mapper{info_uniform_buffer};
			for(std::uint32_t i = 0; i < get_real_mip_level() * 2; ++i){
				(void)ubo_mapper.load(scale, sizeof(bloom_uniform_block) * i + std::bit_cast<std::uint32_t>(&bloom_uniform_block::scale));
			}
		}

		void set_strength(const float strengthSrc = 1.f, const float strengthDst = 1.f){
			vk::buffer_mapper ubo_mapper{info_uniform_buffer};
			for(std::uint32_t i = 0; i < get_real_mip_level() * 2; ++i){
				(void)ubo_mapper.load(strengthSrc, &bloom_uniform_block::strength_src, i);
				(void)ubo_mapper.load(strengthDst, &bloom_uniform_block::strength_dst, i);
			}
			this->strength_src = strengthSrc;
			this->strength_dst = strengthDst;
		}

	private:

		void init_image_state(VkCommandBuffer command_buffer){
			mipmap_tree.init_layout_general(command_buffer);
			output_image.get_image().init_layout(command_buffer,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);

			outputs.resize(1);
			outputs[0] = {
				.image = output_image.get_image(),
				.view = output_image.get_image_view(),
				.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				.queue_family_index = context_->compute_family()
			};

			update_descriptors();
		}

		void update_descriptors(){
			default_init_descriptor_buffer(get_real_mip_level() * 2);
			info_uniform_buffer = {context().get_allocator(), sizeof(bloom_uniform_block) * get_real_mip_level() * 2};

			{
				vk::descriptor_mapper info{descriptor_buffer};
				vk::buffer_mapper ubo_mapper{info_uniform_buffer};
				for(std::uint32_t i = 0; i < get_real_mip_level() * 2; ++i){
					(void)info.set_uniform_buffer(
						4,
						info_uniform_buffer.get_address() + i * sizeof(bloom_uniform_block),
						sizeof(bloom_uniform_block), i);

					(void)ubo_mapper.load(bloom_uniform_block{
						.current_layer = reverse_after_bit(i, get_real_mip_level()),
						.up_scaling = i >= get_real_mip_level(),
						.total_layer = get_real_mip_level(),
						.scale = bloom_scale,
						.strength_src = strength_src,
						.strength_dst = strength_dst,
					}, sizeof(bloom_uniform_block) * i);
				}
			}

			down_mipmap_image_views.resize(get_real_mip_level());
			up_mipmap_image_views.resize(get_real_mip_level());
			for (auto&& [idx, view] : down_mipmap_image_views | std::views::enumerate){
				view = vk::image_view(
					context().get_device(), {
						.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
						.pNext = nullptr,
						.flags = 0,
						.image = mipmap_tree.get_image(),
						.viewType = VK_IMAGE_VIEW_TYPE_2D,
						.format = VK_FORMAT_R8G8B8A8_UNORM,
						.components = {},
						.subresourceRange = VkImageSubresourceRange{
							VK_IMAGE_ASPECT_COLOR_BIT, static_cast<std::uint32_t>(idx), 1, 0, 1
						}
					}
				);
			}

			for (auto&& [idx, view] : up_mipmap_image_views | std::views::enumerate){
				view = vk::image_view(
					context().get_device(), {
						.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
						.pNext = nullptr,
						.flags = 0,
						.image = output_image.get_image(),
						.viewType = VK_IMAGE_VIEW_TYPE_2D,
						.format = VK_FORMAT_R8G8B8A8_UNORM,
						.components = {},
						.subresourceRange = VkImageSubresourceRange{
							VK_IMAGE_ASPECT_COLOR_BIT, static_cast<std::uint32_t>(idx), 1, 0, 1
						}
					}
				);
			}


			for(std::uint32_t i = 0; i < get_real_mip_level() * 2; ++i){
				vk::descriptor_mapper mapper{descriptor_buffer};
				mapper.set_image(1, mipmap_tree.get_image_view(), i, VK_IMAGE_LAYOUT_GENERAL, sampler_);
				mapper.set_image(2, output_image.get_image_view(), i, VK_IMAGE_LAYOUT_GENERAL, sampler_);

				if(i < get_real_mip_level()){
					mapper.set_storage_image(3, down_mipmap_image_views[i], VK_IMAGE_LAYOUT_GENERAL, i);
				}else{
					mapper.set_storage_image(3, up_mipmap_image_views[reverse_after(i, get_real_mip_level())], VK_IMAGE_LAYOUT_GENERAL, i);
				}
			}
		}

	};
}

