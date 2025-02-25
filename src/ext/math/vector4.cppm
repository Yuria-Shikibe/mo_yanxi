module;

#include "../adapted_attributes.hpp"

export module mo_yanxi.math.vector4;

import std;
import mo_yanxi.math;
import mo_yanxi.concepts;
// import Geom.Vector3D;


namespace mo_yanxi::math{
		export
		template <typename T>
		struct vector4{
			T r;
			T g;
			T b;
			T a;

			using value_type = T;
			using floating_point_t = std::conditional_t<std::floating_point<T>, T,
														std::conditional_t<sizeof(T) <= 4, float, double>
			>;
			using const_pass_t = mo_yanxi::conditional_pass_type<const vector4, sizeof(T) * 4>;

			template <std::derived_from<vector4> D>
			[[nodiscard]] FORCE_INLINE friend constexpr D operator+(const D& lhs, const D& rhs) noexcept{
				return D{lhs.r + rhs.r, lhs.g + rhs.g, lhs.b + rhs.b, lhs.a + rhs.a};
			}

			template <std::derived_from<vector4> D>
			[[nodiscard]] FORCE_INLINE friend constexpr D operator-(const D& lhs, const D& rhs) noexcept{
				return D{lhs.r - rhs.r, lhs.g - rhs.g, lhs.b - rhs.b, lhs.a - rhs.a};
			}

			template <std::derived_from<vector4> D>
			[[nodiscard]] FORCE_INLINE friend constexpr D operator*(const D& lhs, const D& rhs) noexcept{
				return D{lhs.r * rhs.r, lhs.g * rhs.g, lhs.b * rhs.b, lhs.a * rhs.a};
			}

			template <std::derived_from<vector4> D>
			[[nodiscard]] FORCE_INLINE friend constexpr D operator/(const D& lhs, const D& rhs) noexcept{
				return D{lhs.r / rhs.r, lhs.g / rhs.g, lhs.b / rhs.b, lhs.a / rhs.a};
			}

			template <std::derived_from<vector4> D>
			[[nodiscard]] FORCE_INLINE friend constexpr vector4 operator%(const D& lhs, const D& rhs) noexcept{
				return {math::mod(lhs.r, rhs.r), math::mod(lhs.g, rhs.g), math::mod(lhs.b, rhs.b), math::mod(lhs.a, rhs.a)};
			}

			template <std::derived_from<vector4> D>
			[[nodiscard]] FORCE_INLINE friend constexpr vector4 operator*(const D& lhs, const T val) noexcept{
				return {lhs.r * val, lhs.g * val, lhs.b * val, lhs.a * val};
			}


			template <std::derived_from<vector4> D>
			[[nodiscard]] FORCE_INLINE friend constexpr vector4 operator/(const D& lhs, const T val) noexcept{
				return {lhs.r / val, lhs.g / val, lhs.b / val, lhs.a / val};
			}

			template <std::derived_from<vector4> D>
			[[nodiscard]] FORCE_INLINE friend constexpr vector4 operator%(const D& lhs, const T val) noexcept{
				return {math::mod(lhs.r, val), math::mod(lhs.g, val), math::mod(lhs.b, val), math::mod(lhs.a, val)};
			}


			template <std::derived_from<vector4> D>
			FORCE_INLINE friend constexpr vector4& operator+=(const D& lhs, const D& rhs) noexcept{
				lhs.r += rhs.r;
				lhs.g += rhs.g;
				lhs.b += rhs.b;
				lhs.a += rhs.a;
				return lhs;
			}

			template <std::derived_from<vector4> D>
			FORCE_INLINE friend constexpr vector4& operator-=(const D& lhs, const D& rhs) noexcept{
				lhs.r -= rhs.r;
				lhs.g -= rhs.g;
				lhs.b -= rhs.b;
				lhs.a -= rhs.a;
				return lhs;
			}

			template <std::derived_from<vector4> D>
			FORCE_INLINE friend constexpr vector4& operator*=(const D& lhs, const D& rhs) noexcept{
				lhs.r *= rhs.r;
				lhs.g *= rhs.g;
				lhs.b *= rhs.b;
				lhs.a *= rhs.a;
				return lhs;
			}

			template <std::derived_from<vector4> D>
			FORCE_INLINE friend constexpr vector4& operator/=(const D& lhs, const D& rhs) noexcept{
				lhs.r /= rhs.r;
				lhs.g /= rhs.g;
				lhs.b /= rhs.b;
				lhs.a /= rhs.a;
				return lhs;
			}

			template <std::derived_from<vector4> D>
			FORCE_INLINE friend constexpr vector4& operator%=(const D& lhs, const D& rhs) noexcept{
				lhs.r = math::mod(lhs.r, rhs.r);
				lhs.g = math::mod(lhs.g, rhs.g);
				lhs.b = math::mod(lhs.b, rhs.b);
				lhs.a = math::mod(lhs.a, rhs.a);
				return lhs;
			}

			template <std::derived_from<vector4> D>
			FORCE_INLINE friend constexpr vector4& operator*=(const D& lhs, const T v) noexcept{
				lhs.r *= v;
				lhs.g *= v;
				lhs.b *= v;
				lhs.a *= v;
				return lhs;
			}

			template <std::derived_from<vector4> D>
			FORCE_INLINE friend constexpr vector4& operator/=(const D& lhs, const T v) noexcept{
				lhs.r /= v;
				lhs.g /= v;
				lhs.b /= v;
				lhs.a /= v;
				return lhs;
			}

			template <std::derived_from<vector4> D>
			FORCE_INLINE friend constexpr vector4& operator%=(const D& lhs, const T v) noexcept{
				lhs.r = math::mod(r, v);
				lhs.g = math::mod(g, v);
				lhs.b = math::mod(b, v);
				lhs.a = math::mod(a, v);
				return lhs;
			}

			FORCE_INLINE constexpr vector4& set(const T ox, const T oy, const T oz, const T ow) noexcept{
				this->r = ox;
				this->g = oy;
				this->b = oz;
				this->a = ow;

				return *this;
			}

			FORCE_INLINE constexpr vector4& set(const T val) noexcept{
				return this->set(val, val, val, val);
			}

			FORCE_INLINE constexpr vector4& set(const_pass_t tgt) noexcept{
				return this->operator=(tgt);
			}

			FORCE_INLINE constexpr vector4& lerp(const_pass_t tgt, const floating_point_t alpha) noexcept{
				return *this = math::lerp(*this, tgt, alpha);
			}

			constexpr vector4& clamp() noexcept{
				r = math::clamp(r);
				g = math::clamp(g);
				b = math::clamp(b);
				a = math::clamp(a);

				return *this;
			}

			constexpr friend bool operator==(const vector4& lhs, const vector4& rhs) noexcept = default;
		};
	}

