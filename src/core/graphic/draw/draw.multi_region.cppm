//
// Created by Matrix on 2024/9/28.
//

export module mo_yanxi.graphic.draw.multi_region;

export import mo_yanxi.graphic.image_multi_region;
export import mo_yanxi.graphic.draw.func;

import std;
import ext.guard;

namespace mo_yanxi::graphic::draw{

	export
	template <typename Vtx, std::derived_from<uniformed_rect_uv> UV = uniformed_rect_uv, typename Proj = basic_batch_param_proj>
	void nine_patch(
		auto_batch_acquirer<Vtx, UV, Proj>& param,
		const image_nine_region& nineRegion,
		math::frect bound,
		const color color = colors::white){
		if(color.a <= 0.0){
			return;
		}

		const auto rects = nineRegion.get_regions(bound.expand(nineRegion.margin, nineRegion.margin));

		param << nineRegion.image_view.view;

		{
			draw::acquirer_guard _{param, image_nine_region::size};
			for(std::size_t i = 0; i < image_nine_region::size; ++i){
				param << nineRegion.regions[i];
				/*if(i == 3 || i == 5)*/fill::rect_ortho(param[i], rects[i], color);
			}
		}
	}
	export
	template <typename Vtx, std::derived_from<uniformed_rect_uv> UV = uniformed_rect_uv, typename Proj = basic_batch_param_proj>
	void caped_region(
		auto_batch_acquirer<Vtx, UV, Proj>& param,
		const image_caped_region& region,
		math::vec2 src,
		math::vec2 dst,
		const color color,
		float scale = 1.f){
		if(color.a <= 0.0 || scale == 0.f) [[unlikely]] {
			return;
		}

		const auto rects = region.get_regions(src, dst, scale);

		param << region.get_image_view();

		{
			draw::acquirer_guard _{param, image_caped_region::size};
			for(std::size_t i = 0; i < image_caped_region::size; ++i){

				param << region[i];
				fill::quad(param[i], rects[i], color);
			}
		}

		// param << draw::white_region;
		// for(std::size_t i = 0; i < image_caped_region::size; ++i){
		// 	line::rect_ortho(param, rects[i].get_bound(), 2.f + i, color);
		// }
	}
}
