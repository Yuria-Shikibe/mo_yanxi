module;

#include "../src/ext/adapted_attributes.hpp"

module mo_yanxi.game.graphic.effect;

import mo_yanxi.graphic.renderer.world;
import mo_yanxi.graphic.draw;
import mo_yanxi.graphic.draw.func;
import mo_yanxi.graphic.draw.trail;

namespace mo_yanxi::game{
	namespace draw = graphic::draw;
	FORCE_INLINE graphic::draw::world_acquirer<> get_acquirer(const fx::effect& e, const fx::effect_draw_context& ctx){
		graphic::draw::world_acquirer<> acq {ctx.renderer.batch};
		acq.proj.depth = e.depth;
		return acq;
	}



	void fx::trail_effect::operator()(const effect& e, const effect_draw_context& ctx) const noexcept{
		const auto c1 = color.in[e.duration.get()];
		const auto c2 = color.out[e.duration.get()];

		auto acquire = get_acquirer(e, ctx);

		if(c1 == c2){
			draw::fancy::trail(acquire, trail, e.trans.rot, c1, e.duration.get_inv());
		}else{
			draw::fancy::trail(acquire, trail, e.trans.rot, c1, c2, e.duration.get_inv());
		}
	}

	math::frect fx::trail_effect::get_clip_region(const effect& e, float) const noexcept{
		return trail.get_bound().expand(e.trans.rot);
	}

	void fx::shape_rect_ortho::operator()(const effect& e, const effect_draw_context& ctx) const noexcept{
		const float prog = e.duration.get();
		auto region = get_rect(e.trans.vec, prog);
		auto w = stroke[prog];

		auto acquire = get_acquirer(e, ctx);

		draw::line::rect_ortho(acquire, region, w, color[prog]);
	}

	math::frect fx::shape_rect_ortho::get_clip_region(const effect& e, const float min_clip_radius) const noexcept{
		const float prog = e.duration.get();
		auto region = get_rect(e.trans.vec, prog);
		math::frect min{e.trans.vec, min_clip_radius};
		return region.expand_by(min);
	}

	void fx::line_splash::operator()(const effect& e, const effect_draw_context& ctx) const noexcept{
		auto acquirer = get_acquirer(e, ctx);

		const auto prog = e.duration.get();
		const auto color_inner = palette.in[prog];
		const auto color_edge = palette.out[prog];
		const auto stroke_base = stroke.base[prog];
		const auto stroke_append = stroke.append[prog];

		const auto len_base = length.base[prog];
		const auto len_append_prog = length.append.interp(prog);

		splash_vec(
			e.id(),
			{
				.count = count,
				.progress = radius_interp(prog),
				.direction = e.trans.rot,
				.tolerance_angle = distribute_angle,
				.base_range = base_range,
				.range = range
			},
			[&](const math::trans2 trans, math::rand& rand){
				draw::line::line_angle(
					acquirer.get_reserved(4),
					{e.trans.vec + trans.vec, trans.rot - math::pi},
					len_base + rand.random(length.append.src, length.append.dst) * len_append_prog,
					stroke_base + rand.random(stroke_append),
					color_edge, color_inner
				);
			});
	}

	void fx::poly_line_out::operator()(const effect& e, const effect_draw_context& ctx) const noexcept{
		auto acquirer = get_acquirer(e, ctx);

		const auto prog = e.duration.get();
		const auto color_i = palette.in[prog];
		const auto color_o = palette.out[prog];

		if(is_circle()){
			draw::line::circle(acquirer, e.trans.vec, radius[prog], stroke[prog], color_i, color_o);
		}else{
			draw::line::poly(acquirer, e.trans, sides, radius[prog], stroke[prog], color_i, color_o);
		}
	}

	void fx::poly_outlined_out::operator()(const effect& e, const effect_draw_context& ctx) const noexcept{
		auto acquirer = get_acquirer(e, ctx);

		const auto prog = e.duration.get();

		if(is_circle()){
			draw::fancy::circle_outlined(
			acquirer, e.trans.vec, radius[prog],
			stroke[prog],
				palette.edge_in[prog],
				palette.edge_out[prog],
				palette.center_in[prog],
				palette.center_out[prog]);
		} else{
			draw::fancy::poly_outlined(
				acquirer, e.trans, sides, radius[prog],
				stroke[prog],
				palette.edge_in[prog],
				palette.edge_out[prog],
				palette.center_in[prog],
				palette.center_out[prog]);
		}
	}

	math::frect fx::virtual_effect_drawer_base::get_clip_region(const effect& e, float min_clip) const noexcept{
		return {e.trans.vec, min_clip};
	}
}
