module;

#include <cassert>
#include <vulkan/vulkan.h>

export module mo_yanxi.graphic.render_graph.fill;

export import mo_yanxi.graphic.render_graph.manager;

import mo_yanxi.vk.context;
import mo_yanxi.vk.image_derives;
import mo_yanxi.vk.ext;
import mo_yanxi.math.vector2;
import std;


namespace mo_yanxi::graphic::render_graph{
	export
	template <typename ClearValue>
	struct image_clear_info{
		ClearValue value{};
		std::uint32_t mip_level_base{0};
		std::uint32_t mip_level_count{1};
	};

	struct buffer_fill_info{
		std::uint32_t value{};
		VkDeviceSize off{};
		VkDeviceSize size{std::numeric_limits<VkDeviceSize>::max()};
	};

	export
	template <typename Impl, typename ClearType>
	struct generic_resource_fill : pass_meta{
		using clear_info = ClearType;

	protected:
		pass_inout_connection sockets_{};

	private:
		std::vector<clear_info> clear_values_{};

	protected:

		const pass_inout_connection& sockets() const noexcept override{
			return sockets_;
		}

		std::span<const clear_info> clear_value() const noexcept{
			return clear_values_;
		}

	public:

		void add(const clear_info& info){
			static_cast<Impl&>(*this).add_impl(info);
			clear_values_.push_back(info);
		}

		void add_impl(const clear_info& info) = delete;

		[[nodiscard]] explicit generic_resource_fill(vk::context& context)
			: pass_meta(context){
		}

		[[nodiscard]] generic_resource_fill(vk::context& ctx, std::initializer_list<clear_info> clear_infos)
			: pass_meta(ctx){
			for (const auto & clear_info : clear_infos){
				this->add(clear_info);
			}
		}

		[[nodiscard]] generic_resource_fill(vk::context& ctx, std::size_t count)
			: pass_meta(ctx){
			for(std::size_t i = 0; i < count; ++i){
				this->add({});
			}
		}
	};

	export
	struct image_clear : generic_resource_fill<image_clear, image_clear_info<VkClearColorValue>>{
		friend generic_resource_fill;
		using generic_resource_fill::generic_resource_fill;


	private:
		void add_impl(const clear_info& info){
			resource_desc::image_requirement req;
			req.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			req.override_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			req.decr = resource_desc::decoration::write;
			sockets_.add(true, true, req);
		}

	public:
		void record_command(vk::context& context, const pass_resource_reference& resources, math::u32size2 extent,
		                    VkCommandBuffer buffer) override{

			for (auto && [idx, clear_value] : clear_value() | std::views::enumerate){
				auto& img = resources.at_in(idx).as_image();

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
	struct depth_stencil_image_clear : generic_resource_fill<depth_stencil_image_clear, image_clear_info<VkClearDepthStencilValue>>{
		friend generic_resource_fill;
		using generic_resource_fill::generic_resource_fill;

	private:

		void add_impl(const clear_info& info){
			resource_desc::image_requirement req;
			req.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			req.override_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			req.decr = resource_desc::decoration::write;
			req.aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;

			sockets_.add(true, true, req);
		}

	public:

		void record_command(vk::context& context, const pass_resource_reference& resources, math::u32size2 extent,
			VkCommandBuffer buffer) override{

			//TODO merge image subrange with the same image entity
			for (auto && [idx, clear_value] : clear_value() | std::views::enumerate){
				auto& img = resources.at_in(idx).as_image();

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
	struct buffer_fill : generic_resource_fill<buffer_fill, buffer_fill_info>{
		friend generic_resource_fill;
		using generic_resource_fill::generic_resource_fill;

	private:
		void add_impl(const clear_info& info){
			resource_desc::buffer_requirement req;
			req.access = VK_ACCESS_TRANSFER_WRITE_BIT;

			sockets_.add(true, true, req);
		}

	public:
		void record_command(vk::context& context, const pass_resource_reference& resources, math::u32size2 extent,
			VkCommandBuffer buffer) override{

			//TODO merge image subrange with the same image entity
			for (auto && [idx, clear_value] : clear_value() | std::views::enumerate){
				auto& buf = resources.at_in(idx).as_buffer();

				const auto rngOff = buf.buffer.begin() + clear_value.off;
				assert(rngOff <= buf.buffer.end());
				const auto rngEnd = std::min(buf.buffer.end(), rngOff + clear_value.size);
				const auto rngSize = rngEnd - rngOff;
				vkCmdFillBuffer(buffer, buf.buffer.buffer, rngOff - buf.buffer.address, rngSize, clear_value.value);
			}
		}
	};

}
