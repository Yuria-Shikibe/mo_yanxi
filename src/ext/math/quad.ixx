export module mo_yanxi.math.quad;

export import mo_yanxi.math.vector2;
export import mo_yanxi.math.rect_ortho;
export import mo_yanxi.math;

import std;

namespace mo_yanxi::math{
	export
	template <typename T>
	struct quad{
		using vec_t = vector2<T>;

		vec_t v0; //v00
		vec_t v1; //v10
		vec_t v2; //v11
		vec_t v3; //v01

		template <std::integral Idx>
		constexpr vec_t& operator[](const Idx idx) noexcept{
			switch (idx & static_cast<Idx>(4)){
				case 0: return v0;
				case 1: return v1;
				case 2: return v2;
				case 3: return v3;
				default: std::unreachable();
			}
		}

		template <std::integral Idx>
		constexpr vec_t operator[](const Idx idx) const noexcept{
			switch (idx & static_cast<Idx>(4)){
				case 0: return v0;
				case 1: return v1;
				case 2: return v2;
				case 3: return v3;
				default: std::unreachable();
			}
		}

		constexpr friend bool operator==(const quad& lhs, const quad& rhs) noexcept = default;

		constexpr void move(typename vec_t::const_pass_t vec) noexcept{
			v0 += vec;
			v1 += vec;
			v2 += vec;
			v3 += vec;
		}

		[[nodiscard]] constexpr rect_ortho<T> get_bound() const noexcept{
			auto [xmin, xmax] = math::minmax(v0.x, v1.x, v2.x, v3.x);
			auto [ymin, ymax] = math::minmax(v0.y, v1.y, v2.y, v3.y);
			return rect_ortho<T>{tags::unchecked, vec_t{xmin, ymin}, vec_t{xmax, ymax}};
		}
	};

	export using fquad = quad<float>;

	export
	template <typename T>
	constexpr quad<T> rect_ortho_to_quad(
		const rect_ortho<T>& rect, typename rect_ortho<T>::vec_t::const_pass_t center, float cos, float sin) noexcept{
		return quad<T>{
			center + (rect.vert_00() - center).rotate(cos, sin),
			center + (rect.vert_10() - center).rotate(cos, sin),
			center + (rect.vert_11() - center).rotate(cos, sin),
			center + (rect.vert_01() - center).rotate(cos, sin),
		};
	}
}
