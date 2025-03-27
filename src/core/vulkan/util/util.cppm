module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk.util;

export import mo_yanxi.vk.util.invoker;
export import mo_yanxi.vk.exception;
import std;


namespace mo_yanxi::vk{
	// [[nodiscard]] constexpr std::uint32_t find_memory_type(
	// 		const
	// 		const std::uint32_t typeFilter,
	// 		const VkMemoryPropertyFlags properties
	// 	){
	// 	for(std::uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++){
	// 		if(typeFilter & (1u << i) && (deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties){
	// 			return i;
	// 		}
	// 	}
	//
	// 	throw unqualified_error("failed to find suitable memory type!");
	// }

	export std::vector<VkExtensionProperties> get_valid_extensions(){
		auto [extensions, rst] = enumerate(vkEnumerateInstanceExtensionProperties, nullptr);

		return extensions;
	}

	export
	template <std::ranges::range Rng>
		requires (std::convertible_to<std::ranges::range_value_t<Rng>, VkImageView>)
	[[nodiscard]] std::vector<VkDescriptorImageInfo> getDescriptorInfoRange_ShaderRead(VkSampler handle, const Rng& imageViews) noexcept{
		std::vector<VkDescriptorImageInfo> rst{};

		for (const auto& view : imageViews){
			rst.emplace_back(handle, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		return rst;
	}

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
	void swapStage(T& t){
		std::swap(t.srcAccessMask, t.dstAccessMask);
		std::swap(t.srcStageMask, t.dstStageMask);
	}

	export
	template <>
	void swapStage(VkImageMemoryBarrier2& t){
		std::swap(t.srcAccessMask, t.dstAccessMask);
		std::swap(t.srcStageMask, t.dstStageMask);
		std::swap(t.srcQueueFamilyIndex, t.dstQueueFamilyIndex);
		std::swap(t.oldLayout, t.newLayout);
	}

	export
	template <std::ranges::range Rng>
		requires (std::same_as<std::ranges::range_reference_t<Rng>, VkImageMemoryBarrier2&>)
	void swapStage(Rng&& arr){
		for (VkImageMemoryBarrier2& t : arr){
			swapStage(t);
		}
	}

	export
	template <std::ranges::range Rng>
		requires (std::same_as<std::ranges::range_reference_t<Rng>, VkBufferMemoryBarrier2&>)
	void swapStage(Rng&& arr){
		for (VkBufferMemoryBarrier2& t : arr){
			swapStage<VkBufferMemoryBarrier2>(t);
		}
	}

	export
	template <std::size_t size>
	void submitCommand(VkQueue queue, const std::array<VkCommandBuffer, size>& commandBuffer, VkFence fence = nullptr,
			VkSemaphore toWait = nullptr, VkPipelineStageFlags2 waitStage = VK_PIPELINE_STAGE_2_NONE,
			VkSemaphore toSignal = nullptr, VkPipelineStageFlags2 signalStage = VK_PIPELINE_STAGE_2_NONE
			){

		VkSemaphoreSubmitInfo semaphoreSubmitInfo_Wait{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = toWait,
			.value = 0,
			.stageMask = waitStage,
			.deviceIndex = 0
		};

		VkSemaphoreSubmitInfo semaphoreSubmitInfo_Signal{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = toSignal,
			.value = 0,
			.stageMask = signalStage,
			.deviceIndex = 0
		};

		std::array<VkCommandBufferSubmitInfo, size> cmds{};
		for (const auto& [i, cmd] : cmds | std::views::enumerate){
			cmd = VkCommandBufferSubmitInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
				.commandBuffer = commandBuffer[i],
				.deviceMask = 0
			};
		}


		VkSubmitInfo2 submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.commandBufferInfoCount = static_cast<std::uint32_t>(cmds.size()),
			.pCommandBufferInfos = cmds.data(),
		};

		if(toWait){
			submitInfo.waitSemaphoreInfoCount = 1,
			submitInfo.pWaitSemaphoreInfos = &semaphoreSubmitInfo_Wait;
		}

		if(toSignal){
			submitInfo.signalSemaphoreInfoCount = 1,
			submitInfo.pSignalSemaphoreInfos = &semaphoreSubmitInfo_Signal;
		}

		vkQueueSubmit2(queue, 1, &submitInfo, fence);
	}

	export
	void submitCommand(VkQueue queue, const std::span<VkCommandBuffer> commandBuffer, VkFence fence = nullptr,
	                   VkSemaphore toWait = nullptr, VkPipelineStageFlags2 waitStage = VK_PIPELINE_STAGE_2_NONE,
	                   VkSemaphore toSignal = nullptr, VkPipelineStageFlags2 signalStage = VK_PIPELINE_STAGE_2_NONE
	){
		if(commandBuffer.size() == 0)return;
		VkSemaphoreSubmitInfo semaphoreSubmitInfo_Wait{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.semaphore = toWait,
				.value = 0,
				.stageMask = waitStage,
				.deviceIndex = 0
			};

		VkSemaphoreSubmitInfo semaphoreSubmitInfo_Signal{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.semaphore = toSignal,
				.value = 0,
				.stageMask = signalStage,
				.deviceIndex = 0
			};

		std::vector<VkCommandBufferSubmitInfo> cmds(commandBuffer.size());
		for(const auto& [i, cmd] : cmds | std::views::enumerate){
			cmd = VkCommandBufferSubmitInfo{
					.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
					.commandBuffer = commandBuffer[i],
					.deviceMask = 0
				};
		}


		VkSubmitInfo2 submitInfo{
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
				.commandBufferInfoCount = static_cast<std::uint32_t>(cmds.size()),
				.pCommandBufferInfos = cmds.data(),
			};

		if(toWait){
			submitInfo.waitSemaphoreInfoCount = 1,
				submitInfo.pWaitSemaphoreInfos = &semaphoreSubmitInfo_Wait;
		}

		if(toSignal){
			submitInfo.signalSemaphoreInfoCount = 1,
				submitInfo.pSignalSemaphoreInfos = &semaphoreSubmitInfo_Signal;
		}

		vkQueueSubmit2(queue, 1, &submitInfo, fence);
	}

	export
	void submitCommand(VkQueue queue, VkCommandBuffer commandBuffer, VkFence fence = nullptr,
		VkSemaphore toWait = nullptr, VkPipelineStageFlags2 waitStage = VK_PIPELINE_STAGE_2_NONE,
		VkSemaphore toSignal = nullptr, VkPipelineStageFlags2 signalStage = VK_PIPELINE_STAGE_2_NONE
		){

		submitCommand(queue, std::array{commandBuffer}, fence, toWait, waitStage, toSignal, signalStage);
	}

	export
	template <std::size_t W, std::size_t S>
	void submitCommand(VkQueue queue, VkCommandBuffer commandBuffer, VkFence fence = nullptr,
		const std::array<VkSemaphore, W> toWait = {}, const std::array<VkPipelineStageFlags2, W> waitStage = {},
		const std::array<VkSemaphore, S> toSignal = {}, const std::array<VkPipelineStageFlags2, S> signalStage = {}
		){


		VkCommandBufferSubmitInfo cmdSubmitInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = commandBuffer,
			.deviceMask = 0
		};

		VkSubmitInfo2 submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.commandBufferInfoCount = 1,
			.pCommandBufferInfos = &cmdSubmitInfo,
		};


		std::array<VkSemaphoreSubmitInfo, W> waits{};
		std::array<VkSemaphoreSubmitInfo, S> signals{};

		if constexpr (W){
			for (const auto & [i, semaphore] : toWait | std::views::enumerate){
				waits[i] = VkSemaphoreSubmitInfo{
					.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
					.semaphore = semaphore,
					.value = 0,
					.stageMask = waitStage[i],
					.deviceIndex = 0
				};
			}

			submitInfo.waitSemaphoreInfoCount = W;
			submitInfo.pWaitSemaphoreInfos = waits.data();
		}

		if constexpr (S){
			for (const auto & [i, semaphore] : toSignal | std::views::enumerate){
				signals[i] = VkSemaphoreSubmitInfo{
					.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
					.semaphore = semaphore,
					.value = 0,
					.stageMask = signalStage[i],
					.deviceIndex = 0
				};
			}

			submitInfo.signalSemaphoreInfoCount = S;
			submitInfo.pSignalSemaphoreInfos = signals.data();
		}

		vkQueueSubmit2(queue, 1, &submitInfo, fence);
	}

	export
	template <std::ranges::range Range, std::ranges::range ValidRange, typename Proj = std::identity>
		requires requires(
			std::unordered_set<std::string>& set,
			std::indirect_result_t<Proj, std::ranges::const_iterator_t<ValidRange>> value
		){
			requires std::constructible_from<std::string, std::ranges::range_value_t<Range>>;
			// set.erase(value);
		}
	[[nodiscard]] bool check_support(const Range& required, const ValidRange& valids, Proj proj = {}){
		std::unordered_set<std::string> requiredExtensions(std::ranges::begin(required), std::ranges::end(required));

		for(const auto& extension : valids){
			requiredExtensions.erase(std::invoke(proj, extension));
		}

		return requiredExtensions.empty();
	}
}
