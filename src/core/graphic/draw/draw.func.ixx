module;

#include "../src/ext/adapted_attributes.hpp"
#define NO_INITIALIZATION /*[[indeterminate]]*/

export module mo_yanxi.graphic.draw.func;
export import mo_yanxi.graphic.draw;

export import mo_yanxi.math.quad;

import std;

namespace mo_yanxi::graphic::draw{
	using vec = math::vec2;
	using col = color;

	constexpr float CircleVertPrecision{12};

	FORCE_INLINE constexpr int getCircleVertices(const float radius) noexcept{
		return math::clamp(static_cast<int>(radius * math::pi / CircleVertPrecision), 12, 512);
	}

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
			col color_scl_01 = colors::white
			) noexcept {
			static constexpr mapper<Vtx> mapper{};

			draw::try_proj(param.proj, mapper(param.target, 0, v00, param.get_texture(), color_scl_00, param.uv.v00()));
			draw::try_proj(param.proj, mapper(param.target, 1, v10, param.get_texture(), color_scl_10, param.uv.v10()));
			draw::try_proj(param.proj, mapper(param.target, 2, v11, param.get_texture(), color_scl_11, param.uv.v11()));
			draw::try_proj(param.proj, mapper(param.target, 3, v01, param.get_texture(), color_scl_01, param.uv.v01()));
			draw::try_proj_primitive(param.proj, *param.target);
		}

		export
		template <typename Vtx, std::derived_from<uniformed_rect_uv> UV = uniformed_rect_uv, typename Proj = basic_batch_param_proj>
		FORCE_INLINE void quad(
			const batch_draw_param<Vtx, UV, Proj>& param,
			vec v00, vec v10, vec v11, vec v01,
			col color_scl = colors::white
			) noexcept {
			fill::fill(
				param,
				v00, v10, v11, v01,
				color_scl, color_scl, color_scl, color_scl
			);
		}


		export
		template <
			typename Vtx, std::derived_from<uniformed_rect_uv> UV,
			typename Proj,
			math::quad_like Quad
		>
		FORCE_INLINE void quad(
			const batch_draw_param<Vtx, UV, Proj>& param,
			const Quad& quad,
			col color_scl = colors::white
			) noexcept {
			fill::quad(
				param,
				quad.v00(), quad.v10(), quad.v11(), quad.v01(),
				color_scl
			);
		}


		export
		template <typename Vtx, std::derived_from<uniformed_rect_uv> UV = uniformed_rect_uv, typename Proj = basic_batch_param_proj>
		FORCE_INLINE void rect_ortho(
			const batch_draw_param<Vtx, UV, Proj>& param,
			const math::frect rect,
			col color_scl = colors::white
			) noexcept {
			fill::fill(
				param,
				rect.vert_00(), rect.vert_10(), rect.vert_11(), rect.vert_01(),
				color_scl, color_scl, color_scl, color_scl
				);
		}

		export
		template <typename Vtx, std::derived_from<uniformed_rect_uv> UV = uniformed_rect_uv, typename Proj = basic_batch_param_proj>
		FORCE_INLINE void rect(
			const batch_draw_param<Vtx, UV, Proj>& param,
			const math::trans2 center,
			const math::frect rect_offset_in_local,
			col color_scl = colors::white
			) noexcept {

			const auto size = rect_offset_in_local.size() * .5f;

			auto [cos, sin] = center.rot_cos_sin();
			const auto off = center.vec + rect_offset_in_local.src.copy().rotate(cos, sin);
			const float w1 =  cos * size.x;
			const float h1 =  sin * size.x;

			const float w2 = -sin * size.y;
			const float h2 = cos  * size.y;
			fill::fill(
				param,
				off + vec{-w1 - w2, -h1 - h2},
				off + vec{+w1 - w2, +h1 - h2},
				off + vec{+w1 + w2, +h1 + h2},
				off + vec{-w1 + w2, -h1 + h2},
				color_scl, color_scl, color_scl, color_scl);
		}

		export
		template <typename Vtx, std::derived_from<uniformed_rect_uv> UV = uniformed_rect_uv, typename Proj = basic_batch_param_proj>
		FORCE_INLINE void rect(
			const batch_draw_param<Vtx, UV, Proj>& param,
			const math::trans2 center,
			const vec size,
			col color_scl = colors::white
			) noexcept {

			const auto sz = size * .5f;

			auto [cos, sin] = center.rot_cos_sin();

			const float w1 =  cos * sz.x;
			const float h1 =  sin * sz.x;

			const float w2 = -sin * sz.y;
			const float h2 = cos  * sz.y;
			fill::fill(
				param,
				center.vec + vec{-w1 - w2, -h1 - h2},
				center.vec + vec{+w1 - w2, +h1 - h2},
				center.vec + vec{+w1 + w2, +h1 + h2},
				center.vec + vec{-w1 + w2, -h1 + h2},
				color_scl, color_scl, color_scl, color_scl);
		}

		export
		template <
			typename Vtx, std::derived_from<uniformed_rect_uv> UV, typename Proj>
		void poly(
			auto_batch_acquirer<Vtx, UV, Proj>& auto_param,
			math::trans2 trans,
			const int sides,
			const float radius,
			const col center_color = colors::white,
			const col edge_color = colors::white){
			const float space = math::pi_2 / static_cast<float>(sides);

			static constexpr std::size_t MaxReserve = 128;

			if(static_cast<std::size_t>(sides / 2 + (sides & 1) + 1) <= MaxReserve){
				acquirer_guard _{auto_param, static_cast<std::size_t>(sides / 2 + (sides & 1))};
				int i = 0;
				for(; i < sides - 1; i+= 2){
					const float abase = space * static_cast<float>(i) + trans.rot;

					auto [cos1, sin1] = math::cos_sin(abase);
					auto [cos2, sin2] = math::cos_sin(abase + space);
					auto [cos3, sin3] = math::cos_sin(abase + space * 2);

					fill::fill(
						auto_param[i / 2],
						trans.vec,
						vec{trans.vec.x + radius * cos1, trans.vec.y + radius * sin1},
						vec{trans.vec.x + radius * cos2, trans.vec.y + radius * sin2},
						vec{trans.vec.x + radius * cos3, trans.vec.y + radius * sin3},
						center_color, edge_color, edge_color, edge_color
					);
				}

				if(i == sides)return;
				const float abase = space * static_cast<float>(i) + trans.rot;

				auto [cos1, sin1] = math::cos_sin(abase);
				auto [cos2, sin2] = math::cos_sin(abase + space);

				fill::fill(
					auto_param[i / 2],
					trans.vec,
					trans.vec,
					vec{trans.vec.x + radius * cos1, trans.vec.y + radius * sin1},
					vec{trans.vec.x + radius * cos2, trans.vec.y + radius * sin2},
					center_color, center_color, edge_color, edge_color
				);
			}else{
				int i = 0;
				for(; i < sides - 1; i+= 2){
					const float abase = space * static_cast<float>(i) + trans.rot;

					auto [cos1, sin1] = math::cos_sin(abase);
					auto [cos2, sin2] = math::cos_sin(abase + space);
					auto [cos3, sin3] = math::cos_sin(abase + space * 2);

					fill::fill(
						auto_param.get_reserved(math::min<std::size_t>((sides - i) / 2 + (sides & 1), MaxReserve)),
						trans.vec,
						vec{trans.vec.x + radius * cos1, trans.vec.y + radius * sin1},
						vec{trans.vec.x + radius * cos2, trans.vec.y + radius * sin2},
						vec{trans.vec.x + radius * cos3, trans.vec.y + radius * sin3},
						center_color, edge_color, edge_color, edge_color
					);
				}

				if(i == sides)return;
				const float abase = space * static_cast<float>(i) + trans.rot;

				auto [cos1, sin1] = math::cos_sin(abase);
				auto [cos2, sin2] = math::cos_sin(abase + space);

				fill::fill(
					auto_param.get(),
					trans.vec,
					trans.vec,
					vec{trans.vec.x + radius * cos1, trans.vec.y + radius * sin1},
					vec{trans.vec.x + radius * cos2, trans.vec.y + radius * sin2},
					center_color, center_color, edge_color, edge_color
				);
			}
		}

		export
		template <typename M>
		FORCE_INLINE void circle(
			M& auto_param,
			vec pos,
			const float radius,
			const col color = colors::white
			){
			fill::poly(auto_param, math::trans2{pos, 0}, getCircleVertices(radius), radius, color);
		}
	}

	namespace line{
		export 
		template <bool cap = true, typename Vtx, std::derived_from<uniformed_rect_uv> UV = uniformed_rect_uv, typename Proj = basic_batch_param_proj>
		FORCE_INLINE void line(
			const batch_draw_param<Vtx, UV, Proj>& param,
			const vec src, const vec dst,
			const float stroke = 2.f,
			const col color_scl_src = colors::white,
			const col color_scl_dst = colors::white
			){
			const float h_stroke = stroke / 2.0f;
			vec diff = dst - src;

			const float len = diff.length();
			diff *= h_stroke / len;

			if constexpr (cap){
				fill::fill(
					param,
					src + vec{-diff.x - diff.y, -diff.y + diff.x},
					src + vec{-diff.x + diff.y, -diff.y - diff.x},
					dst + vec{+diff.x + diff.y, +diff.y - diff.x},
					dst + vec{+diff.x - diff.y, +diff.y + diff.x},
					color_scl_src, color_scl_src, color_scl_dst, color_scl_dst);
			}else{
				fill::fill(
					param,
					src + vec{-diff.y, +diff.x},
					src + vec{+diff.y, -diff.x},
					dst + vec{+diff.y, -diff.x},
					dst + vec{-diff.y, +diff.x},
					color_scl_src, color_scl_src, color_scl_dst, color_scl_dst);
			}
		}

		export
		template <bool cap = true, typename Vtx, std::derived_from<uniformed_rect_uv> UV, typename Proj>
		FORCE_INLINE void line_angle_center(
			const batch_draw_param<Vtx, UV, Proj>& param,
			const math::trans2 trans,
			const float length,
			const float stroke = 2.f,
			const col color_scl_src = colors::white,
			const col color_scl_dst = colors::white){
			vec vec{};

			vec.set_polar_rad(trans.rot, length * 0.5f);

			line::line<cap>(param, trans.vec - vec, trans.vec + vec, stroke,
			                color_scl_src,
			                color_scl_dst
			);
		}

		export
		template <bool cap = true, typename Vtx, std::derived_from<uniformed_rect_uv> UV, typename Proj>
		FORCE_INLINE void line_angle(
			const batch_draw_param<Vtx, UV, Proj>& param,
			const math::trans2 trans,
			const float length,
			const float stroke = 2.f,
			const col color_scl_src = colors::white,
			const col color_scl_dst = colors::white){
			vec vec{};

			vec.set_polar_rad(trans.rot, length * 0.5f);

			line::line<cap>(param, trans.vec, trans.vec + vec, stroke,
							color_scl_src,
							color_scl_dst
			);
		}
		
		export
		template <typename Vtx, std::derived_from<uniformed_rect_uv> UV = uniformed_rect_uv, typename Proj = basic_batch_param_proj>
		FORCE_INLINE void line_ortho(
			const batch_draw_param<Vtx, UV, Proj>& param,
			const vec src, const vec dst,
			const float stroke = 2.f,
			const col color_scl_src = colors::white,
			const col color_scl_dst = colors::white
			){
			const vec diff = (dst - src).sign_or_zero().scl(stroke / 2.f);

			fill::fill(
				param,
				src + vec{-diff.x - diff.y, -diff.y + diff.x},
				src + vec{-diff.x + diff.y, -diff.y - diff.x},
				dst + vec{+diff.x + diff.y, +diff.y - diff.x},
				dst + vec{+diff.x - diff.y, +diff.y + diff.x},
				color_scl_src, color_scl_src, color_scl_dst, color_scl_dst);
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
			line::line_ortho(auto_param[1], rect.vert_01().add_x(stroke), rect.vert_11().add_x(-stroke), stroke, color, color);
			line::line_ortho(auto_param[2], rect.vert_11(), rect.vert_10(), stroke, color, color);
			line::line_ortho(auto_param[3], rect.vert_10().add_x(-stroke), rect.vert_00().add_x(stroke), stroke, color, color);
		}

		export
		template <typename Vtx, std::derived_from<uniformed_rect_uv> UV, typename Proj, typename QuadT>
		FORCE_INLINE void quad(
			auto_batch_acquirer<Vtx, UV, Proj>& auto_param,
			const QuadT& quad,
			const float stroke = 2.f,
			const color color = colors::white){
			acquirer_guard _{auto_param, 4};
			line::line(auto_param[0], quad[0], quad[1], stroke, color, color);
			line::line(auto_param[1], quad[1], quad[2], stroke, color, color);
			line::line(auto_param[2], quad[2], quad[3], stroke, color, color);
			line::line(auto_param[3], quad[3], quad[0], stroke, color, color);
		}

		export
		template <typename Vtx, std::derived_from<uniformed_rect_uv> UV, typename Proj, math::quad_like QuadT>
		FORCE_INLINE void quad_expanded(
			auto_batch_acquirer<Vtx, UV, Proj>& auto_param,
			const QuadT& quad,
			float expand = 2.f /*Can Be Negative*/,
			color color = colors::white){

			std::array<math::vec2, 4> pos NO_INITIALIZATION;
			for(int i = 0; i < 4; ++i){
				const auto v = quad[i];
				const auto normalL = quad.edge_normal_at(i - 1);
				const auto normalR = quad.edge_normal_at(i);
				const auto vert_normal = (normalR + normalL).normalize();
				const auto ang = vert_normal.angle_between_rad(normalL);
				pos[i] = v - vert_normal * (expand / mo_yanxi::math::cos(ang));
			}

			acquirer_guard _{auto_param, 4};
			fill::quad(auto_param[0], quad[0], pos[0], pos[1], quad[1], color);
			fill::quad(auto_param[1], quad[1], pos[1], pos[2], quad[2], color);
			fill::quad(auto_param[2], quad[2], pos[2], pos[3], quad[3], color);
			fill::quad(auto_param[3], quad[3], pos[3], pos[0], quad[0], color);
		}


		export
		template <typename Vtx, std::derived_from<uniformed_rect_uv> UV = uniformed_rect_uv, typename Proj = basic_batch_param_proj>
		FORCE_INLINE void square(
			auto_batch_acquirer<Vtx, UV, Proj>& auto_param,
				math::trans2 trans,
				const float radius,
				const float stroke = 2.f,

				const col color_scl = colors::white){
			trans.rot += math::pi / 4.f;
			const float dst = stroke / math::sqrt2;

			vec vec2_0{}, vec2_1{}, vec2_2{}, vec2_3{}, vec2_4{};

			vec2_0.set_polar_rad(trans.rot, 1);

			vec2_1.set(vec2_0);
			vec2_2.set(vec2_0);

			vec2_1.scl(radius - dst);
			vec2_2.scl(radius + dst);
			
			acquirer_guard _{auto_param, 4};
			for(int i = 0; i < 4; ++i){
				vec2_0.rotate_rt_clockwise();

				vec2_3.set(vec2_0).scl(radius - dst);
				vec2_4.set(vec2_0).scl(radius + dst);

				fill::fill(
					auto_param[i], vec2_1 + trans.vec, vec2_2 + trans.vec, vec2_4 + trans.vec, vec2_3 + trans.vec,
					color_scl, color_scl, color_scl, color_scl);

				vec2_1 = vec2_3;
				vec2_2 = vec2_4;
			}
		}

		export
		template <
			std::ranges::random_access_range Rng,
			typename Vtx, std::derived_from<uniformed_rect_uv> UV = uniformed_rect_uv, typename Proj = basic_batch_param_proj>
				requires (std::ranges::sized_range<Rng> && std::convertible_to<col, std::ranges::range_value_t<Rng>>)
		void poly_partial(
			auto_batch_acquirer<Vtx, UV, Proj>& auto_param,
				math::trans2 trans,
				const int sides,
				const float radius,
				const float ratio,
				const Rng& colorGroup,
				const float stroke = 2.f
			){
#if DEBUG_CHECK
				if(std::ranges::empty(colorGroup)){
					throw std::invalid_argument("Color group is Empty.");
				}
#endif
				const auto fSides = static_cast<float>(sides);

				const float space = math::pi_2 / fSides;
				float h_step = stroke / 2.0f / math::cos(space / 2.0f);
				const float r1 = radius - h_step;
				const float r2 = radius + h_step;

				float currentRatio;

				float currentAng = trans.rot;
				auto [cos1, sin1] = math::cos_sin(currentAng);
				float sin2, cos2;

				int progress = 0;
				col lerpColor1 = colorGroup[0];
				col lerpColor2 = colorGroup[std::ranges::size(colorGroup) - 1];

				const auto sz = math::ceil(fSides * ratio);
				if(sz == 0)return;

				for(; progress < fSides * ratio - 1.0f; ++progress){
					const float fp = static_cast<float>(progress);
					// NOLINT(*-flp30-c)
					// NOLINT(cert-flp30-c)
					currentAng = trans.rot + (fp + 1.0f) * space;

					cos2 = math::cos(currentAng);
					sin2 = math::sin(currentAng);

					currentRatio = fp / fSides;

					lerpColor2.lerp(currentRatio, colorGroup);

					fill::fill(
						auto_param.get_reserved(),
						vec{cos1 * r1 + trans.vec.x, sin1 * r1 + trans.vec.y},
						vec{cos1 * r2 + trans.vec.x, sin1 * r2 + trans.vec.y},
						vec{cos2 * r2 + trans.vec.x, sin2 * r2 + trans.vec.y},
						vec{cos2 * r1 + trans.vec.x, sin2 * r1 + trans.vec.y},
						lerpColor1, lerpColor1, lerpColor2, lerpColor2
					);

					lerpColor1.set(lerpColor2);

					sin1 = sin2;
					cos1 = cos2;
				}

				const float fp = static_cast<float>(progress);
				currentRatio = ratio;
				const float remainRatio = currentRatio * fSides - fp;

				currentAng = trans.rot + (fp + 1.0f) * space;

				cos2 = math::lerp(cos1, math::cos(currentAng), remainRatio);
				sin2 = math::lerp(sin1, math::sin(currentAng), remainRatio);

				lerpColor2.lerp(fp / fSides, colorGroup);
				lerpColor2.lerp(lerpColor1, 1.0f - remainRatio);

				fill::fill(
					auto_param.get(),
					vec{cos1 * r1 + trans.vec.x, sin1 * r1 + trans.vec.y},
					vec{cos1 * r2 + trans.vec.x, sin1 * r2 + trans.vec.y},
					vec{cos2 * r2 + trans.vec.x, sin2 * r2 + trans.vec.y},
					vec{cos2 * r1 + trans.vec.x, sin2 * r1 + trans.vec.y},
					lerpColor1, lerpColor1, lerpColor2, lerpColor2
				);
			}

		export
		template <
			typename Vtx, std::derived_from<uniformed_rect_uv> UV = uniformed_rect_uv, typename Proj = basic_batch_param_proj>
		void poly_partial(
			auto_batch_acquirer<Vtx, UV, Proj>& auto_param,
				math::trans2 trans,
				const int sides,
				const float radius,
				const float ratio,
				const float stroke = 2.f,
				const col color = colors::white
			){
				const auto fSides = static_cast<float>(sides);

				const float space = math::pi_2 / fSides;
				float h_step = stroke / 2.0f / math::cos(space / 2.0f);
				const float r1 = radius - h_step;
				const float r2 = radius + h_step;

				float currentRatio;

				float currentAng = trans.rot;
				auto [cos1, sin1] = math::cos_sin(currentAng);
				float sin2, cos2;

				int progress = 0;

				const auto sz = math::ceil(fSides * ratio);
				if(sz == 0)return;

				for(; progress < fSides * ratio - 1.0f; ++progress){
					const float fp = static_cast<float>(progress);
					// NOLINT(*-flp30-c)
					// NOLINT(cert-flp30-c)
					currentAng = trans.rot + (fp + 1.0f) * space;

					cos2 = math::cos(currentAng);
					sin2 = math::sin(currentAng);

					currentRatio = fp / fSides;

					fill::fill(
						auto_param.get_reserved(),
						vec{cos1 * r1 + trans.vec.x, sin1 * r1 + trans.vec.y},
						vec{cos1 * r2 + trans.vec.x, sin1 * r2 + trans.vec.y},
						vec{cos2 * r2 + trans.vec.x, sin2 * r2 + trans.vec.y},
						vec{cos2 * r1 + trans.vec.x, sin2 * r1 + trans.vec.y},
						color, color, color, color
					);

					sin1 = sin2;
					cos1 = cos2;
				}

				const float fp = static_cast<float>(progress);
				currentRatio = ratio;
				const float remainRatio = currentRatio * fSides - fp;

				currentAng = trans.rot + (fp + 1.0f) * space;

				cos2 = math::lerp(cos1, math::cos(currentAng), remainRatio);
				sin2 = math::lerp(sin1, math::sin(currentAng), remainRatio);

				fill::fill(
					auto_param.get(),
					vec{cos1 * r1 + trans.vec.x, sin1 * r1 + trans.vec.y},
					vec{cos1 * r2 + trans.vec.x, sin1 * r2 + trans.vec.y},
					vec{cos2 * r2 + trans.vec.x, sin2 * r2 + trans.vec.y},
					vec{cos2 * r1 + trans.vec.x, sin2 * r1 + trans.vec.y},
					color, color, color, color
				);
			}

		export
		template <typename Vtx, std::derived_from<uniformed_rect_uv> UV, typename Proj>
		void poly(
			auto_batch_acquirer<Vtx, UV, Proj>& auto_param,
			math::trans2 trans,
			const int sides,
			const float radius,
			const float stroke = 2.f,
			const col color_inner = colors::white,
			const col color_outer = colors::white){

			const auto space = math::pi_2 / static_cast<float>(sides);
			const auto h_step = stroke / 2.0f / math::cos(space / 2.0f);
			const auto r1 = radius - h_step;
			const auto r2 = radius + h_step;

			static constexpr std::size_t MaxReserve = 64;

			if(sides <= MaxReserve){
				acquirer_guard _{auto_param, static_cast<std::size_t>(sides)};
				for(int i = 0; i < sides; i++){
					const float a = space * static_cast<float>(i) + trans.rot;
					auto [cos1, sin1] = math::cos_sin(a);
					auto [cos2, sin2] = math::cos_sin(a + space);
					fill::fill(
						auto_param[i],
						vec{trans.vec.x + r1 * cos1, trans.vec.y + r1 * sin1},
						vec{trans.vec.x + r1 * cos2, trans.vec.y + r1 * sin2},
						vec{trans.vec.x + r2 * cos2, trans.vec.y + r2 * sin2},
						vec{trans.vec.x + r2 * cos1, trans.vec.y + r2 * sin1},
						color_inner, color_inner, color_outer, color_outer
					);
				}
			}else{
				for(int i = 0; i < sides; i++){
					const float a = space * static_cast<float>(i) + trans.rot;
					auto [cos1, sin1] = math::cos_sin(a);
					auto [cos2, sin2] = math::cos_sin(a + space);
					fill::fill(
						auto_param.get_reserved(math::min<std::size_t>(sides - i, MaxReserve)),
						vec{trans.vec.x + r1 * cos1, trans.vec.y + r1 * sin1},
						vec{trans.vec.x + r1 * cos2, trans.vec.y + r1 * sin2},
						vec{trans.vec.x + r2 * cos2, trans.vec.y + r2 * sin2},
						vec{trans.vec.x + r2 * cos1, trans.vec.y + r2 * sin1},
						color_inner, color_inner, color_outer, color_outer
					);
				}
			}
		}

		export
		template <typename M>
		FORCE_INLINE void circle(
			M& auto_param,
			vec pos,
			const float radius,
			const float stroke = 2.f,
			const col color_inner = colors::white,
			const col color_outer = colors::white
		){
			line::poly(auto_param, math::trans2{pos, 0}, getCircleVertices(radius), radius, stroke, color_inner, color_outer);
		}

		export
		template <typename M>
		FORCE_INLINE void circle_partial(
			M& auto_param,
			math::trans2 trans,
			const float radius,
			const float ratio = 1.f,
			const float stroke = 2.f,
			const col color = colors::white
		){
			line::poly_partial(auto_param, trans, getCircleVertices(radius), radius, ratio, stroke, color);
		}
	}

	namespace fancy{
		export
		template <typename Vtx, std::derived_from<uniformed_rect_uv> UV, typename Proj>
		void poly_outlined(
			auto_batch_acquirer<Vtx, UV, Proj>& auto_param,
			const math::trans2 trans,
			int sides,
			const float radius,
			const float outline_stroke = 2.f,
			const col edge_inner_color = colors::white,
			const col edge_outer_color = colors::white,
			const col inner_center_color = colors::white,
			const col inner_edge_color = colors::white
		){
			line::poly(auto_param, trans, sides, radius + outline_stroke / 2.f, outline_stroke, edge_inner_color, edge_outer_color);
			fill::poly(auto_param, trans, sides, radius, inner_center_color, inner_edge_color);
		}

		export
		template <typename M>
		FORCE_INLINE void circle_outlined(
			M& auto_param,
			const vec pos,
			const float radius,
			const float outline_stroke = 2.f,
			const col edge_inner_color = colors::white,
			const col edge_outer_color = colors::white,
			const col inner_center_color = colors::white,
			const col inner_edge_color = colors::white
		){
			fancy::poly_outlined(auto_param, math::trans2{pos, 0}, getCircleVertices(radius), radius,
						outline_stroke,
						edge_inner_color,
			           edge_outer_color,
			           inner_center_color,
			           inner_edge_color);
		}

		export
		template <typename M>
		FORCE_INLINE void  select_rect(
			M& auto_param,
			const math::frect rect,
			const float stroke = 2,
			const col color = colors::white,
			const float cornerSize = 2,
			const bool align = true,
			const float offset = 0.5f
		){
				math::frect inner{};
				math::frect outer{};
				inner = rect;
				outer = rect;

				inner.expand(-offset * stroke);
				outer.expand((1 - offset) * stroke);

				acquirer_guard _{auto_param, 8};
				if(!align){
					fill::quad(auto_param[0],
						outer.vert_00(), inner.vert_00(),
						inner.vert_00().add(0, cornerSize), outer.vert_00().add(0, cornerSize), color
					);
					fill::quad(auto_param[1],
						outer.vert_00(), inner.vert_00(),
						inner.vert_00().add(cornerSize, 0), outer.vert_00().add(cornerSize, 0), color
					);

					fill::quad(auto_param[2],
						outer.vert_10(), inner.vert_10(),
						inner.vert_10().add(0, cornerSize), outer.vert_10().add(0, cornerSize), color
					);
					fill::quad(auto_param[3],
						outer.vert_10(), inner.vert_10(),
						inner.vert_10().add(-cornerSize, 0), outer.vert_10().add(-cornerSize, 0), color
					);

					fill::quad(auto_param[4],
						outer.vert_01(), inner.vert_01(),
						inner.vert_01().add(0, -cornerSize), outer.vert_01().add(0, -cornerSize), color
					);
					fill::quad(auto_param[5],
						outer.vert_01(), inner.vert_01(),
						inner.vert_01().add(cornerSize, 0), outer.vert_01().add(cornerSize, 0), color
					);

					fill::quad(auto_param[6],
						outer.vert_11(), inner.vert_11(),
						inner.vert_11().add(0, -cornerSize), outer.vert_11().add(0, -cornerSize), color
					);
					fill::quad(auto_param[7],
						outer.vert_11(), inner.vert_11(),
						inner.vert_11().add(-cornerSize, 0), outer.vert_11().add(-cornerSize, 0), color
					);
				} else{
					fill::quad(auto_param[0],
						outer.vert_00(), inner.vert_00(),
						vec{inner.vert_00().x, rect.vert_00().y + cornerSize},
						vec{outer.vert_00().x, rect.vert_00().y + cornerSize}, color
					);
					fill::quad(auto_param[1],
						outer.vert_00(), inner.vert_00(),
						vec{rect.vert_00().x + cornerSize, inner.vert_00().y},
						vec{rect.vert_00().x + cornerSize, outer.vert_00().y}, color
					);

					fill::quad(auto_param[2],
						outer.vert_10(), inner.vert_10(),
						vec{inner.vert_10().x, rect.vert_10().y + cornerSize},
						vec{outer.vert_10().x, rect.vert_10().y + cornerSize}, color
					);
					fill::quad(auto_param[3],
						outer.vert_10(), inner.vert_10(),
						vec{rect.vert_10().x - cornerSize, inner.vert_10().y},
						vec{rect.vert_10().x - cornerSize, outer.vert_10().y}, color
					);

					fill::quad(auto_param[4],
						outer.vert_01(), inner.vert_01(),
						vec{inner.vert_01().x, rect.vert_01().y - cornerSize},
						vec{outer.vert_01().x, rect.vert_01().y - cornerSize}, color
					);
					fill::quad(auto_param[5],
						outer.vert_01(), inner.vert_01(),
						vec{rect.vert_01().x + cornerSize, inner.vert_01().y},
						vec{rect.vert_01().x + cornerSize, outer.vert_01().y}, color
					);

					fill::quad(auto_param[6],
						outer.vert_11(), inner.vert_11(),
						vec{inner.vert_11().x, rect.vert_11().y - cornerSize},
						vec{outer.vert_11().x, rect.vert_11().y - cornerSize}, color
					);
					fill::quad(auto_param[7],
						outer.vert_11(), inner.vert_11(),
						vec{rect.vert_11().x - cornerSize, inner.vert_11().y},
						vec{rect.vert_11().x - cornerSize, outer.vert_11().y}, color
					);
				}
			}
	}
}
