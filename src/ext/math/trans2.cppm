module;

#include "../adapted_attributes.hpp"

export module mo_yanxi.math.trans2;

export import mo_yanxi.math.vector2;
export import mo_yanxi.math;
export import mo_yanxi.math.angle;

import mo_yanxi.concepts;
import std;

export namespace mo_yanxi::math{
	template <typename AngTy>
	struct transform2 {
		using angle_t = AngTy;
		using vec_t = vec2;

		vec_t vec;
		angle_t rot;

		FORCE_INLINE constexpr void set_zero(){
			vec.set_zero();
			rot = 0.0f;
		}

		// template <float NaN = std::numeric_limits<angle_t>::signaling_NaN()>
		// FORCE_INLINE constexpr void set_nan() noexcept requires std::is_floating_point_v<angle_t>{
		// 	vec.set(NaN, NaN);
		// 	// rot = NaN;
		// }

		[[nodiscard]] FORCE_INLINE constexpr bool is_NaN() const noexcept {
			return vec.is_NaN() || std::isnan(rot);
		}

		template <typename Ang>
		FORCE_INLINE constexpr transform2& apply_inv(const transform2<Ang>& debaseTarget) noexcept{
			vec.sub(debaseTarget.vec).rotate_rad(-static_cast<float>(debaseTarget.rot));
			rot -= debaseTarget.rot;

			return *this;
		}

		FORCE_INLINE constexpr transform2& apply(const transform2 rebaseTarget) noexcept{
			vec.rotate_rad(static_cast<float>(rebaseTarget.rot)).add(rebaseTarget.vec);
			rot += rebaseTarget.rot;

			return *this;
		}

		[[nodiscard]] FORCE_INLINE constexpr vec_t apply_inv_to(vec_t debaseTarget) const noexcept{
			debaseTarget.sub(vec).rotate_rad(-static_cast<float>(rot));
			return debaseTarget;
		}

		template <typename Ang>
		[[nodiscard]] FORCE_INLINE constexpr transform2<Ang> apply_inv_to(transform2<Ang> debaseTarget) const noexcept{
			debaseTarget.apply_inv(*this);
			return debaseTarget;
		}

		[[nodiscard]] FORCE_INLINE constexpr vec_t apply_to(vec_t target) const noexcept{
			target.rotate_rad(static_cast<float>(rot)).add(vec);
			return target;
		}

		FORCE_INLINE constexpr transform2& operator|=(const transform2 parentRef) noexcept{
			return transform2::apply(parentRef);
		}

		FORCE_INLINE constexpr transform2& operator+=(const transform2 other) noexcept{
			vec += other.vec;
			rot += other.rot;

			return *this;
		}

		FORCE_INLINE constexpr transform2& operator-=(const transform2 other) noexcept{
			vec -= other.vec;
			rot -= other.rot;

			return *this;
		}

		FORCE_INLINE constexpr transform2& operator*=(const float scl) noexcept{
			vec *= scl;
			rot *= scl;

			return *this;
		}

		[[nodiscard]] FORCE_INLINE constexpr friend transform2 operator*(transform2 self, const float scl) noexcept{
			return self *= scl;
		}

		[[nodiscard]] FORCE_INLINE constexpr friend transform2 operator+(transform2 self, const transform2 other) noexcept{
			return self += other;
		}

		[[nodiscard]] FORCE_INLINE constexpr friend transform2 operator-(transform2 self, const transform2 other) noexcept{
			return self -= other;
		}

		[[nodiscard]] FORCE_INLINE constexpr friend transform2 operator-(transform2 self) noexcept{
			self.vec.reverse();
			self.rot = -self.rot;
			return self;
		}

		template <typename T>
			requires (std::convertible_to<angle_t, T>)
		FORCE_INLINE constexpr explicit(false) operator transform2<T>() const noexcept{
			return transform2<T>{vec, T(rot)};
		}

		/**
		 * @brief Local To Parent
		 * @param self To Trans
		 * @param parentRef Parent Frame Reference Trans
		 * @return Transformed Translation
		 */
		[[nodiscard]] FORCE_INLINE constexpr friend transform2 operator|(transform2 self, const transform2 parentRef) noexcept{
			return self |= parentRef;
		}

		[[nodiscard]] FORCE_INLINE constexpr friend vec_t& operator|=(vec_t& vec, const transform2 transform) noexcept{
			return vec.rotate_rad(static_cast<float>(transform.rot)).add(transform.vec);
		}

		[[nodiscard]] FORCE_INLINE constexpr friend vec_t operator|(vec_t vec, const transform2 transform) noexcept{
			return vec |= transform;
		}

		friend bool operator==(const transform2& lhs, const transform2& rhs) noexcept = default;

		[[nodiscard]] constexpr math::cos_sin_result rot_cos_sin() const noexcept{
			return math::cos_sin(rot);
		}
	};

	using trans2 = transform2<float>;
	using uniform_trans2 = transform2<angle>;
}
