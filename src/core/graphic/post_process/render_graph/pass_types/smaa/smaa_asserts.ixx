module;

#include <vulkan/vulkan.h>
#include "AreaTex.h"
#include "SearchTex.h"

export module mo_yanxi.graphic.render_graph.smaa_asserts;

import mo_yanxi.vk.image_derives;
import mo_yanxi.vk.resources;
import mo_yanxi.core.global.graphic;
import mo_yanxi.vk.context;

import std;

namespace mo_yanxi::assets::graphic{
	inline vk::texture area_tex{};
	inline vk::texture search_tex{};
	struct smaa_image_result{
		vk::image_handle area_tex;
		vk::image_handle search_tex;
	};

	//TODO move these to .cpp file
	export smaa_image_result get_smaa_tex(){
		if(area_tex){
			return {area_tex, search_tex};
		}else{
			auto& ctx = core::global::graphic::context;
			ctx.add_dispose([] noexcept {
				area_tex = {};
				search_tex = {};
			});

			constexpr static auto areaTexBinarySz = std::size(areaTexBytes);
			constexpr static auto searchTexBinarySz = std::size(searchTexBytes);
			//
			// constexpr static auto usage =
			// 	VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

			area_tex = vk::texture{ctx.get_allocator(), {AREATEX_WIDTH, AREATEX_HEIGHT}, VK_FORMAT_R8G8_UNORM};
			search_tex = vk::texture{ctx.get_allocator(), {SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT}, VK_FORMAT_R8_UNORM};
			auto buf = vk::templates::create_staging_buffer(ctx.get_allocator(), std::max(areaTexBinarySz, searchTexBinarySz));

			(void)vk::buffer_mapper{buf}.load_range(areaTexBytes);

			{
				auto cmdbuf = ctx.get_transient_compute_command_buffer();
				area_tex.init_layout(cmdbuf);
				search_tex.init_layout(cmdbuf);
				area_tex.write(cmdbuf, {vk::texture_buffer_write{
					.buffer = buf,
					.region = {{}, area_tex.get_image().get_extent2()}
				}});
			}

			{
				auto cmdbuf = ctx.get_transient_compute_command_buffer();
				(void)vk::buffer_mapper{buf}.load_range(searchTexBytes);
				search_tex.write(cmdbuf, {vk::texture_buffer_write{
					.buffer = buf,
					.region = {{}, search_tex.get_image().get_extent2()}
				}});
			}

			return {area_tex, search_tex};
		}
	}
}