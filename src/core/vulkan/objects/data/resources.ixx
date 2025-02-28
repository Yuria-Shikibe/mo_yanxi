module;

#include <cassert>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

export module mo_yanxi.vk.resources;

import std;
export import mo_yanxi.vk.util.cmd.resources;
import mo_yanxi.vk.vma;
import mo_yanxi.vk.concepts;
import mo_yanxi.handle_wrapper;

namespace mo_yanxi::vk{

	export struct buffer : resource_base<VkBuffer>{
	protected:
		VkDeviceSize size{};

		void create(const VkBufferCreateInfo& create_info, const VmaAllocationCreateInfo& alloc_create_info){
			pass_alloc(allocator->allocate(create_info, alloc_create_info));
			size = create_info.size;
		}

	public:
		[[nodiscard]] buffer() = default;

		[[nodiscard]] explicit buffer(
			vk::allocator& allocator,
			const VkBufferCreateInfo& create_info,
			const VmaAllocationCreateInfo& alloc_create_info
		)
			: resource_base(allocator){
			create(create_info, alloc_create_info);
		}

		[[nodiscard]] VkDeviceSize get_size() const noexcept{
			return size;
		}

		template <contigious_range_of<VkBufferCopy> Rng = std::initializer_list<VkBufferCopy>>
		void copy_from(VkCommandBuffer command_buffer, VkBuffer src, Rng&& rng) const noexcept{
			cmd::copy_buffer(command_buffer, src, get(), std::forward<Rng>(rng));
		}

		template <contigious_range_of<VkBufferCopy> Rng = std::initializer_list<VkBufferCopy>>
		void copy_to(VkCommandBuffer command_buffer, VkBuffer dst, Rng&& rng) const noexcept{
			cmd::copy_buffer(command_buffer, get(), dst, std::forward<Rng>(rng));
		}

		[[nodiscard]] VkDeviceAddress get_address() const noexcept{
			const VkBufferDeviceAddressInfo bufferDeviceAddressInfo{
				.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
				.pNext = nullptr,
				.buffer = handle
			};

			return vkGetBufferDeviceAddress(get_device(), &bufferDeviceAddressInfo);
		}

	};

	export
	template <std::derived_from<buffer> Bty>
	struct buffer_mapper{
		using buffer_type = Bty;

	protected:
		exclusive_handle_member<buffer_type*> buffer_obj;
		exclusive_handle_member<std::byte*> mapped;

	public:
		[[nodiscard]] explicit(false) buffer_mapper(buffer_type& buffer)
			: buffer_obj(&buffer){
			mapped = static_cast<std::byte*>(buffer.map());
		}

		~buffer_mapper(){
			if(mapped){
				buffer_obj->unmap();
			}
		}

		template <typename T>
			requires (std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>)
		const buffer_mapper& load(const T& data, const std::ptrdiff_t dst_byte_offset = 0) const noexcept{
			std::memcpy(mapped + dst_byte_offset, &data, sizeof(T));
			return *this;
		}

		template <typename T>
			requires (std::is_pointer_v<T>)
		const buffer_mapper& load(const T* data, const std::size_t count, const std::ptrdiff_t dst_byte_offset = 0) const noexcept{
			std::memcpy(mapped + dst_byte_offset, data, sizeof(T) * count);
			return *this;
		}

		template <typename T>
			requires (std::is_trivially_copyable_v<T>)
		const buffer_mapper& load_range(const std::span<const T> span, const std::ptrdiff_t dst_byte_offset = 0) const noexcept{
			std::memcpy(mapped + dst_byte_offset, span.data(), span.size_bytes());
			return *this;
		}

		template <std::ranges::contiguous_range T>
			requires (std::is_trivially_copyable_v<std::ranges::range_value_t<T>> && std::ranges::sized_range<T>)
		const buffer_mapper& load_range(const T& range, const std::ptrdiff_t dst_byte_offset = 0) const noexcept{
			std::memcpy(mapped + dst_byte_offset, std::ranges::data(range), std::ranges::size(range) * sizeof(std::ranges::range_value_t<T>));
			return *this;
		}

		buffer_mapper(const buffer_mapper& other) = delete;
		buffer_mapper(buffer_mapper&& other) noexcept = default;
		buffer_mapper& operator=(const buffer_mapper& other) = delete;
		buffer_mapper& operator=(buffer_mapper&& other) noexcept = default;
	};

	// export struct buffer :
	export struct image : resource_base<VkImage>{
		static constexpr VkImageSubresourceRange default_image_subrange{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		};

	protected:
		VkExtent3D extent{};

		void create(const VkImageCreateInfo& create_info, const VmaAllocationCreateInfo& alloc_create_info){
			pass_alloc(allocator->allocate(create_info, alloc_create_info));
			extent = create_info.extent;
		}

	public:
		[[nodiscard]] image() noexcept = default;

		[[nodiscard]] image(
			vk::allocator& allocator,
			const VkImageCreateInfo& create_info,
			const VmaAllocationCreateInfo& alloc_create_info
		)
			: resource_base(allocator){
			create(create_info, alloc_create_info);
		}

		[[nodiscard]] image(
			vk::allocator& allocator,
			const VkExtent3D extent,
			const VkImageUsageFlags usage,
			const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
			const std::uint32_t mipLevels = 1,
			const std::uint32_t arrayLayers = 1,
			const VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
			const VkImageType type = VK_IMAGE_TYPE_MAX_ENUM /*Auto Deduction*/
		) : image{
				allocator,
				{
					.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.imageType = type != VK_IMAGE_TYPE_MAX_ENUM
						             ? type
						             : extent.depth > 1
						             ? VK_IMAGE_TYPE_3D
						             : extent.height > 1
						             ? VK_IMAGE_TYPE_2D
						             : VK_IMAGE_TYPE_1D,
					.format = format,
					.extent = extent,
					.mipLevels = mipLevels,
					.arrayLayers = arrayLayers,
					.samples = samples,
					.tiling = VK_IMAGE_TILING_OPTIMAL,
					.usage = usage,
					.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
				},
				{
					.usage = VMA_MEMORY_USAGE_GPU_ONLY,
				}
			}{
		}

		[[nodiscard]] VkExtent3D get_extent() const noexcept{
			return extent;
		}

		[[nodiscard]] VkExtent2D get_extent2() const noexcept{
			return {extent.width, extent.height};
		}

		[[nodiscard]] VkImageType type() const noexcept{
			if(!handle || !extent.width || !extent.height || !extent.depth){
				return VK_IMAGE_TYPE_MAX_ENUM;
			}

			if(extent.depth == 1){
				if(extent.height == 1) return VK_IMAGE_TYPE_2D;
				return VK_IMAGE_TYPE_2D;
			}

			return VK_IMAGE_TYPE_3D;
		}

		void layout_on_init(
			VkCommandBuffer command_buffer,
			VkImageLayout layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			const VkImageSubresourceRange& range = default_image_subrange,
			const std::uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			const std::uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED) const noexcept{
			cmd::memory_barrier(
				command_buffer, handle,
				VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
				VK_ACCESS_2_NONE,
				VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				VK_ACCESS_2_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				layout,
				range,
				srcQueueFamilyIndex,
				dstQueueFamilyIndex);
		}
	};

	export struct image_view : exclusive_handle<VkImageView>{
	private:
		exclusive_handle_member<VkDevice> device{};

	public:
		[[nodiscard]] image_view() = default;

		[[nodiscard]] image_view(VkDevice device, const VkImageViewCreateInfo& create_info) : device(device){
			if(const auto rst = vkCreateImageView(device, &create_info, nullptr, as_data())){
				throw vk_error(rst, "Failed to create image view");
			}
		}

		~image_view(){
			if(get_device())vkDestroyImageView(get_device(), handle, nullptr);
		}

		image_view(const image_view& other) = delete;
		image_view(image_view&& other) noexcept = default;
		image_view& operator=(const image_view& other) = delete;
		image_view& operator=(image_view&& other) noexcept = default;

		[[nodiscard]] VkDevice get_device() const noexcept{
			return device;
		}
	};
}

namespace mo_yanxi::vk::templates{
	export
	[[nodiscard]] buffer create_staging_buffer(allocator& allocator, VkDeviceSize size){
		return buffer{
			allocator,
			{
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = size,
				.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
			}, {
				.usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
			}
		};
	}

	export
	[[nodiscard]] buffer create_index_buffer(allocator& allocator, const std::size_t count){
		return buffer{
			allocator,
			{
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = count * sizeof(std::uint32_t),
				.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
			}, {
				.usage = VMA_MEMORY_USAGE_GPU_ONLY,
			}
		};
	}

	export
	[[nodiscard]] buffer create_vertex_buffer(allocator& allocator, const std::size_t size_in_bytes){
		return buffer{
			allocator,
			{
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = size_in_bytes,
				.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
			}, {
				.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				.preferredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT
			}
		};
	}

	export
	[[nodiscard]] buffer create_storage_buffer(allocator& allocator, const std::size_t size_in_bytes){
		return buffer{
			allocator,
			{
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = size_in_bytes,
				.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
			}, {
				.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				.preferredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT
			}
		};
	}

	export
	[[nodiscard]] buffer create_uniform_buffer(allocator& allocator, const std::size_t size_in_bytes){
		return buffer{
			allocator,
			{
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = size_in_bytes,
				.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
			}, {
				.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				.preferredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT
			}
		};
	}

}
