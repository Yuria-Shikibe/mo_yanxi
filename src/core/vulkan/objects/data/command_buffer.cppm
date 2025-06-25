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


	export
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
		// command_buffer& operator=(command_buffer&& other) noexcept = default;
		command_buffer& operator=(command_buffer&& other) noexcept{
			if(this == &other) return *this;
			if(device && pool && handle)vkFreeCommandBuffers(device, pool, 1, &handle);

			handle = nullptr;
			pool = nullptr;
			device = nullptr;
			wrapper::operator=(std::move(other));
			device = std::move(other.device);
			pool = std::move(other.pool);
			return *this;
		}

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

	export
	struct command_chunk{
	private:
		VkDevice device{};
		VkCommandPool pool{};

	public:
		struct unit{
			friend command_chunk;
		private:
			VkCommandBuffer command_buffer{};
			VkSemaphore semaphore{};
			std::uint64_t counting{};

		public:
			explicit(false) operator VkCommandBuffer() const noexcept{
				return command_buffer;
			}

			void submit(VkQueue queue, VkPipelineStageFlags2 signal_stage) & noexcept{
				VkSemaphoreSubmitInfo to_signal{
						.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
						.semaphore = semaphore,
						.value = ++counting,
						.stageMask = signal_stage
					};

				const VkCommandBufferSubmitInfo cmd_info{
						.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
						.commandBuffer = command_buffer,
					};

				VkSubmitInfo2 submit_info{
						.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
						.waitSemaphoreInfoCount = 0,
						.pWaitSemaphoreInfos = nullptr,
						.commandBufferInfoCount = 1,
						.pCommandBufferInfos = &cmd_info,
						.signalSemaphoreInfoCount = 1,
						.pSignalSemaphoreInfos = &to_signal
					};

				vkQueueSubmit2(queue, 1, &submit_info, nullptr);
			}

			void wait(VkDevice device, std::uint64_t timeout = std::numeric_limits<std::uint64_t>::max()) const noexcept{
				const VkSemaphoreWaitInfo info{
					.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
					.pNext = nullptr,
					.flags = 0,
					.semaphoreCount = 1,
					.pSemaphores = &semaphore,
					.pValues = &counting
				};

				vkWaitSemaphores(device, &info, timeout);
			}

			[[nodiscard]] unit() = default;

			unit(const unit& other) = delete;
			unit(unit&& other) noexcept = default;
			unit& operator=(const unit& other) = delete;
			unit& operator=(unit&& other) noexcept = default;
		};

	private:
		std::vector<unit> units{};

	public:
		using container_type = decltype(units);

		[[nodiscard]] command_chunk() = default;

		[[nodiscard]] command_chunk(VkDevice device, VkCommandPool pool, const std::size_t chunk_count)
			: device(device), pool(pool), units{chunk_count}
		{
			for (auto&& value : units){
				const VkCommandBufferAllocateInfo allocInfo{
					.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
					.pNext = nullptr,
					.commandPool = pool,
					.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
					.commandBufferCount = 1
				};

				if(const auto rst = vkAllocateCommandBuffers(device, &allocInfo, &value.command_buffer)){
					throw vk_error(rst, "Failed to allocate command buffers!");
				}

				constexpr VkSemaphoreTypeCreateInfo semaphore_create_info{
					.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
					.pNext = nullptr,
					.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
					.initialValue = 0
				};

				const VkSemaphoreCreateInfo semaphoreInfo{
					.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
					.pNext = &semaphore_create_info,
					.flags = 0
				};

				if(const auto rst = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &value.semaphore)){
					throw vk_error(rst, "Failed to create semaphore!");
				}


			}
		}

	private:
		void free() const noexcept{
			if(!device || !pool) return;

			for (auto&& value : units){
				vkDestroySemaphore(device, value.semaphore, nullptr);
				vkFreeCommandBuffers(device, pool, 1, &value.command_buffer);
			}
		}

	public:

		~command_chunk(){
			free();
		}

		command_chunk(const command_chunk& other) = delete;
		command_chunk& operator=(const command_chunk& other) = delete;

		command_chunk(command_chunk&& other) noexcept
			: device{std::exchange(other.device, {})},
			  pool{std::exchange(other.pool, {})},
			  units{std::move(other.units)}{
		}

		command_chunk& operator=(command_chunk&& other) noexcept{
			if(this == &other) return *this;
			free();
			device = std::exchange(other.device, {});
			pool = std::exchange(other.pool, {});
			units = std::move(other.units);
			return *this;
		}

		[[nodiscard]] container_type::const_iterator begin() const noexcept{
			return units.begin();
		}

		[[nodiscard]] container_type::const_iterator end() const noexcept{
			return units.end();
		}

		[[nodiscard]] unit& operator[](const std::size_t index) noexcept{
			return units[index];
		}

		[[nodiscard]] container_type::size_type size() const noexcept{
			return units.size();
		}
	};


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

		[[nodiscard]] transient_command get_transient(VkQueue targetQueue, VkFence fence = nullptr) const{
			return transient_command{device, handle, targetQueue, fence};
		}

		void reset_all(const VkCommandPoolResetFlags resetFlags = 0) const{
			vkResetCommandPool(device, handle, resetFlags);
		}

	};
}
