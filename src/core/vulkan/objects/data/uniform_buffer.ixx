module;

#include <vulkan/vulkan.h>
#include <cassert>
#include <vk_mem_alloc.h>

export module mo_yanxi.vk.uniform_buffer;

import mo_yanxi.vk.resources;
import mo_yanxi.vk.vma;


namespace mo_yanxi::vk{
	export
	struct uniform_buffer : buffer{
		using buffer::buffer;

		[[nodiscard]] uniform_buffer(
			vk::allocator& allocator,
			const VkDeviceSize size,
			const VkBufferUsageFlags append_usage = 0)
			: buffer(allocator, {
				.sType =  VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = size,
				.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | append_usage
			}, {
				.usage = VMA_MEMORY_USAGE_CPU_TO_GPU
			}){
		}

		[[nodiscard]] VkDescriptorBufferInfo get_descriptor_info() const noexcept{
			return {
				.buffer = handle,
				.offset = 0,
				.range = get_size()
			};
		}

		[[nodiscard]] VkDescriptorBufferInfo get_descriptor_info(
			const VkDeviceSize offset,
			const VkDeviceSize range
		) const noexcept{
			assert(offset + range <= get_size());
			return {
				.buffer = handle,
				.offset = offset,
				.range = range
			};
		}
	};
}
