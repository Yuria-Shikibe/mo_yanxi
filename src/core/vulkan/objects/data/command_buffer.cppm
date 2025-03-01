module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk.command_buffer;

import mo_yanxi.handle_wrapper;
import mo_yanxi.vk.exception;
import std;

namespace mo_yanxi::vk{
	namespace cmd{
		export void begin(
			VkCommandBuffer buffer,
			const VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			const VkCommandBufferInheritanceInfo& inheritance = {}){
			VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

			beginInfo.flags = flags;
			beginInfo.pInheritanceInfo = &inheritance; // Optional

			if(const auto rst = vkBeginCommandBuffer(buffer, &beginInfo)){
				throw vk_error(rst, "Failed to begin recording command buffer!");
			}
		}

		export void end(VkCommandBuffer buffer){
			if(const auto rst = vkEndCommandBuffer(buffer)){
				throw vk_error(rst, "Failed to record command buffer!");
			}
		}
	}


	struct command_pool;

	export
	struct command_buffer : exclusive_handle<VkCommandBuffer>{
	protected:
		exclusive_handle_member<VkDevice> device{};
		exclusive_handle_member<VkCommandPool> pool{};

	public:
		[[nodiscard]] command_buffer() = default;

		command_buffer(const command_pool& command_pool, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

		command_buffer(
			VkDevice device,
			VkCommandPool commandPool,
			const VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
		) : device{device}, pool{commandPool}{
			const VkCommandBufferAllocateInfo allocInfo{
					.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
					.pNext = nullptr,
					.commandPool = commandPool,
					.level = level,
					.commandBufferCount = 1
				};

			if(const auto rst = vkAllocateCommandBuffers(device, &allocInfo, &handle)){
				throw vk_error(rst, "Failed to allocate command buffers!");
			}
		}

		[[nodiscard]] VkDevice get_device() const noexcept{
			return device;
		}

		[[nodiscard]] VkCommandPool get_pool() const noexcept{
			return pool;
		}

		~command_buffer(){
			if(device && pool && handle) vkFreeCommandBuffers(device, pool, 1, &handle);
		}

		command_buffer(const command_buffer& other) = delete;

		command_buffer& operator=(const command_buffer& other) = delete;

		command_buffer(command_buffer&& other) noexcept = default;
		command_buffer& operator=(command_buffer&& other) noexcept = default;
		// command_buffer& operator=(command_buffer&& other) noexcept{
		// 	if(this == &other) return *this;
		// 	if(device && pool && handle)vkFreeCommandBuffers(device, pool, 1, &handle);
		// 	handle = nullptr;
		// 	pool = nullptr;
		// 	device = nullptr;
		// 	wrapper::operator=(std::move(other));
		// 	device = std::move(other.device);
		// 	pool = std::move(other.pool);
		// 	return *this;
		// }

		[[nodiscard]] VkCommandBufferSubmitInfo submit_info() const noexcept{
			return {
					.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
					.pNext = nullptr,
					.commandBuffer = handle,
					.deviceMask = 0
				};
		}

		void begin(
			const VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			const VkCommandBufferInheritanceInfo& inheritance = {}) const{
			cmd::begin(get(), flags, inheritance);
		}

		void end() const{
			cmd::end(get());
		}

		template <typename T, typename... Args>
			requires std::invocable<T, VkCommandBuffer, const Args&...>
		void push(T fn, const Args&... args) const{
			std::invoke(fn, handle, args...);
		}

		[[deprecated]] void blitDraw() const{
			vkCmdDraw(handle, 3, 1, 0, 0);
		}

		void reset(VkCommandBufferResetFlagBits flagBits = static_cast<VkCommandBufferResetFlagBits>(0)) const{
			vkResetCommandBuffer(handle, flagBits);
		}

		[[deprecated]] void setViewport(const VkViewport& viewport) const{
			vkCmdSetViewport(handle, 0, 1, &viewport);
		}

		[[deprecated]] void setScissor(const VkRect2D& scissor) const{
			vkCmdSetScissor(handle, 0, 1, &scissor);
		}
	};

	export
	struct [[jetbrains::guard]] scoped_recorder{
	private:
		VkCommandBuffer handler;

	public:
		[[nodiscard]] explicit(false) scoped_recorder(VkCommandBuffer handler,
		                                              const VkCommandBufferUsageFlags flags =
			                                              VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		                                              const VkCommandBufferInheritanceInfo& inheritance = {})
			: handler{handler}{
			cmd::begin(handler, flags, inheritance);
		}

		~scoped_recorder(){
			cmd::end(handler);
		}

		explicit(false) operator VkCommandBuffer() const noexcept{
			return handler;
		}

		[[nodiscard]] VkCommandBuffer get() const noexcept{
			return handler;
		}

		scoped_recorder(const scoped_recorder& other) = delete;
		scoped_recorder(scoped_recorder&& other) noexcept = delete;
		scoped_recorder& operator=(const scoped_recorder& other) = delete;
		scoped_recorder& operator=(scoped_recorder&& other) noexcept = delete;
	};

	export
	struct transient_command : command_buffer{
		static constexpr VkCommandBufferBeginInfo BeginInfo{
				VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				nullptr,
				VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
			};

	private:
		VkQueue targetQueue{};
		VkFence fence{};

	public:
		[[nodiscard]] transient_command() = default;

		[[nodiscard]] transient_command(command_buffer&& commandBuffer, VkQueue targetQueue, VkFence fence = nullptr) :
			command_buffer{
				std::move(commandBuffer)
			}, targetQueue{targetQueue}, fence(fence){
			vkBeginCommandBuffer(handle, &BeginInfo);
		}

		[[nodiscard]] transient_command(
			VkDevice device,
			VkCommandPool commandPool,
			VkQueue targetQueue, VkFence fence = nullptr)
			: command_buffer{device, commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY},
			  targetQueue{targetQueue}, fence(fence){
			vkBeginCommandBuffer(handle, &BeginInfo);
		}

		[[nodiscard]] transient_command(
			const command_pool& command_pool,
			VkQueue targetQueue, VkFence fence = nullptr);

		~transient_command() noexcept(false){
			submit();
		}

		transient_command(const transient_command& other) = delete;

		transient_command& operator=(const transient_command& other) = delete;

		transient_command(transient_command&& other) noexcept = default;

		transient_command& operator=(transient_command&& other) noexcept{
			if(this == &other) return *this;
			std::swap(static_cast<command_buffer&>(*this), static_cast<command_buffer&>(other));
			std::swap(fence, other.fence);
			std::swap(targetQueue, other.targetQueue);
			return *this;
		}

	private:
		void submit(){
			if(!handle) return;

			vkEndCommandBuffer(handle);

			const VkSubmitInfo submitInfo{
					.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
					.pNext = nullptr,
					.waitSemaphoreCount = 0,
					.pWaitSemaphores = nullptr,
					.pWaitDstStageMask = nullptr,
					.commandBufferCount = 1,
					.pCommandBuffers = &handle,
					.signalSemaphoreCount = 0,
					.pSignalSemaphores = nullptr
				};

			vkQueueSubmit(targetQueue, 1, &submitInfo, fence);
			if(fence){
				if(const auto rst = vkWaitForFences(device, 1, &fence, true, std::numeric_limits<uint64_t>::max())){
					throw vk_error{rst, "Failed To Wait Fence On Transient Command"};
				}
				if(const auto rst = vkResetFences(device, 1, &fence)){
					throw vk_error{rst, "Failed To Reset Fence On Transient Command"};
				}
			}else{
				vkQueueWaitIdle(targetQueue);
			}
		}
	};
}
