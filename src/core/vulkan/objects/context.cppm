module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk.context;

export import mo_yanxi.vk.instance;
export import mo_yanxi.vk.logical_deivce;
export import mo_yanxi.vk.physical_device;

import mo_yanxi.vk.sync;

import mo_yanxi.vk.validation;
import mo_yanxi.vk.exception;
import mo_yanxi.vk.util;
import mo_yanxi.vk.util.cmd.resources;
import mo_yanxi.vk.util.cmd.render;
import mo_yanxi.vk.swap_chain_info;
import mo_yanxi.vk.concepts;
import mo_yanxi.vk.vma;

import mo_yanxi.vk.command_pool;
import mo_yanxi.vk.command_buffer;

import mo_yanxi.core.window;

import mo_yanxi.meta_programming;
import mo_yanxi.stack_trace;
import mo_yanxi.circular_array;
import mo_yanxi.event;
import std;




namespace mo_yanxi::vk{
	struct SwapChainFrameData{
		VkImage image{};
		command_buffer post_command{};
	};

	struct InFlightData{
		vk::fence fence{};
		vk::semaphore fetch_semaphore{};
		vk::semaphore flush_semaphore{};
	};

	export
	struct swap_chain_staging_image_data{
		VkImage image{};
		VkExtent2D extent{};

		bool clear{};
		std::uint32_t owner_queue_family{};

		VkPipelineStageFlags2 src_stage{};
		VkAccessFlags2 src_access{};
		VkPipelineStageFlags2 dst_stage{};
		VkAccessFlags2 dst_access{};

		VkImageLayout src_layout{};
		VkImageLayout dst_layout{VK_IMAGE_LAYOUT_UNDEFINED};

		VkSemaphore to_wait{};
		VkSemaphore to_singal{};

	};


	export
	class context{
	private:
		instance instance{};

		window_instance window_{};

		physical_device physical_device{};
		logical_device device{};

		allocator allocator_{};
		validation_entry validationEntry{};


		command_pool main_graphic_command_pool{};
		command_pool main_graphic_command_pool_transient{};
		command_pool main_compute_command_pool{};
		command_pool main_compute_command_pool_transient{};

		//Swap Chains
		exclusive_handle_member<VkSwapchainKHR> last_swap_chain{nullptr};
		exclusive_handle_member<VkSwapchainKHR> swap_chain{};
		exclusive_handle_member<VkSurfaceKHR> surface{};
		VkExtent2D swap_chain_extent{};
		VkFormat swapChainImageFormat{};
		std::vector<SwapChainFrameData> swap_chain_frames{};
		circular_array<InFlightData, 3> sync_arr{};
		swap_chain_staging_image_data final_staging_image{};

		events::named_event_manager<std::move_only_function<void() const>, window_instance::resize_event> eventManager{};

	public:

		[[nodiscard]] context() = default;

		[[nodiscard]] explicit(false) context(const VkApplicationInfo& app_info){
			init(app_info);
		}

		void set_staging_image(const swap_chain_staging_image_data& image_data, bool instantly_create_command = true){
			this->final_staging_image = image_data;
			if(instantly_create_command)record_post_command();
		}

		void flush(){
			if(!final_staging_image.image)return;

			auto& current_syncs = ++sync_arr;
			std::uint32_t imageIndex{};
			bool out_of_date{};
			const VkPresentInfoKHR info{
				.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
				.pNext = nullptr,
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = current_syncs.flush_semaphore.as_data(),
				.swapchainCount = 1,
				.pSwapchains = swap_chain.as_data(),
				.pImageIndices = &imageIndex,
				.pResults = nullptr,
			};

			current_syncs.fence.wait_and_reset();

			{
				constexpr auto timeout = std::numeric_limits<std::uint64_t>::max();
				const auto result = vkAcquireNextImageKHR(device, swap_chain, timeout, current_syncs.fetch_semaphore, nullptr, &imageIndex);

				if (result == VK_ERROR_OUT_OF_DATE_KHR) {
					out_of_date = true;
				} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
					throw vk_error(result, "failed to acquire swap chain image!");
				}
			}

			// vkDeviceWaitIdle(device);

			cmd::submit_command/*<2, 2>*/(
				device.primary_graphics_queue(),
				swap_chain_frames[imageIndex].post_command,
				current_syncs.fence,
				current_syncs.fetch_semaphore,
				VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				current_syncs.flush_semaphore,
				VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT
			);

			if(const auto result = vkQueuePresentKHR(device.present_queue(), &info);
				false
				|| out_of_date
				|| result == VK_ERROR_OUT_OF_DATE_KHR
				|| result == VK_SUBOPTIMAL_KHR
				|| window_.check_resized()
			){
				(void)window_.check_resized();
				recreate();
			} else if(result != VK_SUCCESS){
				throw vk_error(result, "Failed to present swap chain image!");
			}
		}

		void submit_graphics(
			VkCommandBuffer commandBuffer,
			VkFence fence = nullptr,
            VkSemaphore toWait = nullptr,
            VkSemaphore toSignal = nullptr,
            VkPipelineStageFlags wait_before = VK_PIPELINE_STAGE_2_NONE,
            VkPipelineStageFlags signal_after = VK_PIPELINE_STAGE_2_NONE
        ) const{
			cmd::submit_command(device.primary_graphics_queue(), commandBuffer, fence, toWait, wait_before, toSignal, signal_after);
		}

		void wait_on_graphic() const{
			waitOnQueue(graphic_queue());
		}

		void wait_on_compute() const{
			waitOnQueue(compute_queue());
		}

		void wait_on_device() const{
			if(const auto rst = vkDeviceWaitIdle(device)){
				throw vk_error{rst, "Failed to wait on device."};
			}
		}

		[[nodiscard]] VkQueue graphic_queue() const noexcept{
			return device.primary_graphics_queue();
		}

		[[nodiscard]] VkQueue compute_queue() const noexcept{
			return device.primary_compute_queue();
		}


		[[nodiscard]] const command_pool& get_graphic_command_pool() const noexcept{
			return main_graphic_command_pool;
		}

		[[nodiscard]] const command_pool& get_compute_command_pool() const noexcept{
			return main_compute_command_pool;
		}

		[[nodiscard]] const command_pool& get_compute_command_pool_transient() const noexcept{
			return main_compute_command_pool_transient;
		}

		[[nodiscard]] const command_pool& get_graphic_command_pool_transient() const noexcept{
			return main_graphic_command_pool_transient;
		}

		[[nodiscard]] transient_command get_transient_graphic_command_buffer() const noexcept{
			return main_graphic_command_pool_transient.get_transient(graphic_queue());
		}

		[[nodiscard]] transient_command get_transient_compute_command_buffer() const noexcept{
			return main_compute_command_pool_transient.get_transient(compute_queue());
		}

		[[nodiscard]] std::uint32_t graphic_family() const noexcept{
			return physical_device.queues.graphic.index;
		}

		[[nodiscard]] std::uint32_t present_family() const noexcept{
			return physical_device.queues.present.index;
		}

		[[nodiscard]] std::uint32_t compute_family() const noexcept{
			return physical_device.queues.compute.index;
		}


		[[nodiscard]] VkInstance get_instance() const noexcept{
			return instance;
		}

		[[nodiscard]] VkPhysicalDevice get_physical_device() const noexcept{
			return physical_device;
		}

		[[nodiscard]] const logical_device& get_device() const noexcept{
			return device;
		}

		[[nodiscard]] const allocator& get_allocator() const noexcept{
			return allocator_;
		}

		[[nodiscard]] allocator& get_allocator() noexcept{
			return allocator_;
		}

		[[nodiscard]] VkExtent2D get_extent() const noexcept{
			return window().get_size();
		}

		[[nodiscard]] VkExtent3D get_extent3() const noexcept{
			return {window().get_size().width, window().get_size().height, static_cast<std::uint32_t>(1)};
		}

		[[nodiscard]] VkRect2D get_screen_area() const noexcept{
			return {{}, get_extent()};
		}

		context(const context& other) = delete;
		context(context&& other) noexcept = default;
		context& operator=(const context& other) = delete;
		context& operator=(context&& other) noexcept = default;

		~context(){
			if(device){
				vkDestroySwapchainKHR(device, swap_chain, nullptr);
				if(last_swap_chain)vkDestroySwapchainKHR(device, last_swap_chain, nullptr);
			}
			if(instance)vkDestroySurfaceKHR(instance, surface, nullptr);

			swap_chain_frames.clear();
		}

		[[nodiscard]] const window_instance& window() const noexcept{
			return window_;
		}

		void register_post_resize(const std::string_view name, std::move_only_function<void(const window_instance::resize_event&) const>&& callback) {
			eventManager.on<window_instance::resize_event>(name, std::move(callback));
		}

	private:
		static void waitOnQueue(VkQueue queue){
			if(const auto rst = vkQueueWaitIdle(queue)){
				throw vk_error{rst, "Wait On Queue Failed"};
			}
		}

		void record_post_command(){
			auto& image_data = final_staging_image;

			for (const auto & arr : sync_arr){
				arr.fence.wait();
			}

			if(!image_data.image){
				//Do nothing when there is no image to post
				for (auto && swap_chain_frame : swap_chain_frames){
					swap_chain_frame.post_command = {};
				}
				return;
			}

			for (auto && swap_chain_frame : swap_chain_frames){
				auto& bf = swap_chain_frame.post_command;
				bf = main_graphic_command_pool.obtain();
				bf.begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

				cmd::dependency_gen dependency_gen{};
				constexpr VkImageSubresourceRange subresource_range{
					VK_IMAGE_ASPECT_COLOR_BIT,
					0, 1, 0, 1
				};

				{
					dependency_gen.push(
					   image_data.image,
					   image_data.src_stage,
					   image_data.src_access,
					   VK_PIPELINE_STAGE_2_TRANSFER_BIT,
					   VK_ACCESS_2_TRANSFER_READ_BIT,
					   image_data.src_layout,
					   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					   subresource_range,
					   image_data.owner_queue_family,
					   graphic_family()
				   );

					dependency_gen.push(
						swap_chain_frame.image,
						image_data.src_stage,
						VK_ACCESS_2_NONE,
						VK_PIPELINE_STAGE_2_TRANSFER_BIT,
						VK_ACCESS_2_TRANSFER_WRITE_BIT,
						VK_IMAGE_LAYOUT_UNDEFINED,
						VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						subresource_range
					);

					dependency_gen.apply(bf);
				}

				if(image_data.extent.width == get_extent().width && image_data.extent.height == get_extent().height){
					cmd::copy_image_to_image(
						bf,
						image_data.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						swap_chain_frame.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,{
							VkImageCopy{
								.srcSubresource = {
									.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
									.mipLevel = 0,
									.baseArrayLayer = 0,
									.layerCount = 1
								},
								.srcOffset = {},
								.dstSubresource = {
									.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
									.mipLevel = 0,
									.baseArrayLayer = 0,
									.layerCount = 1
								},
								.dstOffset = {},
								.extent = get_extent3()
							}
						}
					);
				} else{
					cmd::image_blit(
						bf,
						image_data.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						swap_chain_frame.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						VK_FILTER_LINEAR, {
							VkImageBlit{
								.srcSubresource = {
									.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
									.mipLevel = 0,
									.baseArrayLayer = 0,
									.layerCount = 1
								},
								.srcOffsets = {
									{},
									{
										static_cast<std::int32_t>(image_data.extent.width),
										static_cast<std::int32_t>(image_data.extent.height), 1
									}
								},
								.dstSubresource = {
									.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
									.mipLevel = 0,
									.baseArrayLayer = 0,
									.layerCount = 1
								},
								.dstOffsets = {
									{},
									{
										static_cast<std::int32_t>(swap_chain_extent.width),
										static_cast<std::int32_t>(swap_chain_extent.height), 1
									}
								}
							}
						}
					);
				}


				if(image_data.clear){
					dependency_gen.push(
						image_data.image,
						VK_PIPELINE_STAGE_2_TRANSFER_BIT,
						VK_ACCESS_2_TRANSFER_READ_BIT,
						VK_PIPELINE_STAGE_2_TRANSFER_BIT,
						VK_ACCESS_2_TRANSFER_WRITE_BIT,
						VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						subresource_range
					);
					dependency_gen.apply(bf);

					cmd::clear_color_image(
						bf,
						image_data.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						{}, {subresource_range});
				}

				{
					dependency_gen.push_post_write(
						swap_chain_frame.image,
						VK_PIPELINE_STAGE_2_NONE,
						VK_ACCESS_2_NONE,
						VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
						subresource_range
					);

					if(image_data.dst_layout != VK_IMAGE_LAYOUT_UNDEFINED)dependency_gen.push(
						image_data.image,
						VK_PIPELINE_STAGE_2_TRANSFER_BIT,
						image_data.clear ? VK_ACCESS_2_TRANSFER_WRITE_BIT : VK_ACCESS_2_TRANSFER_READ_BIT,
						image_data.dst_stage,
						image_data.dst_access,
						image_data.clear ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						image_data.dst_layout,
						subresource_range,
						graphic_family(),
						image_data.owner_queue_family
					);
					dependency_gen.apply(bf);
				}

				bf.end();
			}
		}

		void init(const VkApplicationInfo& app_info){
			instance = vk::instance{app_info};
			validationEntry = validation_entry{instance};

			window_ = window_instance{app_info.pApplicationName};
			surface = window_.create_surface(instance);

			create_device();

			main_compute_command_pool = {get_device(), compute_family(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT};
			main_graphic_command_pool = {get_device(), graphic_family(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT};
			main_compute_command_pool_transient = {get_device(), compute_family(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT};
			main_graphic_command_pool_transient = {get_device(), graphic_family(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT};

			for (auto & frame_data : sync_arr){
				frame_data.fence = fence{device, true};
				frame_data.flush_semaphore = semaphore{device};
				frame_data.fetch_semaphore = semaphore{device};
			}

			allocator_ = vk::allocator{instance, physical_device, device};

			createSwapChain();
		}

		void createSwapChain(){
			const swap_chain_info swapChainSupport(physical_device, surface);
			const VkSurfaceFormatKHR surfaceFormat = swap_chain_info::choose_swap_surface_format(swapChainSupport.formats);
			swap_chain_extent = choose_swap_extent(swapChainSupport.capabilities);
			const queue_family_indices indices(physical_device, surface);
			const std::array queueFamilyIndices{indices.graphic.index, indices.present.index};

			std::uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
			if(swapChainSupport.capabilities.maxImageCount){
				imageCount = std::min(imageCount, swapChainSupport.capabilities.maxImageCount);
			}

			VkSwapchainCreateInfoKHR createInfo{
					.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
					.surface = surface,
					.minImageCount = imageCount,
					.imageFormat = surfaceFormat.format,
					.imageColorSpace = surfaceFormat.colorSpace,
					.imageExtent = swap_chain_extent,
					.imageArrayLayers = 1,
					.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				};


			if(indices.graphic.index != indices.present.index){
				createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				createInfo.queueFamilyIndexCount = 2;
				createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
			} else{
				createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				createInfo.queueFamilyIndexCount = 0;     // Optional
				createInfo.pQueueFamilyIndices = nullptr; // Optional
			}

			createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
			createInfo.oldSwapchain = last_swap_chain;

			//Set Window Alpha Blend Mode
			createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

			createInfo.presentMode = swap_chain_info::choose_swap_present_mode(swapChainSupport.presentModes);
			createInfo.clipped = true;

			if(auto rst = vkCreateSwapchainKHR(device, &createInfo, nullptr, swap_chain.as_data())){
				throw vk_error(rst, "Failed to create swap chain!");
			}

			swapChainImageFormat = surfaceFormat.format;

			createImageViews();
		}

		void createImageViews(){
			auto [images, rst] = enumerate(vkGetSwapchainImagesKHR, device.get(), swap_chain.handle);
			swap_chain_frames.resize(images.size());

			for(const auto& [index, imageGroup] : swap_chain_frames | std::ranges::views::enumerate){
				imageGroup.image = images[index];
			}
		}

		[[nodiscard]] VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities) const{
			if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()){
				return capabilities.currentExtent;
			}

			auto size = window_.get_size();

			size.width = std::clamp(size.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			size.height = std::clamp(size.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return size;
		}

		void create_device(){
			auto [devices, rst] = enumerate(vkEnumeratePhysicalDevices, static_cast<VkInstance>(instance));

			if(devices.empty()){
				throw std::runtime_error("Failed to find GPUs with Vulkan support!");
			}

			// Use an ordered map to automatically sort candidates by increasing score
			std::multimap<std::uint32_t, struct physical_device, std::greater<std::uint32_t>> candidates{};

			for(const auto& device : devices){
				auto d = vk::physical_device{device};
				candidates.insert(std::make_pair(d.rate_device(), d));
			}

			// Check if the best candidate is suitable at all
			for(const auto& [score, device] : candidates){
				if(score && device.valid(surface)){
					physical_device = device;
					break;
				}
			}

			if(!physical_device){
				std::println(std::cerr, "[Vulkan] Failed to find a suitable GPU");
				throw unqualified_error("Failed to find a suitable GPU!");
			} else{
				std::println("[Vulkan] On Physical Device: {}", physical_device.get_name());
			}

			physical_device.cache_properties(surface);

			device = logical_device{physical_device, physical_device.queues};
		}

		void recreate(){
			while (window_.iconified() || window_.get_size().width == 0 || window_.get_size().height == 0){
				window_.wait_event();
			}

			vkDeviceWaitIdle(device);

			if(last_swap_chain)vkDestroySwapchainKHR(device, last_swap_chain, nullptr);
			last_swap_chain.handle = swap_chain;

			createSwapChain();

			eventManager.fire(window_instance::resize_event(get_extent()));

			record_post_command();

			// recreateCallback(*this);
			// resized = false;
		}
	};
}
