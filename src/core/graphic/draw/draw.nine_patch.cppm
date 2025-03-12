//
// Created by Matrix on 2024/9/28.
//

export module mo_yanxi.graphic.draw.nine_patch;

export import mo_yanxi.graphic.image_nine_region;
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
		const color color){
		if(color.a <= 0.0){
			return;
		}

		const auto rects = nineRegion.get_patches(bound.expand(nineRegion.margin, nineRegion.margin));

		param << nineRegion.image_view.view;

		draw::acquirer_guard _{param, image_nine_region::size};
		for(std::size_t i = 0; i < image_nine_region::size; ++i){
			param << nineRegion.regions[i];
			fill::rect_ortho(param[i], rects[i], color);
		}

		// param << draw::white_region;
		// for(std::size_t i = 0; i < image_nine_region::size; ++i){
		// 	line::rect_ortho(param, rects[i], 2.f, color);
		// }
	}
}
