module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk.descriptor_buffer;

import std;
export import mo_yanxi.vk.resources;
import mo_yanxi.handle_wrapper;
import mo_yanxi.vk.vma;
import mo_yanxi.vk.ext;

namespace mo_yanxi::vk{
	export struct descriptor_mapper;

	export
	struct descriptor_buffer : buffer{
	private:
		friend descriptor_mapper;
		std::vector<VkDeviceSize> offsets{};

		std::size_t uniformBufferDescriptorSize{};
		std::size_t combinedImageSamplerDescriptorSize{};
		std::size_t storageImageDescriptorSize{};
		std::size_t inputAttachmentDescriptorSize{};

	public:
		[[nodiscard]] descriptor_buffer() = default;

		[[nodiscard]] descriptor_buffer(
			vk::allocator& allocator,
			VkDescriptorSetLayout layout,
			std::uint32_t bindings
		){

			const auto device = allocator.get_device();
			const auto physical_device = allocator.get_physical_device();


			VkDeviceSize dbo_size;
			ext::getDescriptorSetLayoutSizeEXT(device, layout, &dbo_size);


			this->buffer::operator=(buffer{allocator, {
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = dbo_size,
				.usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			}, {
				.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			}});


			{
				VkPhysicalDeviceDescriptorBufferPropertiesEXT descriptor_buffer_properties{
					.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT,
					.pNext = nullptr,
				};

				VkPhysicalDeviceProperties2KHR device_properties{
					.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR,
					.pNext = &descriptor_buffer_properties,
				};

				ext::getPhysicalDeviceProperties2KHR(physical_device, &device_properties);
				uniformBufferDescriptorSize = descriptor_buffer_properties.uniformBufferDescriptorSize;
				combinedImageSamplerDescriptorSize = descriptor_buffer_properties.combinedImageSamplerDescriptorSize;
				storageImageDescriptorSize = descriptor_buffer_properties.storageImageDescriptorSize;
				inputAttachmentDescriptorSize = descriptor_buffer_properties.inputAttachmentDescriptorSize;
			}


			offsets.resize(bindings);
			// Get descriptor bindings offsets as descriptors are placed inside set layout by those offsets.
			for(auto&& [i, offset] : offsets | std::views::enumerate){
				ext::getDescriptorSetLayoutBindingOffsetEXT(device, layout, static_cast<std::uint32_t>(i), offsets.data() + i);
			}
		}

		[[nodiscard]] std::size_t get_uniform_buffer_descriptor_size() const noexcept{
			return uniformBufferDescriptorSize;
		}

		[[nodiscard]] std::size_t get_combined_image_sampler_descriptor_size() const noexcept{
			return combinedImageSamplerDescriptorSize;
		}

		[[nodiscard]] std::size_t get_storage_image_descriptor_size() const noexcept{
			return storageImageDescriptorSize;
		}

		[[nodiscard]] std::size_t get_input_attachment_descriptor_size() const noexcept{
			return inputAttachmentDescriptorSize;
		}

		[[nodiscard]] VkDescriptorBufferBindingInfoEXT get_bind_info(const VkBufferUsageFlags usage) const{
			return {
					.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
					.pNext = nullptr,
					.address = get_address(),
					.usage = usage
				};
		}

		void bind_to(const VkCommandBuffer commandBuffer, const VkBufferUsageFlags usage) const{
			const auto info = get_bind_info(usage);
			ext::cmdBindDescriptorBuffersEXT(commandBuffer, 1, &info);
		}

		void bind_to(
			VkCommandBuffer commandBuffer,
			const VkBufferUsageFlags usage,
			VkPipelineLayout layout,
			const std::uint32_t setIndex,
			const VkPipelineBindPoint bindingPoint,
			const VkDeviceSize offset = 0) const{
			static constexpr std::uint32_t ZERO = 0;
			bind_to(commandBuffer, usage);
			ext::cmdSetDescriptorBufferOffsetsEXT(
				commandBuffer,
				bindingPoint,
				layout,
				setIndex, 1, &ZERO, &offset);
		}

		[[nodiscard]] VkDescriptorBufferBindingInfoEXT get_info(VkBufferUsageFlags usage) const noexcept{
			return {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
				.pNext = nullptr,
				.address = get_address(),
				.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | usage
			};
		}
	private:
		std::pair<VkDescriptorGetInfoEXT, VkDeviceSize> get_image_info(
			VkDescriptorImageInfo&& dangling, const VkDescriptorType descriptorType) const = delete;

		[[nodiscard]] std::pair<VkDescriptorGetInfoEXT, VkDeviceSize> get_image_info(
			const VkDescriptorImageInfo& imageInfo_non_rv, const VkDescriptorType descriptorType) const{
			std::pair<VkDescriptorGetInfoEXT, VkDeviceSize> rst{
					VkDescriptorGetInfoEXT{
						.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
						.pNext = nullptr,
						.type = descriptorType,
						.data = {}
					},
					{}
				};

			switch(descriptorType){
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER :{
				rst.first.data.pCombinedImageSampler = &imageInfo_non_rv;
				rst.second = combinedImageSamplerDescriptorSize;
				break;
			}

			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE :{
				rst.first.data.pStorageImage = &imageInfo_non_rv;
				rst.second = storageImageDescriptorSize;
				break;
			}

			case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT :{
				rst.first.data.pInputAttachmentImage = &imageInfo_non_rv;
				rst.second = inputAttachmentDescriptorSize;
				break;
			}

			default : throw std::runtime_error("Invalid descriptor type!");
			}

			return rst;
		}
	};

	struct descriptor_mapper : buffer_mapper<descriptor_buffer>{
		// using buffer_mapper::buffer_mapper;

		const descriptor_mapper& set_uniform_buffer(
			const std::uint32_t binding,
			const buffer& ubo,
			const VkDeviceSize offset = 0
		) const{
			return this->set_uniform_buffer(binding, ubo.get_address() + offset, ubo.get_size());
		}

		const descriptor_mapper& set_uniform_buffer(
			const std::uint32_t binding,
			const VkDeviceAddress address,
			const VkDeviceSize size
		) const{
			const VkDescriptorAddressInfoEXT addr_info{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
				.pNext = nullptr,
				.address = address,
				.range = size,
				.format = VK_FORMAT_UNDEFINED
			};

			const VkDescriptorGetInfoEXT info{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
				.pNext = nullptr,
				.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.data = VkDescriptorDataEXT{
					.pUniformBuffer = &addr_info
				}
			};

			ext::getDescriptorEXT(
				buffer_obj->get_device(),
				&info,
				buffer_obj->get_uniform_buffer_descriptor_size(),
				mapped + buffer_obj->offsets[binding]
			);

			return *this;
		}

		const descriptor_mapper& set_uniform_buffer_segments(
			std::uint32_t initialBinding,
			const buffer& ubo,
			std::initializer_list<VkDeviceSize> segment_sizes) const{

			VkDeviceSize currentOffset{};

			for (const auto& [idx, sz] : segment_sizes | std::views::enumerate){
				(void)this->set_uniform_buffer(initialBinding + static_cast<std::uint32_t>(idx), ubo.get_address() + currentOffset, sz);

				currentOffset += sz;
			}

			return *this;
		}



		void set_image(
			const std::uint32_t binding,
			const VkDescriptorImageInfo& imageInfo,
			const VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) const{
			auto [info, size] = buffer_obj->get_image_info(imageInfo, descriptorType);

			ext::getDescriptorEXT(
				buffer_obj->get_device(),
				&info,
				size,
				mapped + buffer_obj->offsets[binding]
			);
		}

		template <std::ranges::input_range Rng>
			requires (std::convertible_to<std::ranges::range_const_reference_t<Rng>, const VkDescriptorImageInfo&>)
		void set_image(
			const std::uint32_t binding,
			const Rng& imageInfos,
			const VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) const{
			std::size_t currentOffset = 0;

			for(const VkDescriptorImageInfo& imageInfo : imageInfos){
				auto [info, size] = buffer_obj->get_image_info(imageInfo, descriptorType);

				ext::getDescriptorEXT(
					buffer_obj->get_device(),
					&info,
					size,
					mapped + buffer_obj->offsets[binding] + currentOffset
				);

				currentOffset += size;
			}
		}

		template <std::ranges::input_range Rng>
			requires (std::convertible_to<std::ranges::range_const_reference_t<Rng>, VkImageView>)
		void set_image(
			const std::uint32_t binding,
			const VkImageLayout layout,
			VkSampler sampler,
			const Rng& imageViews) const{
			std::size_t currentOffset = 0;

			for(VkImageView imageInfo : imageViews){
				VkDescriptorImageInfo image_info{
					.sampler = sampler,
					.imageView = imageInfo,
					.imageLayout = layout
				};

				auto [info, size] = buffer_obj->get_image_info(
					image_info, sampler ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

				ext::getDescriptorEXT(
					buffer_obj->get_device(),
					&info,
					size,
					mapped + buffer_obj->offsets[binding] + currentOffset
				);

				currentOffset += size;
			}
			//
			// if(binding + 1 < buffer_obj->offsets.size()){
			//
			// }
		}

		void set_image(
			const std::uint32_t binding,
			const VkImageView imageView,
			const VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			const VkSampler sampler = nullptr,
			const VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
		) const{
			const VkDescriptorImageInfo image_descriptor{
					.sampler = sampler,
					.imageView = imageView,
					.imageLayout = imageLayout
				};

			set_image(binding, image_descriptor, descriptorType);
		}

		// template <std::invocable<descriptor_buffer&> Writer>
		// void load(Writer&& writer){
		// 	map();
		// 	writer(*this);
		// 	unmap();
		// }

		// template <typename... Args>
		// 	requires (std::same_as<UniformBuffer, std::remove_cvref_t<Args>> && ...)
		// void loadUniformBuffer(std::uint32_t initialBinding, const Args&... Ubos){
		// 	map();
		//
		// 	[&, this]<std::size_t... Indices>(std::index_sequence<Indices...>){
		// 		((descriptor_buffer::mapUniformBuffer(initialBinding++, Ubos, 0)), ...);
		// 	}(std::index_sequence_for<Args...>{});
		//
		// 	unmap();
		// }
		//
	};
}
