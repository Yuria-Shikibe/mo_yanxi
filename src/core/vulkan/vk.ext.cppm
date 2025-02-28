module;

#include <vulkan/vulkan.h>
#include "vk_loader.hpp"

export module mo_yanxi.vk.ext;
import std;

namespace mo_yanxi::vk::ext{
	export std::add_pointer_t<decltype(vkGetPhysicalDeviceProperties2KHR)> getPhysicalDeviceProperties2KHR = nullptr;
	export std::add_pointer_t<decltype(vkGetDescriptorSetLayoutSizeEXT)> getDescriptorSetLayoutSizeEXT = nullptr;
	export std::add_pointer_t<decltype(vkGetDescriptorSetLayoutBindingOffsetEXT)> getDescriptorSetLayoutBindingOffsetEXT = nullptr;
	export std::add_pointer_t<decltype(vkGetDescriptorEXT)> getDescriptorEXT = nullptr;
	export std::add_pointer_t<decltype(vkCmdDrawMeshTasksEXT)> drawMeshTasksEXT = nullptr;

	std::add_pointer_t<decltype(vkCmdBindDescriptorBuffersEXT)> PFN_cmdBindDescriptorBuffersEXT = nullptr;
	std::add_pointer_t<decltype(vkCmdSetDescriptorBufferOffsetsEXT)> PFN_cmdSetDescriptorBufferOffsetsEXT = nullptr;

	export void cmdBindDescriptorBuffersEXT(
		VkCommandBuffer commandBuffer,
		std::uint32_t bufferCount,
		const VkDescriptorBufferBindingInfoEXT* pBindingInfos){
		PFN_cmdBindDescriptorBuffersEXT(commandBuffer, bufferCount, pBindingInfos);
	}

	export void cmdBindDescriptorBuffersEXT(
		VkCommandBuffer commandBuffer,
		const std::span<const VkDescriptorBufferBindingInfoEXT> bindingInfos){
		PFN_cmdBindDescriptorBuffersEXT(commandBuffer, static_cast<std::uint32_t>(bindingInfos.size()), bindingInfos.data());
	}

	export void cmdSetDescriptorBufferOffsetsEXT(
		const VkCommandBuffer commandBuffer,
		const VkPipelineBindPoint pipelineBindPoint,
		const VkPipelineLayout layout,
		const uint32_t firstSet,
		const uint32_t setCount,
		const uint32_t* pBufferIndices,
		const VkDeviceSize* pOffsets) noexcept{
		PFN_cmdSetDescriptorBufferOffsetsEXT(commandBuffer, pipelineBindPoint, layout, firstSet, setCount,
		                                     pBufferIndices, pOffsets);
	}

	export void cmdBindThenSetDescriptorBuffers(
		const VkCommandBuffer commandBuffer,
		const VkPipelineBindPoint pipelineBindPoint,
		const VkPipelineLayout layout,
		const uint32_t firstSet,
		const VkDescriptorBufferBindingInfoEXT* pBindingInfos,
		std::uint32_t bufferCount
	){
		cmdBindDescriptorBuffersEXT(commandBuffer, bufferCount, pBindingInfos);
		static constexpr std::uint32_t MaxStackBufferSize = 16;
		static constexpr std::uint32_t IndicesDesignator[MaxStackBufferSize]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
		static constexpr VkDeviceSize OffsetDesignator[MaxStackBufferSize]{};
		if(bufferCount <= MaxStackBufferSize){
			cmdSetDescriptorBufferOffsetsEXT(commandBuffer, pipelineBindPoint, layout, firstSet, bufferCount, IndicesDesignator, OffsetDesignator);
		}else{
			throw std::overflow_error("Too many descriptor buffers in the command buffer.");
		}
	}

	export void cmdBindThenSetDescriptorBuffers(
		const VkCommandBuffer commandBuffer,
		const VkPipelineBindPoint pipelineBindPoint,
		const VkPipelineLayout layout,
		const uint32_t firstSet,
		const std::span<const VkDescriptorBufferBindingInfoEXT> infos
	){
		cmdBindThenSetDescriptorBuffers(commandBuffer, pipelineBindPoint, layout, firstSet, infos.data(), infos.size());
	}

	export void load(VkInstance instance){
		getPhysicalDeviceProperties2KHR = LoadFuncPtr(instance, vkGetPhysicalDeviceProperties2KHR);
		getDescriptorSetLayoutSizeEXT = LoadFuncPtr(instance, vkGetDescriptorSetLayoutSizeEXT);
		getDescriptorSetLayoutBindingOffsetEXT = LoadFuncPtr(instance, vkGetDescriptorSetLayoutBindingOffsetEXT);
		getDescriptorEXT = LoadFuncPtr(instance, vkGetDescriptorEXT);
		drawMeshTasksEXT = LoadFuncPtr(instance, vkCmdDrawMeshTasksEXT);
		PFN_cmdBindDescriptorBuffersEXT = LoadFuncPtr(instance, vkCmdBindDescriptorBuffersEXT);
		PFN_cmdSetDescriptorBufferOffsetsEXT = LoadFuncPtr(instance, vkCmdSetDescriptorBufferOffsetsEXT);
	}
}

