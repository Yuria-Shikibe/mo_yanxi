module;

#include <vulkan/vulkan.h>

module mo_yanxi.vk.context;

import mo_yanxi.vk.ext;

namespace mo_yanxi::vk{
	void context::flush(){
		if(!final_staging_image.image)return;

		auto& current_syncs = ++sync_arr;
		std::uint32_t imageIndex;


		const VkAcquireNextImageInfoKHR acquire_info{
				.sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR,
				.pNext = nullptr,
				.swapchain = swap_chain,
				.timeout = std::numeric_limits<std::uint64_t>::max(),
				.semaphore = current_syncs.fetch_semaphore,
				.fence = nullptr,
				.deviceMask = 0x1
			};

		current_syncs.fence.wait_and_reset();
		const auto result = vkAcquireNextImage2KHR(device, &acquire_info, &imageIndex);
		// constexpr auto timeout = std::numeric_limits<std::uint64_t>::max();
		// const auto result = vkAcquireNextImageKHR(device, swap_chain, timeout, current_syncs.fetch_semaphore, nullptr, &imageIndex);

		bool out_of_date{};
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			out_of_date = true;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw vk_error(result, "failed to acquire swap chain image!");
		}

		const auto& frame = swap_chain_frames[imageIndex];

		cmd::submit_command/*<2, 2>*/(
			device.primary_graphics_queue(),
			frame.post_command,
			current_syncs.fence,
			current_syncs.fetch_semaphore,
			VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT
			,
			frame.flush_semaphore,
			VK_PIPELINE_STAGE_2_TRANSFER_BIT
		);



		const VkPresentInfoKHR info{
				.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
				.pNext = nullptr,
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = frame.flush_semaphore.as_data(),
				.swapchainCount = 1,
				.pSwapchains = swap_chain.as_data(),
				.pImageIndices = &imageIndex,
				.pResults = nullptr,
			};


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

	void context::wait_on_device() const{
		if(const auto rst = vkDeviceWaitIdle(device)){
			throw vk_error{rst, "Failed to wait on device."};
		}
	}

	void context::record_post_command(){
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

	void context::recreate(){
		while (window_.iconified() || window_.get_size().width == 0 || window_.get_size().height == 0){
			window_.wait_event();
		}

		// if(auto size = window_.get_size(); size.width * size.height == 0)

		wait_on_device();

		if(last_swap_chain)vkDestroySwapchainKHR(device, last_swap_chain, nullptr);
		last_swap_chain.handle = swap_chain;

		createSwapChain();

		eventManager.fire(window_instance::resize_event(get_extent()));

		record_post_command();

		// recreateCallback(*this);
		// resized = false;
	}
}
