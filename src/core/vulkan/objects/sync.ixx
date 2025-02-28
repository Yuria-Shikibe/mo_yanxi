module;

#include <cassert>
#include <vulkan/vulkan.h>

export module mo_yanxi.vk.sync;

import std;
import mo_yanxi.vk.exception;
import mo_yanxi.handle_wrapper;

namespace mo_yanxi::vk{
	export
	struct fence : public exclusive_handle<VkFence>{
	private:
		exclusive_handle_member<VkDevice> device{};

	public:
		[[nodiscard]] fence() = default;

		[[nodiscard]] explicit fence(VkDevice device, const bool create_signal) : device{device}{
			const VkFenceCreateInfo fenceInfo{
					.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
					.pNext = nullptr,
					.flags = create_signal ? VK_FENCE_CREATE_SIGNALED_BIT : VkFenceCreateFlags{}
				};

			if(vkCreateFence(device, &fenceInfo, nullptr, &handle) != VK_SUCCESS){
				throw std::runtime_error("Failed to create fence!");
			}
		}

		[[nodiscard]] VkResult get_status() const noexcept{
			assert(handle && device);
			vkGetFenceStatus(device, handle);
		}

		void reset() const{
			if(const auto rst = vkResetFences(device, 1, &handle)){
				throw vk_error{rst, "Failed to reset fence"};
			}
		}

		void wait(const std::uint64_t timeout = std::numeric_limits<std::uint64_t>::max()) const{
			if(const auto rst = vkWaitForFences(device, 1, &handle, true, timeout)){
				throw vk_error{rst, "Failed to wait fence"};
			}
		}

		void wait_and_reset(const std::uint64_t timeout = std::numeric_limits<std::uint64_t>::max()) const{
			wait(timeout);
			reset();
		}

		[[nodiscard]] VkDevice get_device() const noexcept{
			return device;
		}

		fence(const fence& other) = delete;
		fence(fence&& other) noexcept = default;
		fence& operator=(const fence& other) = delete;
		fence& operator=(fence&& other) noexcept = default;

		~fence(){
			if(device) vkDestroyFence(device, handle, nullptr);
		}
	};

	export
	struct semaphore : exclusive_handle<VkSemaphore>{
	private:
		exclusive_handle_member<VkDevice> device{};

	public:
		[[nodiscard]] semaphore() = default;

		[[nodiscard]] explicit semaphore(VkDevice device) : device{device}{
			VkSemaphoreCreateInfo semaphoreInfo{
					.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0
				};

			if(const auto rst = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &handle)){
				throw vk_error(rst, "Failed to create Semaphore!");
			}
		}


		semaphore(const semaphore& other) = delete;
		semaphore(semaphore&& other) noexcept = default;
		semaphore& operator=(const semaphore& other) = delete;
		semaphore& operator=(semaphore&& other) = default;

		~semaphore(){
			if(device) vkDestroySemaphore(device, handle, nullptr);
		}

		[[nodiscard]] VkDevice get_device() const noexcept{
			return device;
		}

		[[nodiscard]] VkSemaphoreSubmitInfo get_submit_info(const VkPipelineStageFlags2 flags) const{
			return {
					.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
					.pNext = nullptr,
					.semaphore = handle,
					.value = 0,
					.stageMask = flags,
					.deviceIndex = 0
				};
		}

		void signal(const std::uint64_t value = 0) const{
			const VkSemaphoreSignalInfo signalInfo{
					.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
					.pNext = nullptr,
					.semaphore = handle,
					.value = value
				};

			if(const auto rst = vkSignalSemaphore(device, &signalInfo)){
				throw vk_error(rst, "Failed to signal semaphore value");
			}
		}

		[[nodiscard]] std::uint64_t get_counter_value() const{
			std::uint64_t value;
			if(const auto rst = vkGetSemaphoreCounterValue(device, handle, &value)){
				throw vk_error(rst, "Failed to get semaphore value");
			}
			return value;
		}

		// void wait() const{
		// 	const VkSemaphoreWaitInfo info{
		// 		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
		// 		.pNext = nullptr,
		// 		.flags = ,
		// 		.semaphoreCount = ,
		// 		.pSemaphores = ,
		// 		.pValues =
		// 	}
		//
		// 	vkWaitSemaphores(device, &signalInfo);
		// }
	};

	export
	struct event : exclusive_handle<VkEvent>{
	private:
		exclusive_handle_member<VkDevice> device{};

	public:
		~event(){
			if(device) vkDestroyEvent(device, handle, nullptr);
		}

		[[nodiscard]] event() = default;

		[[nodiscard]] explicit event(VkDevice device, const bool isDeviceOnly = true) : device(device){
			const VkEventCreateInfo info{
					.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO,
					.pNext = nullptr,
					.flags = isDeviceOnly ? VK_EVENT_CREATE_DEVICE_ONLY_BIT : 0u
				};

			if(const auto rst = vkCreateEvent(device, &info, nullptr, &handle)){
				throw vk_error(rst, "Failed to create vk::event!");
			}
		}

		void set() const{
			if(const auto rst = vkSetEvent(device, handle)){
				throw vk_error(rst, "Failed to set vk::event!");
			}
		}

		void cmd_set(VkCommandBuffer commandBuffer, const VkDependencyInfo& dependencyInfo) const noexcept{
			vkCmdSetEvent2(commandBuffer, handle, &dependencyInfo);
		}

		void cmd_reset(VkCommandBuffer commandBuffer, const VkPipelineStageFlags2 flags) const noexcept{
			vkCmdResetEvent2(commandBuffer, handle, flags);
		}

		void cmdWait(VkCommandBuffer commandBuffer, const VkDependencyInfo& dependencyInfo) const noexcept{
			vkCmdWaitEvents2(commandBuffer, 1, &handle, &dependencyInfo);
		}

		void reset() const{
			if(const auto rst = vkResetEvent(device, handle)){
				throw vk_error(rst, "Failed to reset vk::event!");
			}
		}

		[[nodiscard]] VkResult get_status() const{
			switch(auto rst = get_status_noexcept()){
			case VK_ERROR_OUT_OF_HOST_MEMORY :
			case VK_ERROR_OUT_OF_DEVICE_MEMORY :
			case VK_ERROR_DEVICE_LOST : throw vk_error(rst, "Failed to get vk::event status!");
			default : return rst;
			}
		}

		[[nodiscard]] VkResult get_status_noexcept() const noexcept{
			return vkGetEventStatus(device, handle);
		}

		[[nodiscard]] bool is_set() const{
			return get_status() == VK_EVENT_SET;
		}

		[[nodiscard]] bool is_reset() const{
			return get_status() == VK_EVENT_RESET;
		}

		[[nodiscard]] VkDevice get_device() const noexcept{
			return device;
		}

		event(const event& other) = delete;

		event(event&& other) noexcept = default;

		event& operator=(const event& other) = delete;

		event& operator=(event&& other) noexcept = default;
	};
}
