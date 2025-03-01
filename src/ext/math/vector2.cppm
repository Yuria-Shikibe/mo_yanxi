module;

#include "../adapted_attributes.hpp"

export module mo_yanxi.math.vector2;

import mo_yanxi.math;
import mo_yanxi.concepts;
import std;

export namespace mo_yanxi::math{

	template <mo_yanxi::number T>
	struct vector2
	{
		T x;
		T y;

		using value_type = T;
		using floating_point_t = std::conditional_t<std::floating_point<T>, T,
			std::conditional_t<sizeof(T) <= 4, float, double>
		>;

		using const_pass_t = mo_yanxi::conditional_pass_type<const vector2, sizeof(T) * 2>;

		FORCE_INLINE [[nodiscard]] constexpr vector2 operator+(const_pass_t tgt) const noexcept{
			return {x + tgt.x, y + tgt.y};
		}

		FORCE_INLINE [[nodiscard]] constexpr vector2 operator-(const_pass_t tgt) const noexcept {
			return {x - tgt.x, y - tgt.y};
		}

		FORCE_INLINE [[nodiscard]] constexpr vector2 operator*(const_pass_t tgt) const noexcept {
			return {x * tgt.x, y * tgt.y};
		}

		FORCE_INLINE friend constexpr vector2 operator*(const_pass_t v, const T val) noexcept {
			return {v.x * val, v.y * val};
		}
		FORCE_INLINE friend constexpr vector2 operator*(const T val, const_pass_t v) noexcept {
			return {v.x * val, v.y * val};
		}

		/*template <ext::number N>
		[[nodiscard]] constexpr auto operator*(const N val) const noexcept {
			if(std::is_unsigned_v<T> && !std::is_unsigned_v<N>){
				using S = std::make_signed_t<T>;
				using R = std::common_type_t<S, N>;

				return Vector2D<R>{static_cast<R>(x) * static_cast<R>(val), static_cast<R>(y) * static_cast<R>(val)};
			}
			return Vector2D{x * val, y * val};
		}*/

		FORCE_INLINE [[nodiscard]] constexpr auto operator-() const noexcept{
			if constexpr (std::is_unsigned_v<T>){
				using S = std::make_signed_t<T>;
				return vector2<std::make_signed_t<T>>{-static_cast<S>(x), -static_cast<S>(y)};
			}else{
				return vector2<T>{- x, - y};
			}

		}

		FORCE_INLINE [[nodiscard]] constexpr vector2 operator/(const T val) const noexcept {
			return {x / val, y / val};
		}

		FORCE_INLINE [[nodiscard]] constexpr vector2 operator/(const_pass_t tgt) const noexcept {
			return {x / tgt.x, y / tgt.y};
		}

		FORCE_INLINE [[nodiscard]] constexpr vector2 operator%(const_pass_t tgt) const noexcept {
			return {math::mod<T>(x, tgt.x), math::mod<T>(y, tgt.y)};
		}

		FORCE_INLINE constexpr vector2& operator+=(const_pass_t tgt) noexcept {
			return this->add(tgt);
		}

		FORCE_INLINE constexpr vector2 operator~() const noexcept{
			if constexpr (std::floating_point<T>){
				return {static_cast<T>(1) / x, static_cast<T>(1) / y};
			}else if constexpr (std::unsigned_integral<T>){
				return {~x, ~y};
			}else{
				static_assert(false, "unsupported");
			}
		}

		FORCE_INLINE constexpr vector2& inverse() noexcept{
			return this->set(this->operator~());
		}

		template <std::floating_point Ty = float>
		FORCE_INLINE constexpr vector2<Ty>& reciprocal() const noexcept{
			return ~as<Ty>();
		}

		FORCE_INLINE constexpr vector2& operator+=(const T tgt) noexcept {
			return this->add(tgt);
		}

		FORCE_INLINE constexpr vector2& operator-=(const_pass_t tgt) noexcept {
			return this->sub(tgt);
		}

		FORCE_INLINE constexpr vector2& operator*=(const_pass_t tgt) noexcept {
			return this->mul(tgt);
		}

		FORCE_INLINE constexpr vector2& operator*=(const T val) noexcept {
			return this->scl(val);
		}

		FORCE_INLINE constexpr vector2& operator/=(const_pass_t tgt) noexcept {
			return this->sub(tgt);
		}

		FORCE_INLINE constexpr vector2& operator/=(const T tgt) noexcept {
			return this->div(tgt, tgt);
		}

		FORCE_INLINE constexpr vector2& operator%=(const_pass_t tgt) noexcept {
			return this->mod(tgt.x, tgt.y);
		}

		FORCE_INLINE constexpr vector2& operator%=(const T tgt) noexcept {
			return this->mod(tgt, tgt);
		}

		FORCE_INLINE constexpr vector2& inf_to0() noexcept{
			if(std::isinf(x))x = 0;
			if(std::isinf(y))y = 0;
			return *this;
		}

		FORCE_INLINE constexpr vector2& nan_to(const_pass_t value) noexcept{
			if(std::isnan(x))x = value.x;
			if(std::isnan(y))y = value.y;
			return *this;
		}

		FORCE_INLINE constexpr vector2& nan_to0() noexcept{
			if(std::isnan(x))x = 0;
			if(std::isnan(y))y = 0;
			return *this;
		}

		FORCE_INLINE constexpr vector2& nan_to1() noexcept{
			if(std::isnan(x))x = 1;
			if(std::isnan(y))y = 1;
			return *this;
		}

		FORCE_INLINE constexpr vector2& mod(const T ox, const T oy) noexcept {
			x = math::mod(x, ox);
			y = math::mod(y, oy);
			return *this;
		}

		FORCE_INLINE constexpr vector2& mod(const T val) noexcept {
			return this->mod(val, val);
		}

		FORCE_INLINE constexpr vector2& mod(const_pass_t other) noexcept {
			return this->mod(other.x, other.y);
		}

		[[nodiscard]] FORCE_INLINE constexpr vector2 copy() const noexcept {
			return vector2{ x, y };
		}

		FORCE_INLINE constexpr vector2& set_zero() noexcept {
			return this->set(static_cast<T>(0), static_cast<T>(0));
		}

		FORCE_INLINE constexpr vector2& set_NaN() noexcept requires std::is_floating_point_v<T> {
			return set(std::numeric_limits<float>::signaling_NaN(), std::numeric_limits<float>::signaling_NaN());
		}


		[[nodiscard]] FORCE_INLINE constexpr std::size_t hash_value() const noexcept requires (sizeof(T) <= 8){
			static constexpr std::hash<std::size_t> hasher{};

			if constexpr (sizeof(T) == 8){
				const std::uint64_t a = std::bit_cast<std::uint64_t>(x);
				const std::uint64_t b = std::bit_cast<std::uint64_t>(y);
				return hasher(a ^ b << 31);
			}else if constexpr (sizeof(T) == 4){
				return hasher(std::bit_cast<std::size_t>(*this));
			}else if constexpr (sizeof(T) == 2){
				return hasher(std::bit_cast<std::uint32_t>(*this));
			}else if constexpr (sizeof(T) == 2){
				return hasher(std::bit_cast<std::uint16_t>(*this));
			}else{
				static constexpr std::hash<const std::span<std::uint8_t>> fallback{};
				return fallback(std::span{reinterpret_cast<const std::uint8_t*>(this), sizeof(T) * 2});
			}
		}


		FORCE_INLINE constexpr vector2& set(const T ox, const T oy) noexcept {
			this->x = ox;
			this->y = oy;

			return *this;
		}

		FORCE_INLINE constexpr vector2& set(const T val) noexcept {
			return this->set(val, val);
		}

		FORCE_INLINE constexpr vector2& set(const_pass_t tgt) noexcept {
			return this->operator=(tgt);
		}

		FORCE_INLINE constexpr vector2& add(const T ox, const T oy) noexcept {
			x += ox;
			y += oy;

			return *this;
		}

		FORCE_INLINE constexpr vector2& swap_xy() noexcept{
			std::swap(x, y);
			return *this;
		}

		FORCE_INLINE constexpr vector2& add(const T val) noexcept {
			x += val;
			y += val;

			return *this;
		}

		FORCE_INLINE constexpr vector2& add_x(const T val) noexcept {
			x += val;

			return *this;
		}

		FORCE_INLINE constexpr vector2& add_y(const T val) noexcept {
			y += val;

			return *this;
		}

		FORCE_INLINE constexpr vector2& add(const_pass_t other) noexcept {
			return this->add(other.x, other.y);
		}

		/**
		 * @return self * mul + add
		 */
		FORCE_INLINE /*constexpr*/ vector2& fma(const_pass_t mul, const_pass_t add) noexcept {
			x = math::fma(x, mul.x, add.x);
			y = math::fma(y, mul.y, add.y);

			return *this;
		}

		/**
		 * @return other * scale + self
		 */
		template <typename Prog>
		FORCE_INLINE /*constexpr*/ vector2& fma(const_pass_t other, const Prog scale) noexcept {
			if constexpr (std::floating_point<T> && std::floating_point<Prog>){
				x = std::fma(other.x, scale, x);
				y = std::fma(other.y, scale, y);
			}else{
				x += x * mul.x;
				y += y * mul.y;
			}

			return *this;
		}

		FORCE_INLINE constexpr vector2& sub(const T ox, const T oy) noexcept {
			x -= ox;
			y -= oy;

			return *this;
		}

		FORCE_INLINE constexpr vector2& sub(const_pass_t other) noexcept {
			return this->sub(other.x, other.y);
		}

		FORCE_INLINE constexpr vector2& sub(const_pass_t other, const T scale) noexcept {
			return this->sub(other.x * scale, other.y * scale);
		}

		FORCE_INLINE constexpr vector2& mul(const T ox, const T oy) noexcept {
			x *= ox;
			y *= oy;

			return *this;
		}

		FORCE_INLINE constexpr vector2& mul(const T val) noexcept {
			return this->mul(val, val);
		}

		FORCE_INLINE constexpr vector2& reverse() noexcept requires mo_yanxi::signed_number<T> {
			x = -x;
			y = -y;
			return *this;
		}

		FORCE_INLINE constexpr vector2& mul(const_pass_t other) noexcept {
			return this->mul(other.x, other.y);
		}

		FORCE_INLINE constexpr vector2& div(const T ox, const T oy) noexcept {
			x /= ox;
			y /= oy;

			return *this;
		}

		FORCE_INLINE constexpr vector2& div(const T val) noexcept {
			return this->div(val, val);
		}

		FORCE_INLINE constexpr vector2& div(const_pass_t other) noexcept {
			return this->div(other.x, other.y);
		}

		/*
		[[nodiscard]] FORCE_INLINE constexpr T getX() const noexcept{
			return x;
		}

		[[nodiscard]] FORCE_INLINE constexpr T getY() const noexcept{
			return y;
		}

		FORCE_INLINE constexpr vector2& setX(const T ox) noexcept{
			this->x = ox;
			return *this;
		}

		FORCE_INLINE constexpr vector2& setY(const T oy) noexcept{
			this->y = oy;
			return *this;
		}*/

		[[nodiscard]] FORCE_INLINE constexpr T dst2(const_pass_t other) const noexcept {
			const T dx = math::dst_safe(x, other.x);
			const T dy = math::dst_safe(y, other.y);

			return math::dst2(x, y, other.x, other.y);
		}

		[[nodiscard]] FORCE_INLINE constexpr float dst(const_pass_t other) const noexcept{
			return math::dst(x, y, other.x, other.y);
		}

		[[nodiscard]] FORCE_INLINE constexpr float dst(const float tx, const float ty) const noexcept{
			return math::dst(x, y, tx, ty);
		}

		[[nodiscard]] FORCE_INLINE constexpr float dst2(const float tx, const float ty) const noexcept{
			return math::dst2(x, y, tx, ty);
		}

		[[nodiscard]] FORCE_INLINE constexpr bool within(const_pass_t other, const T dst) const noexcept{
			return this->dst2(other) < dst * dst;
		}

		[[nodiscard]] FORCE_INLINE constexpr bool is_NaN() const noexcept{
			if constexpr(std::is_floating_point_v<T>) {
				return std::isnan(x) || std::isnan(y);
			} else {
				return false;
			}
		}

		[[nodiscard]] FORCE_INLINE bool is_Inf() const noexcept{
			return std::isinf(x) || std::isinf(y);
		}

		[[nodiscard]] FORCE_INLINE float length() const noexcept {
			return math::dst(x, y);
		}

		[[nodiscard]] FORCE_INLINE constexpr T length2() const noexcept {
			return x * x + y * y;
		}

		/**
		 * @brief
		 * @return angle in [-180 deg, 180 deg]
		 */
		[[nodiscard]] FORCE_INLINE float angle() const noexcept {
			return angle_rad() * math::rad_to_deg;
		}


		[[nodiscard]] FORCE_INLINE floating_point_t angle_to(const_pass_t where) const noexcept {
			return (where - *this).angle();
		}

		[[nodiscard]] FORCE_INLINE floating_point_t angle_between(const_pass_t other) const noexcept {
			return math::atan2(this->cross(other), this->dot(other)) * math::rad_to_deg_v<floating_point_t>;
		}

		FORCE_INLINE vector2& normalize() noexcept {
			return div(length());
		}

		FORCE_INLINE vector2& to_sign() noexcept{
			x = math::sign<T>(x);
			y = math::sign<T>(y);

			return *this;
		}

		template <std::floating_point Fp>
		FORCE_INLINE constexpr vector2& rotate_rad(const Fp rad) noexcept{
			//  Matrix Multi
			//  cos rad		-sin rad	x    crx   -sry
			//	sin rad		 cos rad	y	 srx	cry
			return this->rotate(math::cos(rad), math::sin(rad));
		}

		template <std::floating_point Fp>
		FORCE_INLINE constexpr vector2& rotate(const Fp cos, const Fp sin) noexcept{
			if constexpr(std::floating_point<T>) {
				return this->set(cos * x - sin * y, sin * x + cos * y);
			}else {
				return this->set(
					static_cast<T>(cos * static_cast<Fp>(x) - sin * static_cast<Fp>(y)),
					static_cast<T>(sin * static_cast<Fp>(x) + cos * static_cast<Fp>(y))
				);
			}
		}

		template <std::floating_point Fp>
		FORCE_INLINE constexpr vector2& rotate(const Fp degree) noexcept{
			return this->rotate_rad(degree * math::deg_to_rad_v<Fp>);
		}

		FORCE_INLINE constexpr vector2& lerp(const_pass_t tgt, const floating_point_t alpha) noexcept{
			return this->set(math::lerp(x, tgt.x, alpha), math::lerp(y, tgt.y, alpha));
		}

		FORCE_INLINE vector2& approach(const_pass_t target, const mo_yanxi::number auto alpha) noexcept{
			vector2 approach = target - *this;
			const auto alpha2 = alpha * alpha;
			const auto len2 = approach.dst2();
			if (len2 <= alpha){
				return this->set(target);
			}

			const auto scl = std::sqrt(static_cast<floating_point_t>(alpha2) / static_cast<floating_point_t>(len2));
			return this->fma(approach, scl);
		}

		template <std::floating_point Fp>
		[[nodiscard]] FORCE_INLINE static constexpr vector2 from_polar_rad(const Fp angRad, const T length = 1) noexcept{
			return {length * math::cos(angRad), length * math::sin(angRad)};
		}

		template <std::floating_point Fp>
		[[nodiscard]] FORCE_INLINE static constexpr vector2 from_polar(const Fp angDeg, const T length = 1) noexcept{
			return vector2::from_polar_rad(angDeg * math::deg_to_rad_v<Fp>, length);
		}

		template <std::floating_point Fp>
		FORCE_INLINE constexpr vector2& set_polar(const Fp angDeg, const T length) noexcept{
			return vector2::set(length * math::cos(angDeg * math::deg_to_rad_v<Fp>), length * math::sin(angDeg * math::deg_to_rad_v<Fp>));
		}

		FORCE_INLINE constexpr vector2& set_polar(const float angDeg) noexcept{
			return set_polar(angDeg, length());
		}

		[[nodiscard]] FORCE_INLINE constexpr T dot(const_pass_t tgt) const noexcept{
			return x * tgt.x + y * tgt.y;
		}

		[[nodiscard]] FORCE_INLINE constexpr T cross(const_pass_t tgt) const noexcept{
			return x * tgt.y - y * tgt.x;
		}

		[[nodiscard]] FORCE_INLINE constexpr vector2 cross(const T val_zAxis) const noexcept{
			return {y * val_zAxis, -x * val_zAxis};
		}

		FORCE_INLINE constexpr vector2& project(const_pass_t tgt) noexcept{
			float scl = this->dot(tgt);

			return this->set(tgt).mul(scl / tgt.length2());
		}

		FORCE_INLINE constexpr vector2& project_scaled(const_pass_t tgt) noexcept {
			const float scl = this->dot(tgt);

			return this->set(tgt.x * scl, tgt.y * scl);
		}

		[[nodiscard]] FORCE_INLINE constexpr auto scalar_proj2(const_pass_t axis) const noexcept{
			const auto dot = this->dot(axis);
			return dot * dot / axis.length2();
		}

		[[nodiscard]] FORCE_INLINE auto scalar_proj(const_pass_t axis) const noexcept{
			return std::sqrt(this->scalar_proj2(axis));
		}

		FORCE_INLINE friend constexpr bool operator==(const vector2& lhs, const vector2& rhs) noexcept = default;

		FORCE_INLINE constexpr vector2& clamp_x(const T min, const T max) noexcept {
			x = math::clamp(x, min, max);
			return *this;
		}


		FORCE_INLINE constexpr vector2& clamp_xy(const_pass_t min, const_pass_t max) noexcept {
			x = math::clamp(x, min.x, max.x);
			y = math::clamp(y, min.y, max.y);
			return *this;
		}

		FORCE_INLINE constexpr vector2& clamp_y(const T min, const T max) noexcept {
			y = math::clamp(y, min, max);
			return *this;
		}

		FORCE_INLINE constexpr vector2& min_x(const T min) noexcept {
			x = math::min(x, min);
			return *this;
		}

		FORCE_INLINE constexpr vector2& min_y(const T min) noexcept {
			y = math::min(y, min);
			return *this;
		}

		FORCE_INLINE constexpr vector2& max_x(const T max) noexcept {
			x = math::max(x, max);
			return *this;
		}

		FORCE_INLINE constexpr vector2& max_y(const T max) noexcept {
			y = math::max(y, max);
			return *this;
		}


		FORCE_INLINE constexpr vector2& min(const_pass_t min) noexcept {
			x = math::min(x, min.x);
			y = math::min(y, min.y);
			return *this;
		}

		FORCE_INLINE constexpr vector2& max(const_pass_t max) noexcept {
			x = math::max(x, max.x);
			y = math::max(y, max.y);
			return *this;
		}

		// FORCE_INLINE constexpr vector2& clampNormalized() noexcept requires std::floating_point<T>{
		// 	return clamp_x(0, 1).clamp_y(0, 1);
		// }


		FORCE_INLINE vector2& to_abs() noexcept {
			if constexpr(!std::is_unsigned_v<T>) {
				x = math::abs(x);
				y = math::abs(y);
			}

			return *this;
		}

		FORCE_INLINE vector2& limit_y(const T yAbs) {
			y = math::clamp_range(y, yAbs);
			return *this;
		}

		FORCE_INLINE vector2& limit_x(const T xAbs) {
			x = math::clamp_range(x, xAbs);
			return *this;
		}

		FORCE_INLINE vector2& limit(const T xAbs, const T yAbs) noexcept {
			this->limit_x(xAbs);
			this->limit_y(yAbs);
			return *this;
		}

		FORCE_INLINE vector2& limit_max_length(const T limit) noexcept {
			return this->limit_max_length2(limit * limit);
		}

		FORCE_INLINE vector2& limit_max_length2(const T limit2) noexcept {
			const float len2 = length2();
			if (len2 == 0) [[unlikely]] return *this;

			if (len2 > limit2) {
				return this->scl(std::sqrt(static_cast<floating_point_t>(limit2) / static_cast<floating_point_t>(len2)));
			}

			return *this;
		}

		FORCE_INLINE vector2& limit_min_length(const T limit) noexcept {
			return this->limit_min_length2(limit * limit);
		}

		FORCE_INLINE vector2& limit_min_length2(const T limit2) noexcept {
			const float len2 = length2();
			if (len2 == 0) [[unlikely]] return *this;

			if (len2 < limit2) {
				return this->scl(std::sqrt(static_cast<float>(limit2) / static_cast<float>(len2)));
			}

			return *this;
		}

		FORCE_INLINE vector2& limit_length(const T min, const T max) noexcept {
			return this->limit_length2(min * min, max * max);
		}


		FORCE_INLINE vector2& limit_length2(const T min, const T max) noexcept {
			if (const float len2 = length2(); len2 < min) {
				return this->scl(std::sqrt(static_cast<float>(min) / static_cast<float>(len2)));
			}else if(len2 > max){
				return this->scl(std::sqrt(static_cast<float>(max) / static_cast<float>(len2)));
			}

			return *this;
		}

		template <std::floating_point V>
		FORCE_INLINE constexpr vector2& scl_round(const vector2<V> val) noexcept {
			V rstX = static_cast<V>(x) * val.x;
			V rstY = static_cast<V>(y) * val.y;
			return this->set(math::round<T>(rstX), math::round<T>(rstY));
		}

		template <number V>
		FORCE_INLINE constexpr vector2& scl(const V val) noexcept {
			return this->scl(val, val);
		}

		template <number V>
		FORCE_INLINE constexpr vector2& scl(const V ox, const V oy) noexcept {
			x = static_cast<T>(static_cast<V>(x) * ox);
			y = static_cast<T>(static_cast<V>(y) * oy);
			return *this;
		}

		FORCE_INLINE constexpr vector2& flip_x() noexcept {
			x = -x;
			return *this;
		}

		FORCE_INLINE constexpr vector2& flip_y() noexcept {
			y = -y;
			return *this;
		}

		FORCE_INLINE vector2& set_length(const T len) noexcept {
			return this->set_length2(len * len);
		}

		FORCE_INLINE vector2& set_length2(const T len2) noexcept {
			const float oldLen2 = length2();
			return oldLen2 == 0 || oldLen2 == len2 ? *this : this->scl(std::sqrt(len2 / oldLen2));  // NOLINT(clang-diagnostic-float-equal)
		}


		[[nodiscard]] FORCE_INLINE floating_point_t angle_rad() const noexcept{
			return math::atan2(static_cast<floating_point_t>(y), static_cast<floating_point_t>(x));
		}

		// [[nodiscard]] float angleExact() const noexcept{
		// 	return std::atan2(y, x)/* * math::DEGREES_TO_RADIANS*/;
		// }

		/**
		 * \brief clockwise rotation
		 */
		FORCE_INLINE constexpr vector2& rotate_rt_clockwise() noexcept requires std::is_signed_v<T> {
			return this->set(-y, x);
		}

		/**
		 * @brief
		 * @param signProv > 0: counterClockwise; < 0: clockWise; = 0: do nothing
		 * @return
		 */
		template <mo_yanxi::number S>
		FORCE_INLINE constexpr vector2& rotate_rt_with(const S signProv) noexcept requires std::is_signed_v<T> {
			const int sign = math::sign(signProv);
			if(sign){
				return this->set(y * sign, -x * sign);
			}else [[unlikely]]{
				return *this;
			}
		}

		/**
		 * \brief clockwise rotation
		 */
		[[deprecated]] FORCE_INLINE constexpr vector2& rotate_rt() noexcept requires std::is_signed_v<T> {
			return rotate_rt_clockwise();
		}

		/**
		 * \brief counterclockwise rotation
		 */
		FORCE_INLINE constexpr vector2& rotate_rt_counter_clockwise() noexcept requires std::is_signed_v<T> {
			return this->set(y, -x);
		}

		FORCE_INLINE vector2& round_by(const_pass_t other) noexcept requires std::is_floating_point_v<T>{
			x = math::round<T>(x, other.x);
			y = math::round<T>(y, other.y);

			return *this;
		}

		FORCE_INLINE vector2& round_by(const T val) noexcept requires std::is_floating_point_v<T>{
			x = math::round<T>(x, val);
			y = math::round<T>(y, val);

			return *this;
		}

		template <typename N>
		FORCE_INLINE vector2<N> round_with(typename vector2<N>::const_pass_t other) const noexcept{
			vector2<N> tgt = as<N>();
			tgt.x = math::round<N>(static_cast<N>(x), other.x);
			tgt.y = math::round<N>(static_cast<N>(y), other.y);

			return tgt;
		}

		template <typename N>
		FORCE_INLINE vector2<N> round_with(const N val) const noexcept{
			vector2<N> tgt = as<N>();
			tgt.x = math::round<N>(static_cast<N>(x), val);
			tgt.y = math::round<N>(static_cast<N>(y), val);

			return tgt;
		}


		template <typename N>
		[[nodiscard]] FORCE_INLINE vector2<N> round() const noexcept requires std::is_floating_point_v<T>{
			vector2<N> tgt = as<N>();
			tgt.x = math::round<N>(x);
			tgt.y = math::round<N>(y);

			return tgt;
		}

		template <std::integral N>
		[[nodiscard]] FORCE_INLINE vector2<N> trunc() const noexcept{
			return vector2<N>{math::trunc_right<N>(x), math::trunc_right<N>(y)};
		}

		[[nodiscard]] FORCE_INLINE constexpr auto area() const noexcept {
			return x * y;
		}

		[[nodiscard]] FORCE_INLINE constexpr bool is_zero() const noexcept {
			return x == static_cast<T>(0) && y == static_cast<T>(0);
		}

		[[nodiscard]] FORCE_INLINE constexpr bool is_zero(const T margin) const noexcept{
			return length2() < margin;
		}

		[[nodiscard]] FORCE_INLINE constexpr vector2 sign() const noexcept{
			return {math::sign(x), math::sign(y)};
		}

		[[nodiscard]] FORCE_INLINE constexpr bool within(const_pass_t other_in_axis) const noexcept{
			return x < other_in_axis.x && y < other_in_axis.y;
		}

		[[nodiscard]] FORCE_INLINE constexpr bool beyond(const_pass_t other_in_axis) const noexcept{
			return x > other_in_axis.x || y > other_in_axis.y;
		}

		[[nodiscard]] FORCE_INLINE constexpr bool within_equal(const_pass_t other_in_axis) const noexcept{
			return x <= other_in_axis.x && y <= other_in_axis.y;
		}

		[[nodiscard]] FORCE_INLINE constexpr bool beyond_equal(const_pass_t other_in_axis) const noexcept{
			return x >= other_in_axis.x || y >= other_in_axis.y;
		}

		[[nodiscard]] vector2 ortho_dir() const noexcept requires (mo_yanxi::signed_number<T>){
			static constexpr T Zero = static_cast<T>(0);
			static constexpr T One = static_cast<T>(1);
			static constexpr T NOne = static_cast<T>(-1);
			if(x >= Zero && y >= Zero){
				const bool onX = x > y;
				return {
					onX ? One : Zero,
					onX ? Zero : One,
				};
			}

			if(x <= Zero && y <= Zero){
				const bool onX = x < y;
				return {
					onX ? NOne : Zero,
					onX ? Zero : NOne,
				};
			}

			if(x > Zero && y < Zero){
				const bool onX = x > -y;
				return {
					onX ? One : Zero,
					onX ? Zero : NOne,
				};
			}

			if(x < Zero && y > Zero){
				const bool onX = -x > y;
				return {
					onX ? NOne : Zero,
					onX ? Zero : One,
				};
			}

			return {};
		}

		constexpr auto operator<=>(const vector2&) const = default;
		// auto constexpr operator<=>(ConstPassType v) const noexcept {
		// 	T len = length2();
		// 	T lenO = v.length2();
		//
		// 	if(len > lenO) {
		// 		return std::weak_ordering::greater;
		// 	}
		//
		// 	if(len < lenO) {
		// 		return std::weak_ordering::less;
		// 	}
		//
		// 	return std::weak_ordering::equivalent;
		// }

		template <mo_yanxi::number TN>
		[[nodiscard]] FORCE_INLINE constexpr vector2<TN> as() const noexcept{
			if constexpr (std::same_as<TN, T>){
				return *this;
			}else{
				return vector2<TN>{static_cast<TN>(x), static_cast<TN>(y)};
			}
		}
		template <spec_of<vector2> TN>
		[[nodiscard]] FORCE_INLINE constexpr auto as() const noexcept{
			return as<typename TN::value_type>();
		}

		template <mo_yanxi::number TN>
		[[nodiscard]] FORCE_INLINE constexpr explicit operator vector2<TN>() const noexcept{
			return as<TN>();
		}

		[[nodiscard]] FORCE_INLINE constexpr auto as_signed() const noexcept{
			if constexpr (std::is_unsigned_v<T>){
				using S = std::make_signed_t<T>;
				return vector2<S>{static_cast<S>(x), static_cast<S>(y)};
			}else{
				return *this;
			}
		}

		friend std::ostream& operator<<(std::ostream& os, const_pass_t obj) {
			return os << '(' << std::to_string(obj.x) << ", " << std::to_string(obj.y) << ')';
		}

		[[nodiscard]] FORCE_INLINE constexpr bool equals(const_pass_t other) const noexcept{
			if constexpr (std::is_integral_v<T>){
				return *this == other;
			}else{
				return this->within(other, static_cast<T>(math::FLOATING_ROUNDING_ERROR));
			}
		}

		[[nodiscard]] FORCE_INLINE constexpr bool equals(const_pass_t other, const T margin) const noexcept{
			return this->within(other, margin);
		}

		/**
		 * @brief
		 * @return y / x
		 */
		[[nodiscard]] FORCE_INLINE constexpr T slope() const noexcept{
			return y / x;
		}


		/**
		 * @brief
		 * @return x / y
		 */
		[[nodiscard]] FORCE_INLINE constexpr T slope_inv() const noexcept{
			return x / y;
		}

		constexpr auto operator<=>(const vector2&) const noexcept requires (std::integral<T>) = default;

		template <typename Ty>
			requires std::constructible_from<Ty, T, T>
		FORCE_INLINE explicit constexpr operator Ty() const noexcept{
			return Ty(x, y);
		}

		template <typename N1, typename N2>
		[[nodiscard]] FORCE_INLINE constexpr bool axis_less(const N1 ox, const N2 oy) const noexcept{
			if constexpr(std::floating_point<N1> || std::floating_point<N2> || std::floating_point<T>){
				using cmt = std::common_type_t<T, N1, N2>;

				return static_cast<cmt>(x) < static_cast<cmt>(ox) && static_cast<cmt>(y) < static_cast<cmt>(oy);
			} else{
				return std::cmp_less(x, ox) && std::cmp_less(y, oy);
			}
		}

		template <typename N>
		[[nodiscard]] FORCE_INLINE constexpr bool axis_less(typename vector2<N>::const_pass_t other) const noexcept{
			return this->axis_less(other.x, other.y);
		}

		template <typename N1, typename N2>
		[[nodiscard]] FORCE_INLINE constexpr bool axis_greater(const N1 ox, const N2 oy) const noexcept{
			if constexpr(std::floating_point<N1> || std::floating_point<N2> || std::floating_point<T>){
				using cmt = std::common_type_t<T, N1, N2>;

				return static_cast<cmt>(x) > static_cast<cmt>(ox) && static_cast<cmt>(y) > static_cast<cmt>(oy);
			} else{
				return std::cmp_greater(x, ox) && std::cmp_greater(y, oy);
			}
		}

		template <typename N>
		[[nodiscard]] FORCE_INLINE constexpr bool axis_greater(typename vector2<N>::const_pass_t other) const noexcept{
			return this->axis_greater(other.x, other.y);
		}
	};


	using vec2 = vector2<float>;
	using bool2 = vector2<bool>;
	/**
	 * @brief indicate a normalized vector2<float>
	 */
	using nor_vec2 = vector2<float>;

	using point2 = vector2<int>;
	using iszie2 = vector2<int>;
	using uszie2 = vector2<unsigned int>;
	using upoint2 = vector2<unsigned int>;
	using short_point2 = vector2<short>;
	using ushort_point2 = vector2<unsigned short>;
	using u32size2 = vector2<std::uint32_t>;


	namespace vectors{
		template <typename N = float>
		vector2<N> hit_normal(const vector2<N> approach_direction, const vector2<N> tangent){
			auto p = std::copysign(1, tangent.cross(approach_direction));
			return tangent.cross(p);
		}

		constexpr vec2 direction(const float angle_Degree) noexcept{
			return {math::cos_deg(angle_Degree), math::sin_deg(angle_Degree)};
		}

		template <typename T>
		struct constant2{
			static constexpr vector2<T> base_vec2{1, 1};
			static constexpr vector2<T> zero_vec2{0, 0};
			static constexpr vector2<T> x_vec2{1, 0};
			static constexpr vector2<T> y_vec2{0, 1};

			// static constexpr Vector2D<T> left_vec2{-1, 0};
			// static constexpr Vector2D<T> right_vec2{1, 0};
			// static constexpr Vector2D<T> up_vec2{0, 1};
			// static constexpr Vector2D<T> down_vec2{0, -1};
			//
			// static constexpr Vector2D<T> front_vec2{1, 0};
			// static constexpr Vector2D<T> back_vec2{-1, 0};

			static constexpr vector2<T> max_vec2{std::numeric_limits<T>::max(), std::numeric_limits<T>::max()};
			static constexpr vector2<T> min_vec2{std::numeric_limits<T>::min(), std::numeric_limits<T>::min()};
			static constexpr vector2<T> lowest_vec2{std::numeric_limits<T>::lowest(), std::numeric_limits<T>::lowest()};

			static constexpr vector2<T> SNaN_vec2{std::numeric_limits<T>::signaling_NaN(), std::numeric_limits<T>::signaling_NaN()};
			static constexpr vector2<T> QNaN_vec2{std::numeric_limits<T>::quiet_NaN(), std::numeric_limits<T>::quiet_NaN()};
		};
	}
}


template <typename T>
struct std::hash<mo_yanxi::math::vector2<T>>{
	constexpr std::size_t operator()(typename mo_yanxi::math::vector2<T>::const_pass_t v) const noexcept{
		return v.hash_value();
	}
};

// export
// 	template<>
// 	struct std::hash<mo_yanxi::math::vec2>{
// 		constexpr std::size_t operator()(const mo_yanxi::math::vec2& v) const noexcept {
// 			return v.hash_value();
// 		}
// 	};
//
// export
// 	template<>
// 	struct std::hash<mo_yanxi::math::point2>{
// 		constexpr std::size_t operator()(const mo_yanxi::math::point2& v) const noexcept {
// 			return v.hash_value();
// 		}
// 	};
//
// export
// 	template<>
// 	struct std::hash<mo_yanxi::math::upoint2>{
// 		constexpr std::size_t operator()(const mo_yanxi::math::upoint2& v) const noexcept {
// 			return v.hash_value();
// 		}
// 	};
//
// export
// 	template<>
// 	struct std::hash<mo_yanxi::math::ushort_point2>{
// 		constexpr std::size_t operator()(const mo_yanxi::math::ushort_point2& v) const noexcept {
// 			return v.hash_value();
// 		}
// 	};



template <typename T>
struct formatter_base{
	static constexpr std::array wrapper{
		std::pair{std::string_view{"("}, std::string_view{")"}},
		std::pair{std::string_view{"["}, std::string_view{"]"}},
		std::pair{std::string_view{"{"}, std::string_view{"}"}},
	};

	std::size_t wrapperIndex{};
	std::formatter<T> formatter{};

	template <typename CharT>
	constexpr auto parse(std::basic_format_parse_context<CharT>& context) {
		auto begin = context.begin();
		switch(*begin){
			case ')' : [[fallthrough]];
			case '(' :{
				wrapperIndex = 0;
				++begin;
				break;
			}

			case '[' : [[fallthrough]];
			case ']' :{
				wrapperIndex = 1;
				++begin;
				break;
			}
			//
			// case '{' : [[fallthrough]];
			// case '}' :{
			// 	wrapperIndex = 2;
			// 	++begin;
			// 	break;
			// }

			case '!' :{
				wrapperIndex = wrapper.size();
				++begin;
				break;
			}

			default: break;
		}

		context.advance_to(begin);
		return formatter.parse(context);
	}

	template <typename OutputIt, typename CharT>
	auto format(typename mo_yanxi::math::vector2<T>::const_pass_t p, std::basic_format_context<OutputIt, CharT>& context) const{
		if(wrapperIndex != wrapper.size())std::format_to(context.out(), "{}", wrapper[wrapperIndex].first);
		formatter.format(p.x, context);
		std::format_to(context.out(), ", ");
		formatter.format(p.y, context);
		if(wrapperIndex != wrapper.size())std::format_to(context.out(), "{}", wrapper[wrapperIndex].second);
		return context.out();
	}
};

template <typename T>
struct std::formatter<mo_yanxi::math::vector2<T>>: formatter_base<T>{

};

// #define FMT(type) export template <> ;
//
// FMT(float);
// FMT(int);
// FMT(unsigned);
// FMT(std::size_t);
// FMT(bool);



