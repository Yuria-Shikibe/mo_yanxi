module;

#include "../src/ext/adapted_attributes.hpp"

export module mo_yanxi.graphic.draw.trail;

export import mo_yanxi.graphic.draw;
export import mo_yanxi.graphic.draw.func;
export import mo_yanxi.graphic.trail;
export import mo_yanxi.graphic.color;

import std;

namespace mo_yanxi::graphic::draw::fancy{
	export
	template <typename Vtx, std::derived_from<uniformed_rect_uv> UV, typename Proj>
	FORCE_INLINE void trail(
		auto_batch_acquirer<Vtx, UV, Proj>& param,
		const graphic::trail& trail,
		float radius, color color, float percent = 1.f) noexcept {
		acquirer_guard _{param, static_cast<std::size_t>(std::ceil(trail.size() * percent) + 1)};

		trail.each(radius, [&](int idx, math::vec2 v1, math::vec2 v2, math::vec2 v3, math::vec2 v4){
			fill::quad(param[idx], v1, v2, v3, v4, color);
		}, percent);
	}

	export
	template <typename Vtx, std::derived_from<uniformed_rect_uv> UV, typename Proj>
	FORCE_INLINE void trail(
		auto_batch_acquirer<Vtx, UV, Proj>& param,
		const graphic::trail& trail,
		float radius, color color_head, color color_tail, float percent = 1.f) noexcept {
		acquirer_guard _{param, static_cast<std::size_t>(std::ceil(trail.size() * percent) + 1)};

		trail.each(radius, [&](int idx, math::vec2 v1, math::vec2 v2, math::vec2 v3, math::vec2 v4, float prev, float next){
			auto prevc = color_tail.create_lerp(color_head, prev);
			auto postc = color_tail.create_lerp(color_head, next);
			fill::fill(param[idx],
					v1, v2, v3, v4,
					prevc, prevc, postc, postc);
		}, percent);
	}
}
