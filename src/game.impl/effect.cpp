module;

#include "../src/ext/adapted_attributes.hpp"

module mo_yanxi.game.graphic.effect;

import mo_yanxi.graphic.renderer.world;
import mo_yanxi.graphic.draw;
import mo_yanxi.graphic.draw.func;

namespace mo_yanxi::game{
	namespace draw = graphic::draw;
	FORCE_INLINE graphic::draw::world_acquirer<> get_acquirer(const fx::effect& e, const fx::effect_draw_context& ctx){
		graphic::draw::world_acquirer<> acq {ctx.renderer.batch, ctx.get_default_image_region()};
		acq.proj.depth = e.depth;
		return acq;
	}

	graphic::combined_image_region<graphic::uniformed_rect_uv>
		fx::effect_draw_context::get_default_image_region() const noexcept{
		return graphic::draw::white_region;
	}

	float fx::effect_drawer_base::get_effect_prog(const effect& e) noexcept{
		return e.duration.get();
	}

	math::vec2 fx::effect_drawer_base::get_effect_pos(const effect& e) noexcept{
		return e.trans.vec;
	}

	void fx::shape_rect_ortho::operator()(const effect& e, const effect_draw_context& ctx) const noexcept{
		const float prog = e.duration.get();
		auto region = get_rect(e.trans.vec, prog);
		auto w = stroke[prog];

		auto acquire = get_acquirer(e, ctx);

		draw::line::rect_ortho(acquire, region, w, e.palette[0, prog]);
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
		const auto color_edge = e.palette[1, prog];
		const auto color_inner = e.palette[0, prog];
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
					{e.trans.vec + trans.vec, trans.rot - 180},
					len_base + rand.random(length.append.src, length.append.dst) * len_append_prog,
					stroke_base + rand.random(stroke_append),
					color_edge, color_inner
				);
			});
	}

	void fx::poly_line_out::operator()(const effect& e, const effect_draw_context& ctx) const noexcept{
		auto acquirer = get_acquirer(e, ctx);

		const auto prog = e.duration.get();
		const auto color = e.palette[prog];

		if(is_circle()){
			draw::line::circle(acquirer, e.trans.vec, radius[prog], color, stroke[prog]);
		}else{
			draw::line::poly(acquirer, e.trans, sides, radius[prog], color, stroke[prog]);
		}
	}
}
