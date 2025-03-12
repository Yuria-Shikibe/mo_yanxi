module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk.attachment_pool;

import std;
export import mo_yanxi.vk.resources;
export import mo_yanxi.vk.context;
export import mo_yanxi.vk.image_derives;

export namespace mo_yanxi::graphic{
	struct attachment_pool{
		vk::context* context;

		std::vector<vk::color_attachment> attachments{};

		std::vector<vk::image_handle> acquire(const std::span<const VkImageUsageFlags> usages){
			std::vector<vk::image_handle> rst(usages.size());

			if(usages.size() == 1){
				for (auto & attachment : attachments){
					if((attachment.get_usage() & usages[0]) == usages[0]){
						rst[0] = attachment;
						return rst;
					}
				}

				auto& a = attachments.emplace_back(vk::color_attachment{context->get_allocator(), context->get_extent(), usages[0]});
				rst[0] = a;
				return rst;
			}else{
				std::unordered_set<vk::color_attachment*> candidates{};

				for (auto & attachment : attachments){
					candidates.insert(&attachment);
				}

				std::size_t found{};
				for (const VkImageUsageFlags usage : usages){
					for(auto itr = candidates.begin(); itr != candidates.end(); ++itr){
						if(((*itr)->get_usage() & usage) == usage){
							rst.push_back(**itr);
							candidates.erase(itr);
							found++;
							break;
						}
					}
				}


				if(found == usages.size()) return rst;
				//satisfy not found, add news

				for(auto&& [idx, image_handle] : rst | std::views::enumerate){
					if(image_handle.image) continue;

					auto& a = attachments.emplace_back(vk::color_attachment{
							context->get_allocator(), context->get_extent(), usages[idx]
						});
					image_handle = a;
				}

				return rst;
			}

		}

		void resize(const VkExtent2D extent){
			for (auto&& attachment : attachments){
				attachment.resize(extent);
			}
		}
	};
}
