module;

#include <vulkan/vulkan.h>
#include "vk_loader.hpp"

export module mo_yanxi.vk.ext;
import std;

namespace mo_yanxi::vk{
	export std::add_pointer_t<decltype(vkGetPhysicalDeviceProperties2KHR)> getPhysicalDeviceProperties2KHR = nullptr;
	export std::add_pointer_t<decltype(vkGetDescriptorSetLayoutSizeEXT)> getDescriptorSetLayoutSizeEXT = nullptr;
	export std::add_pointer_t<decltype(vkGetDescriptorSetLayoutBindingOffsetEXT)> getDescriptorSetLayoutBindingOffsetEXT
		= nullptr;
	export std::add_pointer_t<decltype(vkGetDescriptorEXT)> getDescriptorEXT = nullptr;
	export std::add_pointer_t<decltype(vkCmdDrawMeshTasksEXT)> drawMeshTasksEXT = nullptr;

	std::add_pointer_t<decltype(vkCmdBindDescriptorBuffersEXT)> PFN_cmdBindDescriptorBuffersEXT = nullptr;
	std::add_pointer_t<decltype(vkCmdSetDescriptorBufferOffsetsEXT)> PFN_cmdSetDescriptorBufferOffsetsEXT = nullptr;

	namespace cmd{
		constexpr std::uint32_t MaxStackBufferSize = 16;
		constexpr std::uint32_t IndicesDesignator[MaxStackBufferSize]{
			0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
		};
		constexpr VkDeviceSize OffsetDesignator[MaxStackBufferSize]{};

		export void bindDescriptorBuffersEXT(
			VkCommandBuffer commandBuffer,
			std::uint32_t bufferCount,
			const VkDescriptorBufferBindingInfoEXT* pBindingInfos){
			PFN_cmdBindDescriptorBuffersEXT(commandBuffer, bufferCount, pBindingInfos);
		}

		export void bindDescriptorBuffersEXT(
			VkCommandBuffer commandBuffer,
			const std::span<const VkDescriptorBufferBindingInfoEXT> bindingInfos){
			PFN_cmdBindDescriptorBuffersEXT(commandBuffer, static_cast<std::uint32_t>(bindingInfos.size()),
			                                bindingInfos.data());
		}

		export void setDescriptorBufferOffsetsEXT(
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

		export void setDescriptorBufferOffsetsEXT(
			const VkCommandBuffer commandBuffer,
			const VkPipelineBindPoint pipelineBindPoint,
			const VkPipelineLayout layout,
			const uint32_t firstSet,
			const std::initializer_list<VkDeviceSize> offsets) noexcept{
			static constexpr std::uint32_t MaxStackBufferSize = 16;

			static constexpr std::uint32_t IndicesDesignator[MaxStackBufferSize]{
					0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
				};

			PFN_cmdSetDescriptorBufferOffsetsEXT(commandBuffer, pipelineBindPoint, layout, firstSet, offsets.size(),
			                                     IndicesDesignator, offsets.begin());
		}

		export void bindThenSetDescriptorBuffers(
			const VkCommandBuffer commandBuffer,
			const VkPipelineBindPoint pipelineBindPoint,
			const VkPipelineLayout layout,
			const std::uint32_t firstSet,
			const VkDescriptorBufferBindingInfoEXT* pBindingInfos,
			std::uint32_t bufferCount
		){
			bindDescriptorBuffersEXT(commandBuffer, bufferCount, pBindingInfos);

			if(bufferCount <= MaxStackBufferSize){
				setDescriptorBufferOffsetsEXT(commandBuffer, pipelineBindPoint, layout, firstSet, bufferCount,
				                              IndicesDesignator, OffsetDesignator);
			} else{
				throw std::overflow_error("Too many descriptor buffers in the command buffer.");
			}
		}

		export void bindThenSetDescriptorBuffers(
			const VkCommandBuffer commandBuffer,
			const VkPipelineBindPoint pipelineBindPoint,
			const VkPipelineLayout layout,
			const uint32_t firstSet,
			const std::span<const VkDescriptorBufferBindingInfoEXT> infos
		){
			bindThenSetDescriptorBuffers(commandBuffer, pipelineBindPoint, layout, firstSet, infos.data(),
			                             infos.size());
		}

		export void bindThenSetDescriptorBuffers(
			const VkCommandBuffer commandBuffer,
			const VkPipelineBindPoint pipelineBindPoint,
			const VkPipelineLayout layout,
			const uint32_t firstSet,
			const std::initializer_list<const VkDescriptorBufferBindingInfoEXT> infos
		){
			bindThenSetDescriptorBuffers(commandBuffer, pipelineBindPoint, layout, firstSet, infos.begin(),
			                             infos.size());
		}

		export
		template <
			std::ranges::contiguous_range InfoRng = std::initializer_list<VkDescriptorBufferBindingInfoEXT>,
			std::ranges::contiguous_range Offset = std::initializer_list<VkDeviceSize>>
		void bind_descriptors(
			VkCommandBuffer commandBuffer,
			const VkPipelineBindPoint pipelineBindPoint,
			const VkPipelineLayout layout,
			const uint32_t firstSet,

			const InfoRng& infos,
			const Offset& offsets
		){
			assert(std::ranges::size(infos) == std::ranges::size(offsets));
			cmd::bindDescriptorBuffersEXT(commandBuffer, std::ranges::size(infos), std::ranges::data(infos));
			cmd::setDescriptorBufferOffsetsEXT(
				commandBuffer,
				pipelineBindPoint,
				layout, firstSet, std::ranges::size(offsets), IndicesDesignator, std::ranges::data(offsets));
		}

		export
		template <
			std::ranges::contiguous_range InfoRng = std::initializer_list<VkDescriptorBufferBindingInfoEXT>,
			std::ranges::contiguous_range Offset = std::initializer_list<VkDeviceSize>>
		void bind_descriptors(
			VkCommandBuffer commandBuffer,
			const InfoRng& infos
		){
			cmd::bindDescriptorBuffersEXT(commandBuffer, std::ranges::size(infos), std::ranges::data(infos));

		}
		export
		template <
			std::ranges::contiguous_range Targets = std::initializer_list<std::uint32_t>,
			std::ranges::contiguous_range Offset = std::initializer_list<VkDeviceSize>
		>
		void set_descriptor_offsets(
			VkCommandBuffer commandBuffer,
			const VkPipelineBindPoint pipelineBindPoint,
			const VkPipelineLayout layout,
			const uint32_t firstSet,

			const Targets& targets,
			const Offset& offsets
		){
			assert(std::ranges::size(offsets) >= std::ranges::size(targets));
			cmd::setDescriptorBufferOffsetsEXT(
				commandBuffer,
				pipelineBindPoint,
				layout, firstSet, std::ranges::size(targets), std::ranges::data(targets), std::ranges::data(offsets));
		}

	}

	export void load_ext(VkInstance instance){
		getPhysicalDeviceProperties2KHR = LoadFuncPtr(instance, vkGetPhysicalDeviceProperties2KHR);
		getDescriptorSetLayoutSizeEXT = LoadFuncPtr(instance, vkGetDescriptorSetLayoutSizeEXT);
		getDescriptorSetLayoutBindingOffsetEXT = LoadFuncPtr(instance, vkGetDescriptorSetLayoutBindingOffsetEXT);
		getDescriptorEXT = LoadFuncPtr(instance, vkGetDescriptorEXT);
		drawMeshTasksEXT = LoadFuncPtr(instance, vkCmdDrawMeshTasksEXT);
		PFN_cmdBindDescriptorBuffersEXT = LoadFuncPtr(instance, vkCmdBindDescriptorBuffersEXT);
		PFN_cmdSetDescriptorBufferOffsetsEXT = LoadFuncPtr(instance, vkCmdSetDescriptorBufferOffsetsEXT);
	}
}

namespace mo_yanxi::vk::cmd{
}
