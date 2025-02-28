module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk.command_pool;

import mo_yanxi.handle_wrapper;
import mo_yanxi.vk.command_buffer;
import mo_yanxi.vk.exception;
import std;

export namespace mo_yanxi::vk{


	struct command_pool : exclusive_handle<VkCommandPool>{
	protected:
		exclusive_handle_member<VkDevice> device{};

	public:
		[[nodiscard]] command_pool() = default;

		~command_pool(){
			if(device)vkDestroyCommandPool(device, handle, nullptr);
		}

		command_pool(const command_pool& other) = delete;
		command_pool(command_pool&& other) noexcept = default;
		command_pool& operator=(const command_pool& other) = delete;
		command_pool& operator=(command_pool&& other) = default;

		command_pool(
			VkDevice device,
			const std::uint32_t queueFamilyIndex,
			const VkCommandPoolCreateFlags flags
		) : device{device}{
			VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
			poolInfo.queueFamilyIndex = queueFamilyIndex;
			poolInfo.flags = flags; // Optional

			if(auto rst = vkCreateCommandPool(device, &poolInfo, nullptr, &handle)){
				throw vk_error(rst, "Failed to create command pool!");
			}
		}

		[[nodiscard]] VkDevice get_device() const noexcept{
			return device;
		}

		[[nodiscard]] command_buffer obtain(const VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const{
			return {device, handle, level};
		}
		//
		// [[nodiscard]] BlockedTransientCommand getTransient(VkQueue targetQueue) const{
		// 	return BlockedTransientCommand{device, handle, targetQueue};
		// }

		void reset_all(const VkCommandPoolResetFlags resetFlags = 0) const{
			vkResetCommandPool(device, handle, resetFlags);
		}

		// [[nodiscard]] std::vector<CommandBuffer> obtainArray(const std::size_t count, const VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const{
		// 	std::vector<CommandBuffer> arr{};
		// 	arr.reserve(count);
		//
		// 	for(std::size_t i = 0; i < count; ++i){
		// 		arr.push_back(obtain(level));
		// 	}
		//
		// 	return arr;
		// }
	};
}
