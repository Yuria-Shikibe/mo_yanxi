module;

#include <vulkan/vulkan.h>

export module mo_yanxi.graphic.render_graph.bloom;

import std;
export import mo_yanxi.graphic.render_graph.post_process_pass;
import mo_yanxi.vk.image_derives;
import mo_yanxi.vk.ext;
import mo_yanxi.math.vector2;

namespace mo_yanxi::graphic::render_graph{
	[[nodiscard]] constexpr std::uint32_t reverse_after(std::uint32_t value, std::uint32_t ceil) noexcept{
		if(value < ceil)return value;
		return (ceil * 2 - value - 1);
	}

	[[nodiscard]] constexpr std::uint32_t reverse_after_bit(std::uint32_t value, std::uint32_t ceil) noexcept{
		if(value < ceil)return value;
		return (ceil * 2 - value - 1) | ~(~0u >> 1);
	}

	[[nodiscard]] constexpr std::uint32_t get_real_mip_level(std::uint32_t expected, math::u32size2 extent) noexcept{
		return std::min(expected, vk::get_recommended_mip_level(extent.x, extent.y));
	}

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

	export
	struct bloom_pass : post_process_stage{
		using post_process_stage::post_process_stage;

	private:

		std::vector<vk::image_view> down_mipmap_image_views{};
		std::vector<vk::image_view> up_mipmap_image_views{};

		[[nodiscard]] std::uint32_t get_required_mipmap_level() const noexcept{
			return sockets().at_out<resource_desc::image_requirement>(0).mip_levels;
		}

		[[nodiscard]] std::uint32_t get_real_mipmap_level(const math::u32size2 extent) const noexcept{
			return get_real_mip_level(get_required_mipmap_level(), extent);
		}

	public:
		void post_init(vk::context& ctx, const math::u32size2 extent) override{
			init_pipeline(ctx);
		}

		void reset_resources(vk::context& context, const pass_resource_reference& resources, const math::u32size2 extent) override{
			uniform_buffer_ = {context.get_allocator(), sizeof(bloom_uniform_block) * get_real_mipmap_level(extent) * 2};
			reset_descriptor_buffer(context, get_real_mipmap_level(extent) * 2);

			const auto mipLevel = get_real_mipmap_level(extent);
			{
				vk::descriptor_mapper info{descriptor_buffer_};
				vk::buffer_mapper ubo_mapper{uniform_buffer_};
				for(std::uint32_t i = 0; i < mipLevel * 2; ++i){
					(void)info.set_uniform_buffer(
						4,
						uniform_buffer_.get_address() + i * sizeof(bloom_uniform_block),
						sizeof(bloom_uniform_block), i);

					(void)ubo_mapper.load(bloom_uniform_block{
						.current_layer = reverse_after_bit(i, mipLevel),
						.up_scaling = i >= mipLevel,
						.total_layer = mipLevel,
						.scale = 1,
						.strength_src = 1,
						.strength_dst = 1,
					}, sizeof(bloom_uniform_block) * i);
				}
			}

			auto& down_sample_image = std::get<resource_desc::image_entity>(resources.at_in(1).resource);
			auto& up_sample_image = std::get<resource_desc::image_entity>(resources.at_out(0).resource);
			down_mipmap_image_views.resize(mipLevel);
			up_mipmap_image_views.resize(mipLevel);

			for (auto&& [idx, view] : down_mipmap_image_views | std::views::enumerate){
				view = vk::image_view(
					context.get_device(), {
						.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
						.pNext = nullptr,
						.flags = 0,
						.image = down_sample_image.image.image,
						.viewType = VK_IMAGE_VIEW_TYPE_2D,
						.format = VK_FORMAT_R16G16B16A16_SFLOAT,
						.components = {},
						.subresourceRange = VkImageSubresourceRange{
							VK_IMAGE_ASPECT_COLOR_BIT, static_cast<std::uint32_t>(idx), 1, 0, 1
						}
					}
				);
			}

			for (auto&& [idx, view] : up_mipmap_image_views | std::views::enumerate){
				view = vk::image_view(
					context.get_device(), {
						.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
						.pNext = nullptr,
						.flags = 0,
						.image = up_sample_image.image.image,
						.viewType = VK_IMAGE_VIEW_TYPE_2D,
						.format = VK_FORMAT_R16G16B16A16_SFLOAT,
						.components = {},
						.subresourceRange = VkImageSubresourceRange{
							VK_IMAGE_ASPECT_COLOR_BIT, static_cast<std::uint32_t>(idx), 1, 0, 1
						}
					}
				);
			}

			VkSampler sampler = get_sampler_at({0, 0});

			vk::descriptor_mapper mapper{descriptor_buffer_};

			for(std::uint32_t i = 0; i < mipLevel * 2; ++i){
				mapper.set_image(0, std::get<resource_desc::image_entity>(resources.at_in(0).resource).image.image_view, i, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler);
				mapper.set_image(1, down_sample_image.image.image_view, i, VK_IMAGE_LAYOUT_GENERAL, sampler);
				mapper.set_image(2, up_sample_image.image.image_view, i, VK_IMAGE_LAYOUT_GENERAL, sampler);

				if(i < mipLevel){
					mapper.set_storage_image(3, down_mipmap_image_views[i], VK_IMAGE_LAYOUT_GENERAL, i);
				}else{
					mapper.set_storage_image(3, up_mipmap_image_views[reverse_after(i, mipLevel)], VK_IMAGE_LAYOUT_GENERAL, i);
				}
			}
		}

		void record_command(vk::context& context, const pass_resource_reference& resources, math::u32size2 extent,
			VkCommandBuffer buffer) override{

			using namespace vk;

			const auto mipLevel = get_real_mipmap_level(extent);

			pipeline_.bind(buffer, VK_PIPELINE_BIND_POINT_COMPUTE);
			cmd::bind_descriptors(buffer, {descriptor_buffer_});

			const auto& down_sample_image = std::get<resource_desc::image_entity>(resources.at_in(1).resource);
			const auto& up_sample_image = std::get<resource_desc::image_entity>(resources.at_out(0).resource);

			for(std::uint32_t i = 0; i < mipLevel * 2; ++i){
				const auto current_mipmap_index = reverse_after(i, mipLevel);
				const auto div = 1 << (current_mipmap_index + 1 - (i >= mipLevel));
				math::u32size2 current_ext{extent / div};

				if(i > 0){
					if(i <= mipLevel){
						// mipmap access barrier
						 cmd::memory_barrier(
						 	buffer,
						 	down_sample_image.image.image,
						 	VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
						 	VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
						 	VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
						 	VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
						 	VK_IMAGE_LAYOUT_GENERAL,
						 	VK_IMAGE_LAYOUT_GENERAL,

						 	VkImageSubresourceRange{
						 		VK_IMAGE_ASPECT_COLOR_BIT, reverse_after(i - 1, mipLevel), 1, 0, 1
						 	}
						 );
					}else{
						cmd::memory_barrier(
							buffer,
							down_sample_image.image.image,
							VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
							VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
							VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
							VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
							VK_IMAGE_LAYOUT_GENERAL,
							VK_IMAGE_LAYOUT_GENERAL,

							VkImageSubresourceRange{
								VK_IMAGE_ASPECT_COLOR_BIT, reverse_after(i, mipLevel) + 1, 1, 0, 1
							}
						);
					}
				}

				if(current_mipmap_index == 0 && i != 0){
					//Final, set output image layout to general
					cmd::memory_barrier(
						buffer,
						up_sample_image.image.image,
						VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
						VK_ACCESS_2_NONE,
						VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
						VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
						VK_IMAGE_LAYOUT_GENERAL,
						VK_IMAGE_LAYOUT_GENERAL
					);
				}

				cmd::set_descriptor_offsets(buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout_, 0, {0}, {descriptor_buffer_.get_chunk_offset(i)});
				const auto groups = get_work_group_size(current_ext);
				vkCmdDispatch(buffer, groups.x, groups.y, 1);
			}
		}

		[[nodiscard]] std::span<const inout_index> get_required_internal_buffer_slots() const noexcept override{
			static constexpr inout_index idx = 1;
			return {&idx, 1};
		}
	};
}