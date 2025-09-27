module;

#include <assert.h>
#include <vulkan/vulkan.h>

export module mo_yanxi.graphic.post_process_graph.fill;

export import mo_yanxi.graphic.post_process_graph;

import mo_yanxi.vk.image_derives;
import mo_yanxi.vk.ext;
import mo_yanxi.math.vector2;
import std;


namespace mo_yanxi::graphic{
	export
	template <typename ClearValue>
	struct image_clear_info{
		ClearValue value{};
		std::uint32_t mip_level_base{0};
		std::uint32_t mip_level_count{1};
	};

	struct buffer_sub_range{
		std::uint32_t value{};
		VkDeviceSize off{};
		VkDeviceSize size{std::numeric_limits<VkDeviceSize>::max()};
	};

	export
	struct image_clear : pass{
		using clear_info = image_clear_info<VkClearColorValue>;


	private:
		inout_data sockets_{};
		std::vector<clear_info> clear_values_{};

	public:
		void add(const clear_info& info){
			resource_desc::image_requirement req;
			req.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			req.override_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			req.desc.decoration = resource_desc::image_decr::write;

			sockets_.add(resource_desc::inout_type::inout, req);
			clear_values_.push_back(info);
		}

		[[nodiscard]] explicit image_clear(vk::context& ctx)
			: pass(ctx){
		}

		[[nodiscard]] image_clear(vk::context& ctx, std::initializer_list<clear_info> clear_infos)
			: pass(ctx){
			for (const auto & clear_info : clear_infos){
				add(clear_info);
			}
		}
		[[nodiscard]] image_clear(vk::context& ctx, std::size_t count)
			: pass(ctx){
			for(std::size_t i = 0; i < count; ++i){
				add({});
			}
		}

		inout_data& sockets() noexcept override{
			return sockets_;
		}

		void record_command(vk::context& context, const pass_resource_reference* resources, math::u32size2 extent,
			VkCommandBuffer buffer) override{

			//TODO merge image subrange with the same image entity
			for (auto && [idx, clear_value] : clear_values_ | std::views::enumerate){
				auto& img = resources->at_in(idx).to_image();

				VkImageSubresourceRange clearRng{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = clear_value.mip_level_base,
					.levelCount = clear_value.mip_level_count,
					.baseArrayLayer = 0,
					.layerCount = 1
				};
				vkCmdClearColorImage(buffer, img.image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_value.value, 1, &clearRng);
			}
		}
	};

	export
	struct depth_stencil_image_clear : pass{
		using clear_info = image_clear_info<VkClearDepthStencilValue>;
	private:
		inout_data sockets_{};
		std::vector<clear_info> clear_values_{};

	public:
		void add(const clear_info& info){
			resource_desc::image_requirement req;
			req.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			req.override_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			req.desc.decoration = resource_desc::image_decr::write;
			req.aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;

			sockets_.add(resource_desc::inout_type::inout, req);
			clear_values_.push_back(info);
		}

		[[nodiscard]] explicit depth_stencil_image_clear(vk::context& ctx)
			: pass(ctx){
		}

		[[nodiscard]] depth_stencil_image_clear(vk::context& ctx, std::initializer_list<clear_info> clear_infos)
			: pass(ctx){
			for (const auto & clear_info : clear_infos){
				add(clear_info);
			}
		}

		inout_data& sockets() noexcept override{
			return sockets_;
		}

		void record_command(vk::context& context, const pass_resource_reference* resources, math::u32size2 extent,
			VkCommandBuffer buffer) override{

			//TODO merge image subrange with the same image entity
			for (auto && [idx, clear_value] : clear_values_ | std::views::enumerate){
				auto& img = resources->at_in(idx).to_image();

				VkImageSubresourceRange clearRng{
					.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
					.baseMipLevel = clear_value.mip_level_base,
					.levelCount = clear_value.mip_level_count,
					.baseArrayLayer = 0,
					.layerCount = 1
				};

				vkCmdClearDepthStencilImage(buffer, img.image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_value.value, 1, &clearRng);
			}
		}
	};


	export
	struct buffer_fill : pass{
		using clear_info = buffer_sub_range;
	private:
		inout_data sockets_{};
		std::vector<clear_info> clear_values_{};

	public:
		void add(const clear_info& info){
			resource_desc::buffer_requirement req;

			req.access = VK_ACCESS_TRANSFER_WRITE_BIT;

			sockets_.add(resource_desc::inout_type::inout, req);
			clear_values_.push_back(info);
		}

		[[nodiscard]] explicit buffer_fill(vk::context& ctx)
			: pass(ctx){
		}

		[[nodiscard]] buffer_fill(vk::context& ctx, std::initializer_list<clear_info> clear_infos)
			: pass(ctx){
			for (const auto & clear_info : clear_infos){
				add(clear_info);
			}
		}
		[[nodiscard]] buffer_fill(vk::context& ctx, std::size_t count)
			: pass(ctx){
			for(std::size_t i = 0; i < count; ++i){
				add({});
			}
		}

		inout_data& sockets() noexcept override{
			return sockets_;
		}

		void record_command(vk::context& context, const pass_resource_reference* resources, math::u32size2 extent,
			VkCommandBuffer buffer) override{

			//TODO merge image subrange with the same image entity
			for (auto && [idx, clear_value] : clear_values_ | std::views::enumerate){
				auto& buf = std::get<resource_desc::buffer_entity>(resources->at_in(idx).resource);

				const auto rngOff = buf.buffer.begin() + clear_value.off;
				assert(rngOff <= buf.buffer.end());
				const auto rngEnd = std::min(buf.buffer.end(), rngOff + clear_value.size);
				const auto rngSize = rngEnd - rngOff;
				vkCmdFillBuffer(buffer, buf.buffer.buffer, rngOff - buf.buffer.address, rngSize, clear_value.value);
			}
		}
	};

}
