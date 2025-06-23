module;

#include "../src/ext/adapted_attributes.hpp"
#include <cassert>

export module mo_yanxi.math.constrained_system;

export import mo_yanxi.math;
import std;

namespace mo_yanxi::math::constrain_resolve{
	template <typename T, typename V>
	FORCE_INLINE [[nodiscard]] constexpr T get_normalized(const T& val, const V abs) noexcept{
		if constexpr (std::is_arithmetic_v<T>){
			if consteval{
				return T{val == 0 ? 0 : val > 0 ? 1 : -1};
			}else{
				return T{val == 0 ? 0 : std::copysign(val, 1)};
			}
		}else{
			if(math::zero(abs))return T{};

			return val * (1 / abs);
		}
	}

	export
	template <typename T, std::floating_point M>
	constexpr T smooth_approach_lerp(
		const T& approach,
		const T& initial_first_derivative,
		const M max_second_derivative,
		const M margin,
		const M yaw_correction_factor = M{0.5}
		) noexcept{

		assert(max_second_derivative > 0);
		assert(margin > 0);

		const auto idvLen = math::sqr(initial_first_derivative);
		const auto idvAbs = [&]{
			if constexpr (std::is_arithmetic_v<T>){
				return math::abs(initial_first_derivative);
			}else{
				return math::sqrt(idvLen);
			}
		}();

		const auto dst = math::abs(approach);
		const auto overhead = idvLen / (2 * max_second_derivative);
		if(math::zero(idvAbs)){
			if(dst <= margin){
				return T{};
			}
		}else{
			if(dst <= overhead + margin){
				return initial_first_derivative * (-max_second_derivative / idvAbs);
			}
		}


		const auto instruction = math::lerp(approach, constrain_resolve::get_normalized(initial_first_derivative, idvAbs / -overhead), yaw_correction_factor);
		return math::get_normalized(instruction, max_second_derivative);
	}

	export
	template <typename T, std::floating_point M>
	constexpr T smooth_approach(
		const T& approach,
		const T& initial_first_derivative,
		const M max_second_derivative,
		const M margin
		) noexcept{

		assert(max_second_derivative > 0);
		assert(margin > 0);

		const auto idvLen = math::sqr(initial_first_derivative);
		const auto idvAbs = [&]{
			if constexpr (std::is_arithmetic_v<T>){
				return math::abs(initial_first_derivative);
			}else{
				return math::sqrt(idvLen);
			}
		}();

		const auto dst = math::abs(approach);
		const auto overhead = idvLen / (2 * max_second_derivative);
		if(math::zero(idvAbs)){
			if(dst <= margin){
				return T{};
			}
		}else{
			if(dst <= overhead + margin){
				return initial_first_derivative * (-max_second_derivative / idvAbs);
			}
		}

		const auto instruction = approach + constrain_resolve::get_normalized(initial_first_derivative, idvAbs / -overhead);
		return math::get_normalized(instruction, max_second_derivative);
	}
}