module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk.image_derives;

import std;
import mo_yanxi.handle_wrapper;
import mo_yanxi.vk.resources;
import mo_yanxi.vk.concepts;
import mo_yanxi.vk.vma;
import mo_yanxi.graphic.bitmap;

namespace mo_yanxi::vk{
	std::uint32_t getMipLevel(const std::uint32_t w, const std::uint32_t h) noexcept{
		return static_cast<std::uint32_t>(std::floor(std::log2(std::max(w, h)))) + 1u;
	}

	export
	struct texture_buffer_write{
		VkBuffer buffer;
		VkRect2D region;
	};

	export
	struct texture_bitmap_write{
		graphic::bitmap bitmap;
		math::point2 offset;
	};

	export
	struct texture{
	private:

		std::uint32_t mipLevel{};
		std::uint32_t layers{};

		image image;
		image_view image_view;
	public:
		[[nodiscard]] texture() = default;

		[[nodiscard]] texture(
			allocator& allocator,
			const VkExtent2D extent,
			const std::uint32_t layers = 1) :
			mipLevel(getMipLevel(extent.width, extent.height)),
			layers(layers),
			image(
				allocator,
				{extent.width, extent.height, 1},
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_FORMAT_R8G8B8A8_UNORM,
				mipLevel, layers, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D
			), image_view{allocator.get_device(), {
					.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
					.image = image.get(),
					.viewType = layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D,
					.format = VK_FORMAT_R8G8B8A8_UNORM,
					.components = {},
					.subresourceRange = VkImageSubresourceRange{
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.baseMipLevel = 0,
						.levelCount = mipLevel,
						.baseArrayLayer = 0,
						.layerCount = layers
					}
				}}{
		}

		[[nodiscard]] const vk::image& get_image() const noexcept{
			return image;
		}

		[[nodiscard]] const vk::image_view& get_image_view() const noexcept{
			return image_view;
		}

		[[nodiscard]] std::uint32_t get_mip_level() const noexcept{
			return mipLevel;
		}

		[[nodiscard]] std::uint32_t get_layers() const noexcept{
			return layers;
		}

		void init_layout(VkCommandBuffer command_buffer) const noexcept{
			image.init_layout_shader(
				command_buffer,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				mipLevel, layers
			);
		}

		template <range_of<texture_bitmap_write> Rng = std::initializer_list<texture_bitmap_write>>
		[[nodiscard("Staging buffers must be reserved until the device has finished the command")]]
		std::vector<buffer> write(
			VkCommandBuffer command_buffer,
			const Rng& args,
			const std::uint32_t baseLayer = 0,
			const std::uint32_t layerCount = 0 /*0 for all*/
			){
			if(std::ranges::empty(args))return {};

			std::vector<buffer> reservation{};
			std::vector<texture_buffer_write> trans{};
			reservation.reserve(std::ranges::size(args));
			trans.reserve(reservation.size());

			for(const texture_bitmap_write& arg : args){
				auto buffer = templates::create_staging_buffer(*image.get_allocator(), arg.bitmap.size_bytes());

				(void)buffer_mapper{buffer}.load_range(arg.bitmap.to_span());
				trans.emplace_back(buffer, VkRect2D{arg.offset.x, arg.offset.y, arg.bitmap.width(), arg.bitmap.height()});

				reservation.push_back(std::move(buffer));
			}

			write(command_buffer, trans, baseLayer, layerCount);

			return reservation;
		}

		template <range_of<texture_buffer_write> Rng = std::initializer_list<texture_buffer_write>>
		void write(
			VkCommandBuffer command_buffer,
			const Rng& args,
			const std::uint32_t baseLayer = 0,
			const std::uint32_t layerCount = 0 /*0 for all*/
			){

			if(std::ranges::empty(args))return;

			const auto lc = layerCount ? layerCount : layers;

			VkImageMemoryBarrier barrier{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
				.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = image,
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = mipLevel,
					.baseArrayLayer = baseLayer,
					.layerCount = lc
				}
			};

			vkCmdPipelineBarrier(
					command_buffer,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					0,
					0, nullptr,
					0, nullptr,
					1, &barrier);

			for(const texture_buffer_write& arg : args){
				cmd::copy_buffer_to_image(command_buffer, arg.buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {VkBufferImageCopy{
					.imageSubresource = {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.mipLevel = 0,
						.baseArrayLayer = baseLayer,
						.layerCount = lc
					},
					.imageOffset = {arg.region.offset.x, arg.region.offset.y, 0},
					.imageExtent = {arg.region.extent.width, arg.region.extent.height, 1},
				}});
			}

			if(std::ranges::size(args) == 1){
				std::array regions{static_cast<const texture_buffer_write&>(*std::ranges::begin(args)).region};
				cmd::generate_mipmaps(command_buffer, get_image(), regions, mipLevel, baseLayer, lc);
			}else{
				std::vector<VkRect2D> regions;
				cmd::generate_mipmaps(command_buffer, get_image(), regions, mipLevel, baseLayer, lc);
			}
		}
	};

	export
	struct color_attachment{
	private:
		VkImageUsageFlags usage;

		image image;
		image_view image_view;

	public:
		[[nodiscard]] color_attachment() = default;

		[[nodiscard]] color_attachment(
			allocator& allocator,
			const VkExtent2D extent, VkImageUsageFlags usage) :
			usage{usage},
			image(
				allocator,
				{extent.width, extent.height, 1},
				usage,
				VK_FORMAT_R8G8B8A8_UNORM,
				1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D
			), image_view{allocator.get_device(), {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = image.get(),
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = VK_FORMAT_R8G8B8A8_UNORM,
				.components = {},
				.subresourceRange = image::default_image_subrange
			}}{
		}

		void init_layout(VkCommandBuffer command_buffer) const noexcept{
			image.init_layout_shader(
				command_buffer,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			);
		}

		void resize(VkCommandBuffer command_buffer, VkExtent2D extent) noexcept{
			image = {
				*image.get_allocator(),
				{extent.width, extent.height, 1},
				usage,
				VK_FORMAT_R8G8B8A8_UNORM,
				1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D
			};

			image_view = {image.get_device(), {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = image.get(),
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = VK_FORMAT_R8G8B8A8_UNORM,
				.components = {},
				.subresourceRange = image::default_image_subrange
			}};

			image.init_layout_shader(
				command_buffer,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			);
		}

		[[nodiscard]] const vk::image& get_image() const noexcept{
			return image;
		}

		[[nodiscard]] const vk::image_view& get_image_view() const noexcept{
			return image_view;
		}
	};
}
