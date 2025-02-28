module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk.util.cmd.resources;

import std;
import mo_yanxi.vk.exception;
import mo_yanxi.vk.concepts;

namespace mo_yanxi::vk::cmd{
	export
	template <typename T>
		requires requires(T& t){
			t.srcAccessMask;
			t.dstAccessMask;
			t.srcStageMask;
			t.dstStageMask;

			std::swap(t.srcAccessMask, t.dstAccessMask);
			std::swap(t.srcStageMask, t.dstStageMask);
		}
	void swap_stage(T& t){
		std::swap(t.srcAccessMask, t.dstAccessMask);
		std::swap(t.srcStageMask, t.dstStageMask);
	}

	export
	template <>
	void swap_stage(VkImageMemoryBarrier2& t){
		std::swap(t.srcAccessMask, t.dstAccessMask);
		std::swap(t.srcStageMask, t.dstStageMask);
		std::swap(t.srcQueueFamilyIndex, t.dstQueueFamilyIndex);
		std::swap(t.oldLayout, t.newLayout);
	}

	export
	template <std::ranges::range Rng>
		requires (std::same_as<std::ranges::range_reference_t<Rng>, VkImageMemoryBarrier2&>)
	void swap_stage(Rng&& arr){
		for (VkImageMemoryBarrier2& t : arr){
			swap_stage(t);
		}
	}

	export
	template <std::ranges::range Rng>
		requires (std::same_as<std::ranges::range_reference_t<Rng>, VkBufferMemoryBarrier2&>)
	void swap_stage(Rng&& arr){
		for (VkBufferMemoryBarrier2& t : arr){
			swap_stage<VkBufferMemoryBarrier2>(t);
		}
	}


	export
	template <contigious_range_of<VkImageBlit> Rng = std::initializer_list<VkImageBlit>>
	void image_blit(VkCommandBuffer command_buffer,
	                VkImage src, VkImageLayout srcExpectedLayout,
	                VkImage dst, VkImageLayout dstExpectedLayout,
	                VkFilter filter_type,
	                const Rng& rng
	){
		// VkBlitImageInfo
		::vkCmdBlitImage(command_buffer, src, srcExpectedLayout, dst, dstExpectedLayout,
		                 static_cast<std::uint32_t>(std::ranges::size(rng)),
		                 std::ranges::data(rng),
		                 filter_type
		);
	}
	export
	template <contigious_range_of<VkImageSubresourceRange> Rng = std::initializer_list<VkImageSubresourceRange>>
	void clear_color_image(VkCommandBuffer command_buffer,
	                VkImage image, VkImageLayout expectedLayout,
	                const VkClearColorValue& clearValue,
	                const Rng& rng
	){
		::vkCmdClearColorImage(command_buffer, image, expectedLayout,
			&clearValue,
			static_cast<std::uint32_t>(std::ranges::size(rng)),
			std::ranges::data(rng)
		);
	}

	export
	template <contigious_range_of<VkImageCopy> Rng = std::initializer_list<VkImageCopy>>
	void copy_image_to_image(VkCommandBuffer command_buffer,
	                         VkImage src, VkImageLayout srcExpectedLayout,
	                         VkImage dst, VkImageLayout dstExpectedLayout,
	                         const Rng& rng
	){
		::vkCmdCopyImage(command_buffer, src, srcExpectedLayout, dst, dstExpectedLayout,
		                 static_cast<std::uint32_t>(std::ranges::size(rng)),
		                 std::ranges::data(rng)
		);
	}

	export
	template <contigious_range_of<VkBufferImageCopy> Rng = std::initializer_list<VkBufferImageCopy>>
	void copy_buffer_to_image(
		VkCommandBuffer command_buffer, VkBuffer src, VkImage dst, VkImageLayout expectedLayout, const Rng& rng){
		::vkCmdCopyBufferToImage(command_buffer, src, dst, expectedLayout,
		                         static_cast<std::uint32_t>(std::ranges::size(rng)),
		                         std::ranges::data(rng)
		);
	}

	export
	template <contigious_range_of<VkBufferImageCopy> Rng = std::initializer_list<VkBufferImageCopy>>
	void copy_image_to_buffer(
		VkCommandBuffer command_buffer, VkImage src, VkBuffer dst, VkImageLayout expectedLayout, const Rng& rng){
		::vkCmdCopyImageToBuffer(command_buffer, src, expectedLayout, dst,
		                         static_cast<std::uint32_t>(std::ranges::size(rng)),
		                         std::ranges::data(rng)
		);
	}

	export
	template <contigious_range_of<VkBufferCopy> Rng = std::initializer_list<VkBufferCopy>>
	void copy_buffer(VkCommandBuffer command_buffer, VkBuffer src, VkBuffer dst, Rng&& rng) noexcept{
		::vkCmdCopyBuffer(
			command_buffer, src, dst,
			static_cast<std::uint32_t>(std::ranges::size(rng)),
			std::ranges::data(rng));
	}

	export
	struct dependency_gen{
		std::vector<VkMemoryBarrier2> memory_barriers;
		std::vector<VkBufferMemoryBarrier2> buffer_memory_barriers;
		std::vector<VkImageMemoryBarrier2> image_memory_barriers;

		void clear() noexcept{
			memory_barriers.clear();
			buffer_memory_barriers.clear();
			image_memory_barriers.clear();
		}

		[[nodiscard]] VkDependencyInfo create() const noexcept{
			return {
					.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
					.pNext = nullptr,
					.dependencyFlags = 0,
					.memoryBarrierCount = static_cast<std::uint32_t>(memory_barriers.size()),
					.pMemoryBarriers = memory_barriers.empty() ? nullptr : memory_barriers.data(),
					.bufferMemoryBarrierCount = static_cast<std::uint32_t>(buffer_memory_barriers.size()),
					.pBufferMemoryBarriers = buffer_memory_barriers.empty()
						                         ? nullptr
						                         : buffer_memory_barriers.data(),
					.imageMemoryBarrierCount = static_cast<std::uint32_t>(image_memory_barriers.size()),
					.pImageMemoryBarriers = image_memory_barriers.empty() ? nullptr : image_memory_barriers.data()
				};
		}

		void apply(VkCommandBuffer command_buffer, bool reserve = false) noexcept{
			const auto d = create();
			vkCmdPipelineBarrier2(command_buffer, &d);
			if(!reserve){
				clear();
			}
		}

		void push_staging(
			const VkBuffer staging_buffer,
			const std::uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			const std::uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			const VkDeviceSize offset = 0,
			const VkDeviceSize size = VK_WHOLE_SIZE){
			push(staging_buffer,
			     VK_PIPELINE_STAGE_2_HOST_BIT,
			     VK_ACCESS_2_HOST_WRITE_BIT,
			     VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			     VK_ACCESS_2_TRANSFER_READ_BIT,
			     srcQueueFamilyIndex,
			     dstQueueFamilyIndex,
			     offset,
			     size
			);
		}

		void push_on_initialization(
			const VkBuffer buffer_first_init,
			const std::uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			const std::uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			const VkDeviceSize offset = 0,
			const VkDeviceSize size = VK_WHOLE_SIZE){
			push(buffer_first_init,
			     VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
			     VK_ACCESS_2_NONE,
			     VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			     VK_ACCESS_2_TRANSFER_WRITE_BIT,
			     srcQueueFamilyIndex,
			     dstQueueFamilyIndex,
			     offset,
			     size
			);
		}

		void push_post_write(
			const VkBuffer buffer_after_write,
			const VkPipelineStageFlags2 dstStageMask,
			const VkAccessFlags2 dstAccessMask,
			const std::uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			const std::uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			const VkDeviceSize offset = 0,
			const VkDeviceSize size = VK_WHOLE_SIZE){
			push(buffer_after_write,
			     VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			     VK_ACCESS_2_TRANSFER_WRITE_BIT,
			     dstStageMask,
			     dstAccessMask,
			     srcQueueFamilyIndex,
			     dstQueueFamilyIndex,
			     offset,
			     size
			);
		}
		
		void push_post_write(
			const VkImage buffer_after_write,
			const VkPipelineStageFlags2 dstStageMask,
			const VkAccessFlags2 dstAccessMask,
			const VkImageLayout newLayout,
			const VkImageSubresourceRange& range,
			const std::uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			const std::uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED){
			push(buffer_after_write,
			     VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			     VK_ACCESS_2_TRANSFER_WRITE_BIT,
			     dstStageMask,
			     dstAccessMask,
			     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			     newLayout,
			     range,
			     srcQueueFamilyIndex,
			     dstQueueFamilyIndex
			);
		}

		void push(
			const VkBuffer buffer,
			const VkPipelineStageFlags2 srcStageMask,
			const VkAccessFlags2 srcAccessMask,
			const VkPipelineStageFlags2 dstStageMask,
			const VkAccessFlags2 dstAccessMask,
			const std::uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			const std::uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			const VkDeviceSize offset = 0,
			const VkDeviceSize size = VK_WHOLE_SIZE
		){
			buffer_memory_barriers.push_back({
					.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
					.pNext = nullptr,
					.srcStageMask = srcStageMask,
					.srcAccessMask = srcAccessMask,
					.dstStageMask = dstStageMask,
					.dstAccessMask = dstAccessMask,
					.srcQueueFamilyIndex = srcQueueFamilyIndex,
					.dstQueueFamilyIndex = dstQueueFamilyIndex,
					.buffer = buffer,
					.offset = offset,
					.size = size
				});
		}

		void push(
			const VkImage image,
			const VkPipelineStageFlags2 srcStageMask,
			const VkAccessFlags2 srcAccessMask,
			const VkPipelineStageFlags2 dstStageMask,
			const VkAccessFlags2 dstAccessMask,
			const VkImageLayout oldLayout,
			const VkImageLayout newLayout,
			const VkImageSubresourceRange& range,
			const std::uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			const std::uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED
		){
			image_memory_barriers.push_back({
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
					.pNext = nullptr,
					.srcStageMask = srcStageMask,
					.srcAccessMask = srcAccessMask,
					.dstStageMask = dstStageMask,
					.dstAccessMask = dstAccessMask,
					.oldLayout = oldLayout,
					.newLayout = newLayout,
					.srcQueueFamilyIndex = srcQueueFamilyIndex,
					.dstQueueFamilyIndex = dstQueueFamilyIndex,
					.image = image,
					.subresourceRange = range
				});
		}
	};

	export
	void memory_barrier(
		const VkCommandBuffer command_buffer,
		const VkImage image,
		const VkPipelineStageFlags2 srcStageMask,
		const VkAccessFlags2 srcAccessMask,
		const VkPipelineStageFlags2 dstStageMask,
		const VkAccessFlags2 dstAccessMask,
		const VkImageLayout oldLayout,
		const VkImageLayout newLayout,
		const VkImageSubresourceRange& range,
		const std::uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		const std::uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED
	){
		VkImageMemoryBarrier2 barrier2{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				.pNext = nullptr,
				.srcStageMask = srcStageMask,
				.srcAccessMask = srcAccessMask,
				.dstStageMask = dstStageMask,
				.dstAccessMask = dstAccessMask,
				.oldLayout = oldLayout,
				.newLayout = newLayout,
				.srcQueueFamilyIndex = srcQueueFamilyIndex,
				.dstQueueFamilyIndex = dstQueueFamilyIndex,
				.image = image,
				.subresourceRange = range
			};

		VkDependencyInfo dependency{
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.imageMemoryBarrierCount = 1,
				.pImageMemoryBarriers = &barrier2,
			};

		vkCmdPipelineBarrier2(command_buffer, &dependency);
	}

	export
	void memory_barrier(
		const VkCommandBuffer command_buffer,
		const VkBuffer buffer,
		const VkPipelineStageFlags2 srcStageMask,
		const VkAccessFlags2 srcAccessMask,
		const VkPipelineStageFlags2 dstStageMask,
		const VkAccessFlags2 dstAccessMask,
		const std::uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		const std::uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		const VkDeviceSize offset = 0,
		const VkDeviceSize size = VK_WHOLE_SIZE
	){
		VkBufferMemoryBarrier2 barrier2{
				.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
				.pNext = nullptr,
				.srcStageMask = srcStageMask,
				.srcAccessMask = srcAccessMask,
				.dstStageMask = dstStageMask,
				.dstAccessMask = dstAccessMask,
				.srcQueueFamilyIndex = srcQueueFamilyIndex,
				.dstQueueFamilyIndex = dstQueueFamilyIndex,
				.buffer = buffer,
				.offset = offset,
				.size = size
			};

		VkDependencyInfo dependency{
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.bufferMemoryBarrierCount = 1,
				.pBufferMemoryBarriers = &barrier2,
			};

		vkCmdPipelineBarrier2(command_buffer, &dependency);
	}
}
