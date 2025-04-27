module;

#include <vulkan/vulkan.h>
#include <cassert>

module mo_yanxi.graphic.image_atlas;

import mo_yanxi.vk.util.cmd.render;

namespace mo_yanxi::graphic{
	void async_image_loader::load(allocated_image_load_description&& desc){
		static constexpr VkDeviceSize cahnnel_size = 4;
		static constexpr std::uint32_t max_prov = 3;

		const auto mipmap_level = std::min(std::min(max_prov, desc.mip_level), desc.desc.get_prov_levels());
		assert(mipmap_level > 0);
		VkDeviceSize maximumsize = desc.region.area();
		maximumsize = vk::get_mipmap_pixels(maximumsize, mipmap_level);
		maximumsize *= cahnnel_size;

		vk::buffer buffer{};
		if(auto itr = stagings.lower_bound(maximumsize); itr != stagings.end()){
			buffer = std::move(stagings.extract(itr).mapped());
		}else{
			constexpr VkDeviceSize chunk_size = 256;
			auto ceil = std::max(std::bit_ceil(maximumsize * 2), chunk_size * chunk_size * mipmap_level);
			if(ceil > 2048 * 2048 * cahnnel_size){
				ceil = chunk_size * (maximumsize / chunk_size + (maximumsize % chunk_size != 0));
			}
			buffer = vk::templates::create_staging_buffer(context_->get_allocator(), ceil);
		}

		vk::command_buffer command_buffer{context_->get_graphic_command_pool_transient().obtain()};

		{
			vk::scoped_recorder scoped_recorder{command_buffer};
			{
				VkDeviceSize off{};
				for(std::uint32_t mip_lv = 0; mip_lv < mipmap_level; ++mip_lv){
					const auto scl = 1u << mip_lv;
					const VkExtent2D extent{desc.region.width() / scl, desc.region.height() / scl};
					auto bitmap = desc.desc.get(extent.width, extent.height, mip_lv);
					(void)vk::buffer_mapper{buffer}.load_range(bitmap.get_bytes(), static_cast<std::ptrdiff_t>(off));
					off += extent.width * extent.height * cahnnel_size;
				}
			}

			vk::cmd::generate_mipmaps(command_buffer, desc.texture.image, buffer, {
										  static_cast<std::int32_t>(desc.region.src.x),
										  static_cast<std::int32_t>(desc.region.src.y), desc.region.width(),
										  desc.region.height()
									  }, desc.mip_level, mipmap_level, desc.layer_index, 1);
		}

		fence_.wait_and_reset();
		vk::cmd::submit_command(
			context_->get_device().graphic_queue(1), command_buffer, fence_);

		running_command_buffer_ = std::move(command_buffer);

		if(stagings.size() > 16){
			//TODO better resource recycle
			stagings.clear();
		}

		auto sz = buffer.get_size();
		stagings.insert({sz, std::move(buffer)});
	}
}
