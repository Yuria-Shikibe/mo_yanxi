module;

#include <cassert>
#include "../adapted_attributes.hpp"
#define MATH_ATTR [[nodiscard]] FORCE_INLINE

export module mo_yanxi.math.angle;

import mo_yanxi.math;
import mo_yanxi.tags;
import std;

#define CONSTEXPR_26 /*constexpr*/

namespace mo_yanxi::math{

	export
	template <typename T>
	struct backward_forward_ang{
		T forward;
		T backward;
	};

	/** @brief Uniformed Angle in [-pi, pi) radians*/
	export
	template <std::floating_point T>
	struct uniformed_angle{
		using value_type = T;
	private:
		static constexpr value_type DefMargin = 8 / static_cast<value_type>(std::numeric_limits<std::uint16_t>::max());
		static constexpr value_type Pi = std::numbers::pi_v<value_type>;
		static constexpr value_type Pi_mod = [](){
			if constexpr (std::same_as<value_type, float>){
				return std::bit_cast<float>(std::bit_cast<std::int32_t>(std::numbers::pi_v<float>) + 1);
			}else{
				return std::bit_cast<double>(std::bit_cast<std::int64_t>(std::numbers::pi_v<double>) + 1);
			}
		}();
		// static constexpr value_type Bound = std::numbers::pi_v<value_type>;

		static constexpr value_type half_cycles = std::numbers::pi_v<value_type>;
		static constexpr value_type cycles = half_cycles * 2.;
		// static constexpr float HALF_VAL = Bound;


	private:
		value_type ang_{};

		/**
		 * @return Angle in [0, 360] degree
		 */
		MATH_ATTR static value_type getAngleInPi2(value_type rad) noexcept{
			rad = std::fmod(rad, cycles);
			if(rad < 0) rad += cycles;
			return rad;
		}

		/**
		 * @return Angle in [-180, 180] degree
		 */
		MATH_ATTR static value_type getAngleInPi(value_type a) noexcept{
			return std::fmod(a, Pi_mod);
		}

		MATH_ATTR CONSTEXPR_26 void clampInternal() noexcept{
			ang_ = uniformed_angle::getAngleInPi(ang_);
		}

		MATH_ATTR static constexpr value_type simpleClamp(value_type val) noexcept{
			//val should between [-540 deg, 540 deg]
			CHECKED_ASSUME(val >= -half_cycles * 3);
			CHECKED_ASSUME(val <= half_cycles * 3);

			if(val >= half_cycles){
				return val - cycles;
			}

			if(val < -half_cycles){
				return val + cycles;
			}

			return val;
		}

		MATH_ATTR constexpr void simpleClamp() noexcept{
			ang_ = uniformed_angle::simpleClamp(ang_);
		}

	public:
		[[nodiscard]] constexpr uniformed_angle() noexcept = default;

		[[nodiscard]] CONSTEXPR_26 uniformed_angle(tags::from_deg_t, const value_type deg) noexcept : ang_(deg * deg_to_rad_v<value_type>){
			clampInternal();
		}

		[[nodiscard]] CONSTEXPR_26 explicit(false) uniformed_angle(tags::from_rad_t, const value_type rad) noexcept : ang_(rad){
			clampInternal();
		}

		[[nodiscard]] CONSTEXPR_26 explicit(false) uniformed_angle(const value_type rad) noexcept : ang_(rad){
			clampInternal();
		}

		[[nodiscard]] constexpr uniformed_angle(tags::unchecked_t, const value_type rad) noexcept : ang_(rad){
			assert(rad >= -Pi);
			assert(rad <= Pi);
		}


		FORCE_INLINE constexpr void clamp(const value_type min, const value_type max) noexcept{
			ang_ = math::clamp(ang_, min, max);
		}

		FORCE_INLINE constexpr void clamp(const value_type maxabs) noexcept{
			assert(maxabs >= 0.f);
			ang_ = math::clamp(ang_, -maxabs, maxabs);
		}

		constexpr friend bool operator==(const uniformed_angle& lhs, const uniformed_angle& rhs) noexcept = default;
		constexpr auto operator<=>(const uniformed_angle&) const noexcept = default;

		FORCE_INLINE constexpr uniformed_angle operator-() const noexcept{
			uniformed_angle rst{*this};
			rst.ang_ = -rst.ang_;
			return rst;
		}

		FORCE_INLINE constexpr uniformed_angle& operator+=(const uniformed_angle other) noexcept{
			ang_ += other.ang_;

			simpleClamp();

			return *this;
		}

		FORCE_INLINE constexpr uniformed_angle& operator-=(const uniformed_angle other) noexcept{
			ang_ -= other.ang_;

			simpleClamp();

			return *this;
		}

		FORCE_INLINE CONSTEXPR_26 uniformed_angle& operator*=(const value_type val) noexcept{
			ang_ *= val;

			if(val < -1 || val > 1){
				clampInternal();
			}

			return *this;
		}

		FORCE_INLINE CONSTEXPR_26 uniformed_angle& operator/=(const value_type val) noexcept{
			ang_ /= val;

			if(val > -1 || val < 1){
				clampInternal();
			}

			return *this;
		}

		FORCE_INLINE CONSTEXPR_26 uniformed_angle& operator%=(const value_type val) noexcept{
			ang_ = std::fmod(ang_, val);

			return *this;
		}

		FORCE_INLINE constexpr friend uniformed_angle operator+(uniformed_angle lhs, const uniformed_angle rhs) noexcept{
			return lhs += rhs;
		}

		FORCE_INLINE constexpr friend uniformed_angle operator-(uniformed_angle lhs, const uniformed_angle rhs) noexcept{
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
			return ang_;
		}

		template <std::floating_point Ty>
		FORCE_INLINE constexpr explicit(false) operator uniformed_angle<Ty>() const noexcept{
			return uniformed_angle{tags::unchecked, ang_};
		}

		MATH_ATTR constexpr value_type radians() const noexcept{
			return ang_;
		}

		MATH_ATTR constexpr value_type degrees() const noexcept{
			return ang_ * rad_to_deg_v<value_type>;
		}

		MATH_ATTR constexpr bool equals(const uniformed_angle other, const value_type margin) const noexcept{
			return math::abs(static_cast<value_type>(*this - other)) < margin;
		}

		MATH_ATTR constexpr bool equals(tags::unchecked_t, const value_type other,
		                                    const value_type margin) const noexcept{
			return math::abs(uniformed_angle::simpleClamp(static_cast<value_type>(*this) - other)) < margin;
		}

		MATH_ATTR backward_forward_ang<value_type> forward_backward_ang(const uniformed_angle other) const noexcept{
			auto abs = math::abs(ang_ - other.ang_);
			return {abs, cycles - abs};
		}

		/**
		 * @brief move or set to another angle
		 * @return true if equals to the target angle after rotating
		 */
		FORCE_INLINE /*constexpr*/ bool rotate_toward(
			const uniformed_angle target,
			const value_type speed
		) noexcept{
			const auto dst = static_cast<value_type>(target - *this);

			if(uniformed_angle::equals(target, speed)){
				*this = target;
				return true;
			} else{
				auto [forward, backward] = uniformed_angle::forward_backward_ang(target);

				if((ang_ > target.ang_) == (backward > forward)){
					ang_ -= speed;
				} else{
					ang_ += speed;
				}

				simpleClamp();

				return false;
			}
		}

		FORCE_INLINE constexpr void slerp(const uniformed_angle target, const value_type progress) noexcept{
			const value_type delta = target - *this;
			ang_ += delta * progress;

			simpleClamp();
		}
	};

	export using angle = uniformed_angle<float>;


	export
	uniformed_angle<long double> operator"" _deg_to_uang(const long double val){
		return {tags::from_deg, val};
	}

	export
	uniformed_angle<long double> operator"" _rad_to_uang(const long double val){
		return {tags::from_rad, val};
	}

}


