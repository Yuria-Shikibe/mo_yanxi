module;

#include "../src/ext/adapted_attributes.hpp"


export module mo_yanxi.graphic.draw.func;
export import mo_yanxi.graphic.draw;

export import mo_yanxi.math.quad;

import std;

namespace mo_yanxi::graphic::draw{
	using vec = math::vec2;
	using col = color;

	template <typename Proj, typename Vtx>
	void try_proj(Proj& proj, Vtx& vtx){
		if constexpr (std::invocable<Proj, Vtx&>){
			proj(vtx);
		}
	}
	template <typename Proj, typename Vtx>
	void try_proj_primitive(Proj& proj, Vtx& vtx){
		if constexpr (std::invocable<Proj, per_primitive_t, Vtx&>){
			proj(per_primitive, vtx);
		}
	}

	namespace fill{
		export
		template <typename Vtx, std::derived_from<uniformed_rect_uv> UV = uniformed_rect_uv, typename Proj = basic_batch_param_proj>
		FORCE_INLINE void fill(
			const batch_draw_param<Vtx, UV, Proj>& param,
			vec v00, vec v10, vec v11, vec v01,
			col color_scl_00 = colors::white,
			col color_scl_10 = colors::white,
			col color_scl_11 = colors::white,
			col color_scl_01 = colors::white,
			col color_ovr_00 = {},
			col color_ovr_10 = {},
			col color_ovr_11 = {},
			col color_ovr_01 = {}
			) noexcept {
			static constexpr mapper<Vtx> mapper{};

			draw::try_proj(param.proj, mapper(param.target, 0, v00, param.get_texture(), color_scl_00, color_ovr_00, param.uv.v00()));
			draw::try_proj(param.proj, mapper(param.target, 1, v10, param.get_texture(), color_scl_10, color_ovr_10, param.uv.v10()));
			draw::try_proj(param.proj, mapper(param.target, 2, v11, param.get_texture(), color_scl_11, color_ovr_11, param.uv.v11()));
			draw::try_proj(param.proj, mapper(param.target, 3, v01, param.get_texture(), color_scl_01, color_ovr_01, param.uv.v01()));
			draw::try_proj_primitive(param.proj, *param.target);
		}

		export
		template <typename Vtx, std::derived_from<uniformed_rect_uv> UV = uniformed_rect_uv, typename Proj = basic_batch_param_proj>
		FORCE_INLINE void quad(
			const batch_draw_param<Vtx, UV, Proj>& param,
			vec v00, vec v10, vec v11, vec v01,
			col color_scl = colors::white,
			col color_ovr = {}
			) noexcept {
			fill::fill(
				param,
				v00, v10, v11, v01,
				color_scl, color_scl, color_scl, color_scl,
				color_ovr, color_ovr, color_ovr, color_ovr
			);
		}


		export
		template <typename Vtx, std::derived_from<uniformed_rect_uv> UV = uniformed_rect_uv, typename Proj = basic_batch_param_proj>
		FORCE_INLINE void quad(
			const batch_draw_param<Vtx, UV, Proj>& param,
			const math::fquad& quad,
			col color_scl = colors::white,
			col color_ovr = {}
			) noexcept {
			fill::quad(
				param,
				quad.v0, quad.v1, quad.v2, quad.v3,
				color_scl, color_ovr
			);
		}


		export
		template <typename Vtx, std::derived_from<uniformed_rect_uv> UV = uniformed_rect_uv, typename Proj = basic_batch_param_proj>
		FORCE_INLINE void rect_ortho(
			const batch_draw_param<Vtx, UV, Proj>& param,
			const math::frect rect,
			col color_scl = colors::white,
			col color_ovr = {}
			) noexcept {
			fill::fill(
				param,
				rect.vert_00(), rect.vert_10(), rect.vert_11(), rect.vert_01(),
				color_scl, color_scl, color_scl, color_scl,
				color_ovr, color_ovr, color_ovr, color_ovr
			);
		}
	}

	namespace line{
		export
		template <typename Vtx, std::derived_from<uniformed_rect_uv> UV = uniformed_rect_uv, typename Proj = basic_batch_param_proj>
		FORCE_INLINE void line_ortho(
			const batch_draw_param<Vtx, UV, Proj>& param,
			const vec src, const vec dst,
			const float stroke = 2.f,
			const col color_scl_src = colors::white,
			const col color_scl_dst = colors::white,
			const col color_ovr_src = {},
			const col color_ovr_dst = {}
			){
			const vec diff = (dst - src).sign().scl(stroke / 2.f);

			fill::fill(
				param,
				src + vec{-diff.x - diff.y, -diff.y + diff.x},
				src + vec{-diff.x + diff.y, -diff.y - diff.x},
				dst + vec{+diff.x + diff.y, +diff.y - diff.x},
				dst + vec{+diff.x - diff.y, +diff.y + diff.x},
				color_scl_src, color_scl_src, color_scl_dst, color_scl_dst,
				color_ovr_src, color_ovr_src, color_ovr_dst, color_ovr_dst
			);
		}

		export
		template <typename Vtx, std::derived_from<uniformed_rect_uv> UV = uniformed_rect_uv, typename Proj = basic_batch_param_proj>
		FORCE_INLINE void rect_ortho(
			auto_batch_acquirer<Vtx, UV, Proj>& auto_param,
			const math::frect rect,
			const float stroke = 2.f,
			const color color = colors::white){
			acquirer_guard _{auto_param, 4};
			line::line_ortho(auto_param[0], rect.vert_00(), rect.vert_01(), stroke, color, color);
			line::line_ortho(auto_param[1], rect.vert_01(), rect.vert_11(), stroke, color, color);
			line::line_ortho(auto_param[2], rect.vert_11(), rect.vert_10(), stroke, color, color);
			line::line_ortho(auto_param[3], rect.vert_10(), rect.vert_00(), stroke, color, color);
		}
		//
		// export
		// template <typename Vtx, std::derived_from<uniformed_rect_uv> UV = uniformed_rect_uv, typename Proj = basic_batch_param_proj>
		// FORCE_INLINE void rect_ortho2(
		// 	auto_batch_acquirer<Vtx, UV, Proj>& auto_param,
		// 	const math::frect rect,
		// 	const float stroke = 2.f,
		// 	const color color = colors::white){
		// 	acquirer_guard _{auto_param, 4};
		// 	line::line_ortho(auto_param[0], rect.vert_00(), rect.vert_01(), stroke, color, color);
		// 	line::line_ortho(auto_param[1], rect.vert_01(), rect.vert_11(), stroke, color, color);
		// 	line::line_ortho(auto_param[2], rect.vert_11(), rect.vert_10(), stroke, color, color);
		// 	line::line_ortho(auto_param[3], rect.vert_10(), rect.vert_00(), stroke, color, color);
		// }
	}
}
