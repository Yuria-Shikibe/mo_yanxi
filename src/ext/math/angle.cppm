module;

#include <cassert>
#include "../adapted_attributes.hpp"
#define MATH_ATTR [[nodiscard]] FORCE_INLINE

export module mo_yanxi.math.angle;

import mo_yanxi.tags;
import std;

#define CONSTEXPR_26 /*constexpr*/

namespace mo_yanxi::math{
	constexpr float HALF_VAL = 180.f;

	export
	template <typename T>
	struct backward_forward_ang{
		T forward;
		T backward;
	};

	/** @brief Uniformed Angle in [-180, 180] degree*/
	export
	template <std::floating_point T>
	struct uniformed_angle{
		using value_type = T;
		static constexpr value_type DefMargin = 8 / static_cast<value_type>(std::numeric_limits<std::uint16_t>::max());
		static constexpr value_type Pi = std::numbers::pi_v<value_type>;
		static constexpr value_type Bound = HALF_VAL;
		static constexpr value_type DegHalf = static_cast<value_type>(180);
		static constexpr value_type DegToRad = Pi / DegHalf;
		static constexpr value_type RadToDeg = DegHalf / Pi;

	private:
		value_type deg{};

		/**
		 * @return Angle in [0, 360] degree
		 */
		MATH_ATTR static value_type getAngleInPi2(value_type rad) noexcept{
			rad = std::fmod(rad, HALF_VAL * 2.f);
			if(rad < 0) rad += HALF_VAL * 2.f;
			return rad;
		}

		/**
		 * @return Angle in [-180, 180] degree
		 */
		MATH_ATTR static value_type getAngleInPi(value_type a) noexcept{
			a = uniformed_angle::getAngleInPi2(a);
			return a > HALF_VAL ? a - HALF_VAL * 2.f : a;
		}

		MATH_ATTR CONSTEXPR_26 void clampInternal() noexcept{
			deg = uniformed_angle::getAngleInPi(deg);
		}

		MATH_ATTR static constexpr value_type simpleClamp(value_type val) noexcept{
			//val should between [-540 deg, 540 deg]
			CHECKED_ASSUME(val >= -Bound * 3);
			CHECKED_ASSUME(val <= Bound * 3);
			if(val >= Bound){
				return val - Bound * 2;
			}

			if(val <= -Bound){
				return val + Bound * 2;
			}

			return val;
		}

		MATH_ATTR constexpr void simpleClamp() noexcept{
			deg = uniformed_angle::simpleClamp(deg);
		}

	public:
		[[nodiscard]] constexpr uniformed_angle() noexcept = default;

		[[nodiscard]] CONSTEXPR_26 uniformed_angle(tags::from_rad_t, const value_type rad) noexcept : deg(rad * RadToDeg){
			clampInternal();
		}

		[[nodiscard]] constexpr uniformed_angle(tags::unchecked_t, const value_type deg) noexcept : deg(deg){
			assert(deg >= -DegHalf);
			assert(deg <= DegHalf);
		}

		[[nodiscard]] CONSTEXPR_26 explicit(false) uniformed_angle(const value_type deg) noexcept : deg(deg){
			clampInternal();
		}

		FORCE_INLINE constexpr void clamp(const value_type min, const value_type max) noexcept{
			deg = std::clamp(deg, min, max);
		}

		FORCE_INLINE constexpr void clamp(const value_type maxabs) noexcept{
			assert(maxabs >= 0.f);
			deg = std::clamp(deg, -maxabs, maxabs);
		}

		constexpr friend bool operator==(const uniformed_angle& lhs, const uniformed_angle& rhs) noexcept = default;
		constexpr auto operator<=>(const uniformed_angle&) const noexcept = default;

		FORCE_INLINE constexpr uniformed_angle operator-() const noexcept{
			uniformed_angle rst{*this};
			rst.deg = -rst.deg;
			return rst;
		}

		FORCE_INLINE constexpr uniformed_angle& operator+=(const uniformed_angle other) noexcept{
			deg += other.deg;

			simpleClamp();

			return *this;
		}

		FORCE_INLINE constexpr uniformed_angle& operator-=(const uniformed_angle other) noexcept{
			deg -= other.deg;

			simpleClamp();

			return *this;
		}

		FORCE_INLINE CONSTEXPR_26 uniformed_angle& operator*=(const value_type val) noexcept{
			deg *= val;

			if(val < -1 || val > 1){
				clampInternal();
			}

			return *this;
		}

		FORCE_INLINE CONSTEXPR_26 uniformed_angle& operator/=(const value_type val) noexcept{
			deg /= val;

			if(val > -1 || val < 1){
				clampInternal();
			}

			return *this;
		}

		FORCE_INLINE CONSTEXPR_26 uniformed_angle& operator%=(const value_type val) noexcept{
			deg = std::fmod(deg, val);

			return *this;
		}

		FORCE_INLINE constexpr friend uniformed_angle operator
		+(uniformed_angle lhs, const uniformed_angle rhs) noexcept{
			return lhs += rhs;
		}

		FORCE_INLINE constexpr friend uniformed_angle operator
		-(uniformed_angle lhs, const uniformed_angle rhs) noexcept{
			return lhs -= rhs;
		}

		FORCE_INLINE CONSTEXPR_26 friend uniformed_angle operator*(uniformed_angle lhs, const value_type rhs) noexcept{
			return lhs *= rhs;
		}

		FORCE_INLINE CONSTEXPR_26 friend uniformed_angle operator/(uniformed_angle lhs, const value_type rhs) noexcept{
			return lhs /= rhs;
		}

		FORCE_INLINE CONSTEXPR_26 friend uniformed_angle operator%(uniformed_angle lhs, const value_type rhs) noexcept{
			return lhs %= rhs;
		}

		FORCE_INLINE constexpr explicit(false) operator value_type() const noexcept{
			return deg;
		}

		MATH_ATTR constexpr value_type radians() const noexcept{
			return deg * DegToRad;
		}

		MATH_ATTR constexpr value_type degrees() const noexcept{
			return deg;
		}

		MATH_ATTR /*constexpr*/ bool equals(const uniformed_angle other, const value_type margin) const noexcept{
			return std::abs(static_cast<value_type>(*this - other)) < margin;
		}

		MATH_ATTR /*constexpr*/ bool equals(tags::unchecked_t, const value_type other,
		                                    const value_type margin) const noexcept{
			return std::abs(uniformed_angle::simpleClamp(static_cast<value_type>(*this) - other)) < margin;
		}

		MATH_ATTR backward_forward_ang<value_type> forward_backward_ang(const uniformed_angle other) const noexcept{
			auto abs = std::abs(deg - other.deg);
			return {abs, DegHalf * 2 - abs};
		}

		/**
		 * @brief move or set to another angle
		 * @return true if equals to the target angle after rotating
		 */
		MATH_ATTR /*constexpr*/ bool rotate_toward(
			const uniformed_angle target,
			const value_type speed
		) noexcept{
			const auto dst = static_cast<value_type>(target - *this);

			if(uniformed_angle::equals(target, speed)){
				*this = target;
				return true;
			} else{
				auto [forward, backward] = uniformed_angle::forward_backward_ang(target);

				if((deg > target.deg) == (backward > forward)){
					deg -= speed;
				} else{
					deg += speed;
				}

				simpleClamp();

				return false;
			}
		}

		FORCE_INLINE constexpr void slerp(const uniformed_angle target, const value_type progress) noexcept{
			const value_type delta = target - *this;
			deg += delta * progress;

			simpleClamp();
		}
	};

	export using angle = uniformed_angle<float>;
}
