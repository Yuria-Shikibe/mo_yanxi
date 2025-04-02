module;

#include "../src/ext/adapted_attributes.hpp"

export module mo_yanxi.game.graphic.effect;

import :gen_func;

export import mo_yanxi.math.vector2;
export import mo_yanxi.math.trans2;
export import mo_yanxi.math.rect_ortho;
export import mo_yanxi.math.timed;
export import mo_yanxi.math.interpolation;
export import mo_yanxi.graphic.color;
export import mo_yanxi.graphic.image_region;
import std;

namespace mo_yanxi::graphic{
	export struct renderer_world;
}


namespace mo_yanxi::game::fx{


	export
	template <typename T>
	struct mix_func{
		FORCE_INLINE constexpr static T operator()(const T& src, const T& dst, float interp) noexcept{
			return src * (1 - interp) + dst * interp;
		}
	};

	export
	template <>
	struct mix_func<float>{
		FORCE_INLINE constexpr static auto operator()(const float src, const float dst, float interp) noexcept{
			return math::lerp(src, dst, interp);
		}
	};

	template <typename T>
	struct mix_func<math::vector2<T>>{
		FORCE_INLINE constexpr static auto operator(
		)(math::vector2<T> src, math::vector2<T> dst, float interp) noexcept{
			return src.scl(1 - interp).fma(dst, interp);
		}
	};

	export
	struct interp{
		math::interp::no_state_interp_func func{};

		constexpr float operator()(float prog) const noexcept{
			if(!func)return prog;
			return func(prog);
		}

		explicit constexpr operator bool() const noexcept{
			return func != nullptr;
		}
	};

	export
	template <typename T>
	struct gradient{
		T src;
		T dst;
		interp interp{};

		constexpr T operator[](float prog) const noexcept{
			return mix_func<T>::operator()(src, dst, interp(prog));
		}
	};

	export
	template <typename T>
	struct trivial_gradient{
		T src;
		T dst;

		constexpr T operator[](float prog) const noexcept{
			return mix_func<T>::operator()(src, dst, prog);
		}
	};

	export namespace pal{
		template <std::size_t Sz>
			requires (Sz > 0)
		struct standalone_colors{
			std::array<graphic::color, Sz> colors;

			constexpr graphic::color operator [](std::size_t idx, float) const noexcept{
				return colors[idx % Sz];
			}
		};

		template <std::size_t Sz>
			requires (Sz > 0)
		struct trivial_gradient_colors{
			std::array<trivial_gradient<graphic::color>, Sz> colors;

			constexpr graphic::color operator [](std::size_t idx, float lerp) const noexcept{
				return colors[idx % Sz][lerp];
			}
		};

		template <std::size_t Sz>
		trivial_gradient_colors(const trivial_gradient<graphic::color>(&)[Sz]) -> trivial_gradient_colors<Sz>;

		template <std::size_t Sz>
			requires (Sz > 0)
		struct gradient_colors{
			std::array<gradient<graphic::color>, Sz> colors;

			constexpr graphic::color operator [](std::size_t idx, float lerp) const noexcept{
				return colors[idx % Sz][lerp];
			}
		};

		template <std::size_t Sz>
		gradient_colors(const gradient<graphic::color>(&)[Sz]) -> gradient_colors<Sz>;

		template <std::size_t Sz>
			requires (Sz > 0)
		struct color_wheel{
			std::array<graphic::color, Sz> colors;

			constexpr graphic::color operator [](std::size_t idx, float lerp) const noexcept{
				return graphic::color::from_lerp_span(lerp, colors);
			}
		};
	}

	export
	struct effect_draw_context{
		graphic::renderer_world& renderer;
		math::frect viewport;


		[[nodiscard]] FORCE_INLINE graphic::combined_image_region<graphic::uniformed_rect_uv>
		get_default_image_region() const noexcept;
	};

	struct palette{
		std::variant<
			graphic::color,
			pal::standalone_colors<2>,
			pal::standalone_colors<3>,
			pal::standalone_colors<4>,
			pal::trivial_gradient_colors<1>,
			pal::trivial_gradient_colors<2>,
			pal::trivial_gradient_colors<3>,
			pal::gradient_colors<1>,
			pal::gradient_colors<2>,
			pal::color_wheel<3>,
			pal::color_wheel<4>,
			pal::color_wheel<5>
		> pal;

		FORCE_INLINE graphic::color operator[](std::size_t idx, float lerp) const noexcept{
			return std::visit([idx, lerp] <typename T>(const T& pal){
				if constexpr (std::same_as<graphic::color, T>){
					return pal;
				}else{
					return pal[idx, lerp];
				}
			}, pal);
		}

		FORCE_INLINE graphic::color operator[](float lerp) const noexcept{
			return this->operator[](0, lerp);
		}
	};

	export struct effect;

	export
	struct effect_drawer_base{
		void operator()(const effect&, const effect_draw_context&) const noexcept = delete;

		[[nodiscard]] math::frect get_clip_region(const effect& e, float min_clip_radius) const noexcept = delete;

	protected:
		template <typename T>
			requires requires (const T& t){
				{ t[float{}] } -> std::convertible_to<float>;
			}
		[[nodiscard]] FORCE_INLINE static math::frect get_clip_region_by_radius(const effect& e, float min_clip_radius, const T& radius, float margin = 20) noexcept{
			const auto prog = get_effect_prog(e);
			const auto clipSize = math::max(min_clip_radius, radius[prog] + margin);

			return math::frect{get_effect_pos(e), clipSize};
		}

	private:
		static FORCE_INLINE float get_effect_prog(const effect& e) noexcept;
		static FORCE_INLINE math::vec2 get_effect_pos(const effect& e) noexcept;
	};

	export
	struct shape_rect_ortho : effect_drawer_base{
		gradient<math::vec2> size;
		gradient<math::vec2> offset;
		gradient<float> stroke;

		[[nodiscard]] constexpr math::frect get_rect(const math::vec2 pos, const float interp) const noexcept{
			const auto off = offset[interp] + pos;
			const auto sz = size[interp];
			const auto stk = stroke[interp] / 2.f;
			return math::frect{off, sz.x, sz.y}.expand(stk);
		}

		void operator()(const effect&, const effect_draw_context&) const noexcept;

		[[nodiscard]] math::frect get_clip_region(const effect&, float) const noexcept;
	};

	export
	struct line_splash : effect_drawer_base{
		unsigned count{12};
		float base_range{32};
		float distribute_angle{180};
		math::range range{};
		math::based_section<gradient<float>> stroke{};
		math::based_section<gradient<float>> length{};
		interp radius_interp{math::interp::pow3Out};

		void operator()(const effect& e, const effect_draw_context& ctx) const noexcept;

		[[nodiscard]] math::frect get_clip_region(const effect& e, const float min_clip) const noexcept{
			return get_clip_region_by_radius(e, min_clip, range, base_range + 15);
		}
	};

	export
	struct poly_line_out : effect_drawer_base{
		int sides{};
		gradient<float> radius{};
		gradient<float> stroke{};

		[[nodiscard]] constexpr bool is_circle() const noexcept{
			return sides == 0;
		}

		void operator()(const effect& e, const effect_draw_context& ctx) const noexcept;

		[[nodiscard]] math::frect get_clip_region(const effect& e, float min_clip) const noexcept{
			return get_clip_region_by_radius(e, min_clip, radius);
		}
	};


	export
	struct dynamic_drawer : effect_drawer_base{
		void operator()(const effect&, const effect_draw_context&) const noexcept{
		}
	};

	using styles = std::variant<
		dynamic_drawer,
		shape_rect_ortho,
		line_splash
	>;

	// struct

	export
	using effect_id_type = std::size_t;

	export
	struct effect_data{
		styles style{};

		math::trans2 trans{};
		float depth{};
		math::timed duration{};
		float min_clip_radius{50};
		palette palette{graphic::colors::white};
	};

	export
	struct effect : effect_data{
	private:
		static inline std::atomic_size_t id_counter{};

		effect_id_type id_{};
		bool referenced{};

	public:
		static effect_id_type acquire_id() noexcept{
			return ++id_counter;
		}

		effect& set_data(effect_data&& data) noexcept{
			this->effect_data::operator=(std::move(data));
			return *this;
		}

		effect& set_data(const effect_data& data) noexcept(std::is_nothrow_move_assignable_v<effect_data>){
			this->effect_data::operator=(data);
			return *this;
		}

		[[nodiscard]] effect() = default;

		[[nodiscard]] explicit effect(effect_id_type id)
			: id_(id){
		}

		[[nodiscard]] effect_id_type id() const noexcept{
			return id_;
		}

		/**
		 * @return is erasable
		 */
		bool update(float delta_in_tick) noexcept{
			return duration.update_and_get(delta_in_tick) && !referenced;
		}

		void draw(const effect_draw_context& draw_context) const noexcept{
			std::visit([&, this] <typename S>(const S& s){
				s(*this, draw_context);
			}, style);
		}

		void try_draw(const effect_draw_context& draw_context) const noexcept{
			if(get_clip_region().overlap_exclusive(draw_context.viewport)){
				draw(draw_context);
			}
		}

		[[nodiscard]] math::frect get_clip_region() const noexcept{
			return std::visit([this] <typename S>(const S& s) -> math::frect{
				if constexpr(requires(const effect& effect){
					{ s.get_clip_region(effect, float{0}) } -> std::convertible_to<math::frect>;
				}){
					return s.get_clip_region(*this, min_clip_radius);
				} else{
					return math::frect{trans.vec, min_clip_radius};
				}
			}, style);
		}
	};
}
