module;

#include "../adapted_attributes.hpp"
#include <cassert>

#define MATH_ATTR [[nodiscard]] inline FORCE_INLINE
#define MATH_ASSERT(expr) CHECKED_ASSUME(expr)

export module mo_yanxi.math;

import std;

import SinTable;
import mo_yanxi.concepts;

//TODO clean some ...

namespace mo_yanxi::math {
	export constexpr inline std::array SIGNS{ -1, 1 };
	export constexpr inline std::array ZERO_ONE{ 0, 1 };
	export constexpr inline std::array BOOLEANS{ true, false };

	export constexpr double inline FLOATING_ROUNDING_ERROR = 0.000001;
	export constexpr float inline pi                   = std::numbers::pi_v<float>;



	export
	template <std::floating_point Fp>
	constexpr float inline pi_2_v = std::numbers::pi_v<Fp> * static_cast<Fp>(2);
	export
	template <std::floating_point Fp>
	constexpr float inline pi_half_v = std::numbers::pi_v<Fp> / static_cast<Fp>(2);

	export constexpr inline float pi_half = pi_half_v<float>;
	export constexpr inline float pi_2       = pi_2_v<float>;


	export constexpr inline float e         = std::numbers::e_v<float>;
	export constexpr inline float sqrt2     = std::numbers::sqrt2_v<float>;
	export constexpr inline float sqrt3     = std::numbers::sqrt3_v<float>;

	export constexpr inline float circle_deg_full          = 360.0f;
	export constexpr inline float circle_deg_half          = 180.0f;

	export
	template <std::floating_point Fp>
	constexpr float inline rad_to_deg_v = static_cast<Fp>(180) / std::numbers::pi_v<Fp>;

	export
	template <std::floating_point Fp>
	constexpr float inline deg_to_rad_v = std::numbers::pi_v<Fp> / static_cast<Fp>(180);

	export constexpr float inline rad_to_deg = rad_to_deg_v<float>;
	export constexpr float inline deg_to_rad = deg_to_rad_v<float>;

	constexpr float inline RAD_DEG            = rad_to_deg;
	constexpr float inline DEG_RAD            = deg_to_rad;


	constexpr inline unsigned int SIN_BITS  = 16;
	constexpr inline unsigned int SIN_MASK  = ~(~0u << SIN_BITS);
	constexpr inline unsigned int SIN_COUNT = SIN_MASK + 1;
	constexpr float RAD_TO_INDEX      = SIN_COUNT / pi_2;
	constexpr float DEG_TO_INDEX      = SIN_COUNT / circle_deg_full;
	constexpr int BIG_ENOUGH_INT      = 16 * 1024;
	constexpr double BIG_ENOUGH_FLOOR = BIG_ENOUGH_INT;
	constexpr double CEIL             = 0.99999;
	constexpr double BIG_ENOUGH_ROUND = static_cast<double>(BIG_ENOUGH_INT) + 0.5f;

	constexpr auto sinTable = genTable<SIN_MASK, DEG_TO_INDEX>();

	/**
	 * @brief indicate a floating point generally in [0, 1]
	 */
	export using progress_t = float;


	/** Returns the sine in radians from a lookup table. */
	export MATH_ATTR constexpr float sin(const float radians) noexcept {
		return sinTable[static_cast<unsigned>(radians * RAD_TO_INDEX) & SIN_MASK];
	}

	export MATH_ATTR constexpr float sin(const float radians, const float scl, const float mag) noexcept {
		return sin(radians / scl) * mag;
	}

	export MATH_ATTR constexpr float cos(const float radians) noexcept {
		return sinTable[static_cast<unsigned>((radians + pi / 2) * RAD_TO_INDEX) & SIN_MASK];
	}

	export MATH_ATTR constexpr float sin_deg(const float degrees) noexcept {
		return sinTable[static_cast<unsigned>(degrees * DEG_TO_INDEX) & SIN_MASK];
	}

	export MATH_ATTR constexpr float cos_deg(const float degrees) noexcept {
		return sinTable[static_cast<unsigned>((degrees + 90) * DEG_TO_INDEX) & SIN_MASK];
	}

	/**
	 * @return [0, max] within cycle
	 */
	export MATH_ATTR constexpr float absin(const float inRad, const float cycle, const float max) noexcept {
		return (sin(inRad, cycle / pi_2, max) + max) / 2.0f;
	}

	export MATH_ATTR constexpr float sin_range(const float inRad, const float cycle, const float min, const float max) noexcept {
		const auto sine = absin(inRad, cycle, max - min);
		return min + sine;
	}

	export MATH_ATTR constexpr float tan(const float radians, const float scl, const float mag) noexcept {
		return sin(radians / scl) / cos(radians / scl) * mag;
	}

	export MATH_ATTR constexpr float cos(const float radians, const float scl, const float mag) noexcept {
		return cos(radians / scl) * mag;
	}

	export struct cos_sin_result{
		float cos;
		float sin;
	};

	export MATH_ATTR constexpr cos_sin_result cos_sin(const float radians) noexcept{
		return {cos(radians), sin(radians)};
	}

	export MATH_ATTR constexpr cos_sin_result cos_sin_deg(const float degree)noexcept{
		return {cos_deg(degree), sin_deg(degree)};
	}

	namespace angles{

		/** Wraps the given angle to the range [-pi, pi]
		 * @param a the angle in radians
		 * @return the given angle wrapped to the range [-pi, pi] */
		export
		template <std::floating_point T>
		MATH_ATTR T uniform_rad(const T angle_in_rad) noexcept{
			static constexpr auto pi = std::numbers::pi_v<T>;
			static constexpr auto pi2 = pi_2_v<T>;
			if(angle_in_rad >= 0){
				T rotation = std::fmod(angle_in_rad, pi2);
				if(rotation > pi) rotation -= pi2;
				return rotation;
			}

			T rotation = std::fmod(-angle_in_rad, pi2);
			if(rotation > pi) rotation -= pi2;
			return -rotation;
		}
	}


	export
	template <std::floating_point T1, std::floating_point T2>
	[[deprecated]] MATH_ATTR auto angle_exact_positive(const T1 x, const T2 y) noexcept{
		using Ty = std::common_type_t<T1, T2>;
		auto result = std::atan2(static_cast<Ty>(y), static_cast<Ty>(x)) * rad_to_deg_v<Ty>;
		if(result < 0) result += static_cast<Ty>(circle_deg_full);
		return result;
	}

	/** A variant on atan that does not tolerate infinite inputs for speed reasons, and because infinite inputs
	 * should never occur where this is used (only in {@link atan2(float, float)@endlink}).
	 * @param i any finite float
	 * @return an output from the inverse tangent function, from pi/-2.0 to pi/2.0 inclusive
	 * */
	export MATH_ATTR double atn(const double i) noexcept{
		// We use double precision internally, because some constants need double precision.
		const double n = std::abs(i);
		// c uses the "equally-good" formulation that permits n to be from 0 to almost infinity.
		const double c = (n - 1.0) / (n + 1.0);
		// The approximation needs 6 odd powers of c.
		const double c2 = c * c, c3 = c * c2, c5 = c3 * c2, c7 = c5 * c2, c9 = c7 * c2, c11 = c9 * c2;
		return
			std::copysign(std::numbers::pi * 0.25
			              + (0.99997726 * c - 0.33262347 * c3 + 0.19354346 * c5 - 0.11643287 * c7 + 0.05265332 * c9 -
				              0.0117212 * c11), i);
	}

	/** Close approximation of the frequently-used trigonometric method atan2, with higher precision than libGDX's atan2
	 * approximation. Average error is 1.057E-6 radians; maximum error is 1.922E-6. Takes y and x (in that unusual order) as
	 * floats, and returns the angle from the origin to that point in radians. It is about 4 times faster than
	 * {@link atan2(double, double)@endlink} (roughly 15 ns instead of roughly 60 ns for Math, on Java 8 HotSpot). <br>
	 * Credit for this goes to the 1955 research study "Approximations for Digital Computers," by RAND Corporation. This is sheet
	 * 11's algorithm, which is the fourth-fastest and fourth-least precise. The algorithms on sheets 8-10 are faster, but only by
	 * a very small degree, and are considerably less precise. That study provides an atan method, and that cleanly
	 * translates to atan2().
	 * @param y y-component of the point to find the angle towards; note the parameter order is unusual by convention
	 * @param x x-component of the point to find the angle towards; note the parameter order is unusual by convention
	 * @return the angle to the given point, in radians as a float; ranges from [-pi to pi] */
	export
	template <std::floating_point T>
	MATH_ATTR T atan2(const T y, T x) noexcept {
		T n = y / x;

		if(std::isnan(n)) [[unlikely]] {
			n = y == x ? static_cast<T>(1.0f) : static_cast<T>(-1.0f); // if both y and x are infinite, n would be NaN
		} else if(std::isinf(n)) [[unlikely]]  {
			x = static_cast<T>(0.0f); // if n is infinite, y is infinitely larger than x.
		}

		if(x > 0.0f) {
			return static_cast<T>(math::atn(n));
		}
		if(x < 0.0f) {
			return y >= 0 ? static_cast<T>(math::atn(n)) + std::numbers::pi_v<T> : static_cast<T>(math::atn(n)) - std::numbers::pi_v<T>;
		}
		if(y > 0.0f) {
			return x + pi_half_v<T>;
		}
		if(y < 0.0f) {
			return x - pi_half_v<T>;
		}

		return x + y; // returns 0 for 0,0 or NaN if either y or x is NaN
	}

	export MATH_ATTR float vec_to_angle_deg(const float x, const float y) noexcept {
		float result = atan2(y, x) * RAD_DEG;
		if(result < 0) result += circle_deg_full;
		return result;
	}

	export MATH_ATTR float vec_to_angle_rad(const float x, const float y) noexcept {
		float result = atan2(y, x);
		if(result < 0) result += pi_2;
		return result;
	}

	export
	template <typename T>
		requires (std::is_arithmetic_v<T>)
	MATH_ATTR constexpr std::ranges::min_max_result<T> minmax(const T a, const T b) noexcept{
		if(b < a){
			return {b, a};
		}

		return {a, b};
	}


	export
	template <mo_yanxi::small_object T>
		requires requires(T t){
		{t < t} -> std::convertible_to<bool>;
		}
	MATH_ATTR constexpr T max(const T v1, const T v2) noexcept(noexcept(v2 < v1)) {
		if constexpr (std::floating_point<T>){
			if (!std::is_constant_evaluated()){
				return std::fmax(v1, v2);
			}
		}

		return v2 < v1 ? v1 : v2;
	}

	export
	template <mo_yanxi::small_object T>
		requires requires(T t){
		{t < t} -> std::convertible_to<bool>;
		}
	MATH_ATTR constexpr T min(const T v1, const T v2) noexcept(noexcept(v1 < v2)) {
		if constexpr (std::floating_point<T>){
			if (!std::is_constant_evaluated()){
				return std::fmin(v1, v2);
			}
		}

		return v1 < v2 ? v1 : v2;
	}


	export
	template <typename T, typename... Args>
		requires (std::is_arithmetic_v<T> && (std::is_arithmetic_v<Args> && ...))
	MATH_ATTR constexpr std::ranges::min_max_result<T> minmax(T first, T second, Args... args) noexcept{
		const auto [min1, max1] = math::minmax(first, second);

		if constexpr(sizeof ...(Args) == 1){
			return {math::min(min1, args...), math::max(max1, args...)};
		} else{
			const auto [min2, max2] = math::minmax(args...);
			return {math::min(min1, min2), math::max(max1, max2)};
		}
	}

	export
	template <std::integral T>
		requires (sizeof(T) <= 4)
	MATH_ATTR constexpr int digits(const T n_positive) noexcept {
		return n_positive < 100000
			       ? n_positive < 100
				         ? n_positive < 10
					           ? 1
					           : 2
				         : n_positive < 1000
					           ? 3
					           : n_positive < 10000
						             ? 4
						             : 5
			       : n_positive < 10000000
				         ? n_positive < 1000000
					           ? 6
					           : 7
				         : n_positive < 100000000
					           ? 8
					           : n_positive < 1000000000
						             ? 9
						             : 10;
	}

	export
		template <mo_yanxi::number T>
	MATH_ATTR int digits(const T n_positive) noexcept {
		return n_positive == 0 ? 1 : static_cast<int>(std::log10(n_positive) + 1);
	}

	export MATH_ATTR constexpr float sqr(const float x) noexcept {
		return x * x;
	}

	export [[deprecated]] MATH_ATTR float sqrt(const float x) noexcept {
		return std::sqrt(x);
	}

	export
	template <typename T1, typename T2>
		/*requires requires(T1 v1, T2 v2){
			v1 - v1;
			(v1 - v1) / (v1 - v1);

			v2 - v2;
			{ (v1 - v1) / (v1 - v1) * (v2 - v2) } -> std::convertible_to<T2>;
		}*/
	MATH_ATTR constexpr T2 map(
		const T1 value,
		const T1 fromA, const T1 toa,
		const T2 fromB, const T2 tob
	) noexcept/*nvm*/ {
		const auto prog = (value - fromA) / (toa - fromA);
		if constexpr (std::floating_point<T2> && std::floating_point<decltype(prog)>){
			return std::fma(prog, tob - fromB, fromB);
		}else{
			return prog * (tob - fromB) + fromB;
		}
	}

	MATH_ATTR
	constexpr /** Map value from [0, 1].*/
	float map(const float value, const float from, const float to) noexcept {
		return map(value, 0.f, 1.f, from, to);
	}

	/**Returns -1 if f<0, 1 otherwise.*/
	MATH_ATTR constexpr bool diff_sign(const mo_yanxi::number auto a, const  auto b) noexcept {
		return a * b < 0;
	}

	export
	template <mo_yanxi::number T>
	MATH_ATTR constexpr T sign(const T f) noexcept{
		if constexpr (std::floating_point<T>){
			if(f == static_cast<T>(0.0)) [[unlikely]] {
				return static_cast<T>(0.0);
			}

			return f < static_cast<T>(0.0) ? static_cast<T>(-1.0) : static_cast<T>(1.0);
		}else if constexpr (std::unsigned_integral<T>){
			return f == 0 ? 0 : 1;
		}else{
			return f == 0 ? 0 : f < 0 ? -1 : 1;
		}
	}

	/** Returns 1 if true, -1 if false. */
	export MATH_ATTR constexpr int sign(const bool b) noexcept {
		return b ? 1 : -1;
	}

	export
	template <unsigned Exponent, typename T>
	MATH_ATTR constexpr T pow_integral(const T val) noexcept {
		if constexpr(Exponent == 0) {
			return 1;
		}else if constexpr (Exponent == 1) {
			return val;
		}else if constexpr (Exponent % 2 == 0) {
			const T v = math::pow_integral<Exponent / 2, T>(val);
			return v * v;
		}else {
			const T v = math::pow_integral<(Exponent - 1) / 2, T>(val);
			return val * v * v;
		}
	}

	export
	template <typename T, std::unsigned_integral E>
	MATH_ATTR constexpr T pow_integral(T val, const E exp) noexcept{
		const auto count = sizeof(E) * 8 - std::countl_zero(exp);

		T rst{1};

		for(auto i = count; i; --i){
			rst *= rst;
			if(exp >> (i - 1) & static_cast<E>(1u)){
				rst *= val;
			}
		}

		return rst;
	}

	export
	template <typename T, std::signed_integral E>
	MATH_ATTR constexpr T pow_integral(T val, const E exp) noexcept{
		assert(exp > 0);
		return math::pow_integral(val, static_cast<std::make_unsigned_t<E>>(exp));
	}

	export
	template <typename T, std::unsigned_integral E>
	MATH_ATTR constexpr T div_integral(const T val, const T div, const E times) noexcept {
		T rst = val;
		for(E i = 0; i < times; ++i){
			rst /= div;
		}
		return rst;
	}

	/** Returns the next power of two. Returns the specified value if the value is already a power of two. */
	export MATH_ATTR constexpr int next_bit_ceil(std::unsigned_integral auto value) noexcept {
		return std::bit_ceil(value) + 1;
	}

	export
	template <mo_yanxi::number T>
	MATH_ATTR constexpr T clamp(const T v, const T min = static_cast<T>(0), const T max = static_cast<T>(1)) noexcept{
		if(v > max) return max;
		if(v < min) return min;

		return v;
	}

	export
	template <mo_yanxi::number T>
	MATH_ATTR constexpr T clamp_sorted(const T v, const T min, const T max) noexcept{
		const auto [l, r] = math::minmax(min, max);
		if(v > r) return max;
		if(v < l) return min;

		return v;
	}

	export
	template <mo_yanxi::number T>
	MATH_ATTR constexpr T abs(const T v) noexcept {
		//TODO make it std::abs after c++ 23
		if (std::is_constant_evaluated()){
			return v < 0 ? -v : v;
		}else{
			return std::abs(v);
		}
	}

	export
	template <mo_yanxi::number T>
	MATH_ATTR T clamp_range(const T v, const T absMax) noexcept{
		MATH_ASSERT(absMax >= 0);

		if(math::abs(v) > absMax){
			return std::copysign(absMax, v);
		}

		return v;
	}

	export
	template <mo_yanxi::number T>
	MATH_ATTR constexpr T max_abs(const T v1, const T v2) noexcept {
		if constexpr (mo_yanxi::non_negative<T>){
			return math::max(v1, v2);
		}else{
			return math::abs(v1) > math::abs(v2) ? v1 : v2;
		}
	}

	export
	template <mo_yanxi::number T>
	MATH_ATTR constexpr T min_abs(const T v1, const T v2) noexcept {
		if constexpr (mo_yanxi::non_negative<T>){
			return math::min(v1, v2);
		}else{
			return math::abs(v1) < math::abs(v2) ? v1 : v2;
		}
	}

	export
	template <mo_yanxi::number T>
	MATH_ATTR constexpr T clamp_positive(const T val) noexcept {
		return math::max<T>(val, 0);
	}


	/** Approaches a value at linear speed. */
	export MATH_ATTR constexpr auto approach(const auto from, const auto to, const auto speed) noexcept {
		return from + math::clamp(to - from, -speed, speed);
	}

	/** Approaches a value at linear speed. */
	export FORCE_INLINE constexpr void approach_inplace(auto& from, const auto to, const auto speed) noexcept {
		from += math::clamp(to - from, -speed, speed);
	}

	export
	template <typename T>
	struct approach_result{
		T rst;
		bool reached;
	};

	export
	template <typename T>
	MATH_ATTR constexpr approach_result<T> forward_approach_then(const T current, const T destination, const T delta) noexcept {
		MATH_ASSERT(current <= destination);
		const auto rst = current + delta;
		if(rst >= destination){
			return {destination, true};
		}else{
			return {rst, false};
		}
	}

	export
	template <typename T>
	MATH_ATTR constexpr float forward_approach(const T current, const T destination, const float delta) noexcept {
		MATH_ASSERT(current <= destination);

		const auto rst = current + delta;
		if(rst >= destination){
			return destination;
		}else{
			return rst;
		}
	}

	export
	template <mo_yanxi::small_object T, typename Prog>
	MATH_ATTR constexpr T lerp(const T fromValue, const T toValue, const Prog progress) noexcept {
		if constexpr (std::floating_point<T> && std::floating_point<Prog>){
			if (!std::is_constant_evaluated()){
				return std::fma((toValue - fromValue), progress, fromValue);
			}
		}
		
		return fromValue * (1 - progress) + toValue * progress;
	}

	export
	template <typename T, typename Prog>
		requires (!small_object<T>)
	MATH_ATTR constexpr T lerp(const T& fromValue, const T& toValue, const Prog progress) noexcept {
		return fromValue + (toValue - fromValue) * progress;
	}

	export
	template <mo_yanxi::small_object T, typename Prog>
	MATH_ATTR constexpr void lerp_inplace(T& fromValue, const T toValue, const Prog progress) noexcept {
		if constexpr (std::floating_point<T> && std::floating_point<Prog>){
			if (!std::is_constant_evaluated()){
				fromValue = std::fma((toValue - fromValue), progress, fromValue);
				return;
			}
		}
		
		fromValue += (toValue - fromValue) * progress;
	}
	
	/**
	 * Linearly interpolates between two angles in radians. Takes into account that angles wrap at two pi and always takes the
	 * direction with the smallest delta angle.
	 * @param fromRadians start angle in radians
	 * @param toRadians target angle in radians
	 * @param progress interpolation value in the range [0, 1]
	 * @return the interpolated angle in the range [0, pi2]
	 */
	export
	template <std::floating_point T>
	MATH_ATTR T slerp_rad(const T fromRadians, const T toRadians, const T progress) noexcept {
		using namespace std::numbers;
		const float delta = std::fmod(toRadians - fromRadians + pi_2_v<T> + pi_v<T>, pi_2_v<T>) - pi_v<T>;
		return std::fmod(fromRadians + delta * progress + pi_2_v<T>, pi_2_v<T>);
	}

	/**
	 * Linearly interpolates between two angles in degrees. Takes into account that angles wrap at 360 degrees and always takes
	 * the direction with the smallest delta angle.
	 * @param fromDegrees start angle in degrees
	 * @param toDegrees target angle in degrees
	 * @param progress interpolation value in the range [0, 1]
	 * @return the interpolated angle in the range [0, 360[
	 */
	export MATH_ATTR float slerp(const float fromDegrees, const float toDegrees, const float progress) noexcept {
		const float delta = std::fmod(toDegrees - fromDegrees + circle_deg_full + 180.0f, circle_deg_full) - 180.0f;
		return std::fmod(fromDegrees + delta * progress + circle_deg_full, circle_deg_full);
	}

	/**
	 * Returns the largest integer less than or equal to the specified float. This method will only properly floor floats from
	 * -(2^14) to (Float.MAX_VALUE - 2^14).
	 */
	export
	template <std::integral T = int>
	MATH_ATTR constexpr T floorLEqual(const float value) noexcept {
		return static_cast<T>(value + BIG_ENOUGH_FLOOR) - BIG_ENOUGH_INT;
	}

	/**
	 * Returns the largest integer less than or equal to the specified float. This method will only properly floor floats that are
	 * positive. Note this method simply casts the float to int.
	 */
	export
	template <std::integral T = int>
	MATH_ATTR constexpr T trunc_right(const float value) noexcept {
		T val = static_cast<T>(value);
		if constexpr (mo_yanxi::signed_number<T>){
			if(value < 0)--val;
		}

		return val;
	}

	/**
	 * Returns the smallest integer greater than or equal to the specified float. This method will only properly ceil floats from
	 * -(2^14) to (Float.MAX_VALUE - 2^14).
	 */
	export
	template <typename T = int>
	MATH_ATTR constexpr T ceil(const float value) noexcept {
		if (std::is_constant_evaluated()){
			return BIG_ENOUGH_INT - static_cast<T>(BIG_ENOUGH_FLOOR - value);
		}else{
			return static_cast<T>(std::ceil(value));
		}

	}

	/**
	 * Returns the smallest integer greater than or equal to the specified float. This method will only properly ceil floats that
	 * are positive.
	 */
	export
	template <std::integral T = int>
	MATH_ATTR constexpr T ceil_positive(const float value) noexcept {
		MATH_ASSERT(value >= 0);
		return static_cast<T>(value + CEIL);
	}


	export
	template <typename T, typename T0>
		requires (std::is_integral_v<T>)
	MATH_ATTR constexpr T round(const T0 value) noexcept {
		if constexpr (std::floating_point<T0>){
			return std::lround(value);
		}else{
			return static_cast<T>(value);
		}
	}

	export MATH_ATTR constexpr auto floor(const std::integral auto value, const std::integral auto step) noexcept {
		return value / step * step;
	}

	export
	template <std::integral T = int>
	MATH_ATTR constexpr T floor(const float value, const T step = 1) noexcept {
		return static_cast<T>(value / static_cast<float>(step)) * step;
	}

	export
	template <mo_yanxi::number T>
	MATH_ATTR T round(const T num, const T step) {
		return static_cast<T>(std::lround(std::round(static_cast<float>(num) / static_cast<float>(step)) * static_cast<float>(step)));
	}

	/**
	 * Returns true if the value is zero.
	 * @param value N/A
	 * @param tolerance represent an upper bound below which the value is considered zero.
	 */
	export MATH_ATTR bool zero(const float value, const float tolerance = FLOATING_ROUNDING_ERROR) noexcept {
		return abs(value) <= tolerance;
	}

	/**
	 * Returns true if a is nearly equal to b. The function uses the default floating error tolerance.
	 * @param a the first value.
	 * @param b the second value.
	 */
	export MATH_ATTR bool equal(const float a, const float b) noexcept {
		return std::abs(a - b) <= FLOATING_ROUNDING_ERROR;
	}

	/**
	 * Returns true if a is nearly equal to b.
	 * @param a the first value.
	 * @param b the second value.
	 * @param tolerance represent an upper bound below which the two values are considered equal.
	 */
	export MATH_ATTR bool equal(const float a, const float b, const float tolerance) noexcept {
		return abs(a - b) <= tolerance;
	}

	export
	template <mo_yanxi::number T>
	MATH_ATTR T mod(const T x, const T n) noexcept {
		if constexpr(std::floating_point<T>) {
			return std::fmod(x, n);
		} else {
			return x % n;
		}
	}

	/**
	 * @return a sampled value based on position in an array of float values.
	 * @param values toLerp
	 * @param time [0, 1]
	 */
	export
	template <typename T>
	constexpr MATH_ATTR float lerp_span(const std::span<T> values, float time) noexcept {
		if constexpr (values.size() == 0){
			return T{};
		}

		time             = clamp(time);

		const auto sizeF = static_cast<float>(values.size() - 1ull);

		const float pos = time * sizeF;

		const auto cur  = static_cast<std::size_t>(pos);
		const auto next = math::min(cur + 1ULL, values.size() - 1ULL);
		const float mod = pos - static_cast<float>(cur);
		return math::lerp<T>(values[cur], values[next], mod);
	}

	export
	template <typename T, std::size_t size>
	constexpr MATH_ATTR float lerp_span(const std::array<T, size>& values, float time) noexcept{
		if constexpr (size == 0){
			return T{};
		}

		time = clamp(time);
		const auto sizeF = static_cast<float>(size - 1ull);
		const float pos = time * sizeF;

		const auto cur = static_cast<std::size_t>(pos);
		const auto next = math::min(cur + 1ULL, size - 1ULL);
		const float mod = pos - static_cast<float>(cur);
		return math::lerp(values[cur], values[next], mod);
	}

	export
	/** @return the input 0-1 value scaled to 0-1-0. */
	constexpr MATH_ATTR float slope(const float fin) noexcept {
		return 1.0f - math::abs(fin - 0.5f) * 2.0f;
	}

	/**Converts a 0-1 value to 0-1 when it is in [offset, 1].*/
	export
	template <std::floating_point Fp>
	constexpr MATH_ATTR Fp curve(const Fp f, const Fp offset) noexcept {
		if(f < offset) {
			return 0.0f;
		}
		return (f - offset) / (static_cast<Fp>(1.0f) - offset);
	}

	/**Converts a 0-1 value to 0-1 when it is in [offset, to].*/
	export
	template <std::floating_point Fp>
	constexpr MATH_ATTR Fp curve(const Fp f, const Fp from, const Fp to) noexcept {
		if(f < from) {
			return 0.0f;
		}
		if(f > to) {
			return 1.0f;
		}
		return (f - from) / (to - from);
	}

	/** Transforms a 0-1 value to a value with a 0.5 plateau in the middle. When margin = 0.5, this method doesn't do anything. */
	export MATH_ATTR constexpr float curve_margin(const float f, const float marginLeft, const float marginRight) noexcept {
		if(f < marginLeft) return f / marginLeft * 0.5f;
		if(f > 1.0f - marginRight) return (f - 1.0f + marginRight) / marginRight * 0.5f + 0.5f;
		return 0.5f;
	}


	/**
	 * \return a correCt dst value safe for unsigned numbers, can be negative if params are signed.
	 */
	export
	template <mo_yanxi::number T>
	MATH_ATTR T constexpr dst_safe(const T src, const T dst) noexcept {
		if constexpr(!std::is_unsigned_v<T>) {
			return dst - src;
		} else {
			return src > dst ? src - dst : dst - src;
		}
	}

	/** Transforms a 0-1 value to a value with a 0.5 plateau in the middle. When margin = 0.5, this method doesn't do anything. */
	export MATH_ATTR constexpr float curve_margin(const float f, const float margin) noexcept {
		return curve_margin(f, margin, margin);
	}

	export MATH_ATTR constexpr float dot(const float x1, const float y1, const float x2, const float y2) noexcept {
		return x1 * x2 + y1 * y2;
	}

	export
	template <typename T>
	MATH_ATTR constexpr T dst(const T x1, const T y1) noexcept {
		return static_cast<T>(std::hypot(x1, y1));
	}

	export
	template <typename T>
	MATH_ATTR constexpr T dst2(const T x1, const T y1) noexcept {
		return x1 * x1 + y1 * y1;
	}

	export
	template <typename T>
	MATH_ATTR T dst(const T x1, const T y1, const T x2, const T y2) noexcept {
		return math::dst(math::dst_safe(x2, x1), math::dst_safe(y2, y1));
	}

	export
	template <typename T>
	MATH_ATTR constexpr T dst2(const T x1, const T y1, const T x2, const T y2) noexcept {
		const float xd = math::dst_safe(x1, x2);
		const float yd = math::dst_safe(y1, y2);
		return xd * xd + yd * yd;
	}


	/** Manhattan distance. */
	export
	template <typename T>
	MATH_ATTR T dst_mht(const T x1, const T y1, const T x2, const T y2) noexcept {
		return math::abs(x1 - x2) + math::abs(y1 - y2);
	}


	/** @return whether dst(x1, y1, x2, y2) < dst */
	export
	template <typename T>
	MATH_ATTR bool within(const T x1, const T y1, const T x2, const T y2, const T dst) noexcept {
		return math::dst2(x1, y1, x2, y2) < dst * dst;
	}

	/** @return whether dst(x, y, 0, 0) < dst */
	export
	template <typename T>
	MATH_ATTR bool within(const T x1, const T y1, const T dst) noexcept {
		return (x1 * x1 + y1 * y1) < dst * dst;
	}

	// template<ext::number auto cycle, auto trigger = cycle / 2>
	// export MATH_ATTR bool cycleStep(ext::number auto cur){
	// 	return math::mod<decltype(cur)>(cur, cycle) < trigger;
	// }

	namespace angles{
		/**
		 * @return Angle in [0, 360] degree
		 */
		export MATH_ATTR float remove_deg_cycles(float a) noexcept {
			a = math::mod<float>(a, circle_deg_full);
			if(a < 0)a += circle_deg_full;
			return a;
		}

		/**
		 * @return Angle in [-180, 180] degree
		 */
		export MATH_ATTR float uniform_deg(float a) noexcept {
			a = remove_deg_cycles(a);
			if(a > circle_deg_half){
				return a - circle_deg_full;
			}
			return a;
		}

		export MATH_ATTR float forward_distance(const float angle1, const float angle2) noexcept {
			return math::abs(angle1 - angle2);
		}

		export MATH_ATTR float backward_distance(const float angle1_in_deg, const float angle2_in_deg) noexcept {
			return circle_deg_full - math::abs(angle1_in_deg - angle2_in_deg);
		}

		export MATH_ATTR float dst_with_sign(float a_in_deg, float b_in_deg) noexcept {
			a_in_deg = remove_deg_cycles(a_in_deg);
			b_in_deg = remove_deg_cycles(b_in_deg);

			float dst = -(a_in_deg - b_in_deg);

			if(abs(dst) > 180){
				dst *= -1;
			}

			return std::copysign(math::min((a_in_deg - b_in_deg) < 0 ? a_in_deg - b_in_deg + circle_deg_full : a_in_deg - b_in_deg, (b_in_deg - a_in_deg) < 0 ? b_in_deg - a_in_deg + circle_deg_full : b_in_deg - a_in_deg), dst);
		}

		export MATH_ATTR float dst(float a_in_deg, float b_in_deg) noexcept {
			a_in_deg = remove_deg_cycles(a_in_deg);
			b_in_deg = remove_deg_cycles(b_in_deg);
			return math::min((a_in_deg - b_in_deg) < 0 ? a_in_deg - b_in_deg + circle_deg_full : a_in_deg - b_in_deg, (b_in_deg - a_in_deg) < 0 ? b_in_deg - a_in_deg + circle_deg_full : b_in_deg - a_in_deg);
		}

		/**
		 * @brief
		 * @param a Angle A
		 * @param b Angle B
		 * @return A incounter-clockwise to B -> 1 or -1, 0
		 */
		export MATH_ATTR float sign_of_dst(float a, float b) noexcept {
			a = remove_deg_cycles(a);
			b = remove_deg_cycles(b);

			float dst = -(a - b);

			if(abs(dst) > 180){
				dst *= -1;
			}

			return sign(dst);
		}

		export MATH_ATTR float move_toward_unsigned(float angle, float to, const float speed_in_positive) noexcept {
			MATH_ASSERT(speed_in_positive >= 0);
			if(math::abs(angles::dst(angle, to)) < speed_in_positive) return to;
			angle = remove_deg_cycles(angle);
			to = remove_deg_cycles(to);

			if((angle > to) == (backward_distance(angle, to) > forward_distance(angle, to))){
				angle -= speed_in_positive;
			}else{
				angle += speed_in_positive;
			}

			return angle;
		}

		export MATH_ATTR approach_result<float> move_toward_signed(
			const float angle,
			const float to,
			const float speed,
			const float margin) noexcept {
			if(const float adst = angles::dst(angle, to), absSpeed = std::abs(speed); adst < absSpeed && absSpeed < margin){
				return {to, true};
			}

			return {angle + speed, false};
		}

		export MATH_ATTR bool within(const float a, const float b, const float margin) noexcept {
			return dst(a, b) <= margin;
		}

		export MATH_ATTR float clamp_range(const float angle, const float dest, const float range) noexcept {
			const float dst = angles::dst(angle, dest);
			return dst <= range ? angle : move_toward_unsigned(angle, dest, dst - range);
		}
	}

	export
	template <small_object T>
	MATH_ATTR T fma(const T x, const T y, const T z) noexcept {
		if constexpr (std::floating_point<T>){
			return  std::fma(x, y, z);
		}else{
			return x * y + z;
		}
	}


	export
	template <typename T>
	struct section{
		T from;
		T to;

		MATH_ATTR constexpr section to_ordered() const noexcept{
			const auto [min, max] = std::minmax(from, to);
			return {min, max};
		}

		MATH_ATTR constexpr T length() const noexcept{
			return math::dst_safe(from, to);
		}

		MATH_ATTR constexpr T clamp(const T val) const noexcept{
			return math::clamp(val, from, to);
		}

		MATH_ATTR constexpr T operator[](std::floating_point auto fp) const noexcept{
			return math::lerp(from, to, fp);
		}

		FORCE_INLINE constexpr section& operator*=(const T val) noexcept{
			from *= val;
			to *= val;
			return *this;
		}

		FORCE_INLINE constexpr friend section operator*(section section, const T val) noexcept{
			section *= val;
			return section;
		}
	};

	export
	using range = section<float>;


	export
	template <typename T>
	struct based_section{
		T base;
		T append;

		MATH_ATTR constexpr T length() const noexcept{
			return math::abs(append);
		}

		MATH_ATTR constexpr T operator[](std::floating_point auto fp) const noexcept{
			return math::fma(fp, append, base);
		}

		operator section<T>(){
			return section<T>{base, base + append};
		}
	};

	export
	template <std::floating_point T>
	constexpr MATH_ATTR T snap(T value, T step) {
		MATH_ASSERT(step > 0);
		return std::round(value / step) * step;
	}
}
//
// template <std::size_t N, typename T>
// struct std::tuple_element<N, Math::minmax_result<T>> {
// 	using type = T;
// };
//
// template < typename T>
// struct std::tuple_size<std::ranges::min_max_result<T>> : std::integral_constant<std::size_t, 2>{};
//
//
// void foo(){
// 	std::unordered_map<int, float> m{};
// 	m | std::ranges::views::values();
// }
