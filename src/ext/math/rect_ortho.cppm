module;

#include <cassert>
#include <immintrin.h>
#include "../adapted_attributes.hpp"

export module mo_yanxi.math.rect_ortho;

import std;

import mo_yanxi.concepts;
export import mo_yanxi.math.vector2;
export import mo_yanxi.tags;
import mo_yanxi.math;

namespace mo_yanxi::math{
	export
	template <typename T>
	struct rect_ortho_trivial{
	public:
		using vec_t = vector2<T>;

		vec_t src;
		vec_t extent;

		constexpr rect_ortho_trivial& flip() noexcept{
			src += extent;
			extent = -extent;
			return *this;
		}

		constexpr vec_t get_abs_extent() const noexcept{
			return extent.copy().to_abs();
		}

		constexpr static rect_ortho_trivial from_vert(vec_t begin, vec_t end) noexcept{
			return rect_ortho_trivial{begin, end - begin};
		}


		[[nodiscard]] FORCE_INLINE constexpr vec_t vert_00() const noexcept{
			return src;
		}

		[[nodiscard]] FORCE_INLINE constexpr vec_t vert_10() const noexcept{
			return {src.x + extent.x, src.y};
		}

		[[nodiscard]] FORCE_INLINE constexpr vec_t vert_01() const noexcept{
			return {src.x, src.y + extent.y};
		}

		[[nodiscard]] FORCE_INLINE constexpr vec_t vert_11() const noexcept{
			return src + extent;
		}

		FORCE_INLINE rect_ortho_trivial& expand(vec_t expansion) noexcept{
			extent += expansion * 2;
			src -= expansion;
			return *this;
		}


		FORCE_INLINE rect_ortho_trivial& shrink(vec_t shrinsion) noexcept{
			extent -= shrinsion * 2;
			src += shrinsion;
			return *this;
		}
		FORCE_INLINE rect_ortho_trivial& move(vec_t mov) noexcept{
			src += mov;
			return *this;
		}


	};
	/**
	 * \brief width, height should be always non-negative.
	 * \tparam T Arithmetic Type
	 */
	export
	template <mo_yanxi::arithmetic T>
	class rect_ortho{
		static constexpr T TWO{2};

	public:
		using vec_t = vector2<T>;

		vec_t src{};

	private:
		//TODO make it unsigned?
		vec_t size_{};

	public:
		constexpr rect_ortho(tags::unchecked_t, const T srcX, const T srcY, const T width, const T height) noexcept
			: src(srcX, srcY), size_{width, height}{
			assert(width >= 0 && height >= 0);
		}

		constexpr rect_ortho(const T srcX, const T srcY, const T width, const T height) noexcept
			: src(srcX, srcY){
			this->set_size(width, height);
		}

		constexpr rect_ortho(typename vec_t::const_pass_t center, const T width, const T height) noexcept{
			this->set_size(width, height);
			this->set_center(center.x, center.y);
		}

		constexpr rect_ortho(const T width, const T height) noexcept{
			this->set_size(width, height);
		}

		constexpr explicit rect_ortho(typename vec_t::const_pass_t size) noexcept{
			this->set_size(size);
		}

		constexpr rect_ortho(typename vec_t::const_pass_t center, const T size) noexcept{
			this->set_size(size, size);
			this->set_center(center.x, center.y);
		}

		/**
		 * @warning Create by vertex [src, end] instead of [src, size]
		 */
		constexpr rect_ortho(typename vec_t::const_pass_t src, typename vec_t::const_pass_t end) noexcept{
			this->set_vert(src, end);
		}
		/**
		 * @warning Create by vertex [src, end] instead of [src, size]
		 */
		constexpr rect_ortho(tags::unchecked_t, typename vec_t::const_pass_t bot_lft, typename vec_t::const_pass_t top_rit) noexcept :
		src(bot_lft), size_(top_rit - bot_lft){
			assert(bot_lft.within_equal(top_rit));
		}		/**
		 * @warning Create by vertex [src, end] instead of [src, size]
		 */
		constexpr rect_ortho(tags::unchecked_t, tags::from_extent_t, typename vec_t::const_pass_t src, typename vec_t::const_pass_t sz) noexcept :
			src(src), size_(sz){
			assert(sz.x >= 0);
			assert(sz.y >= 0);
		}

		/**
		 * @brief Create by [src, size]
		 */
		constexpr rect_ortho(tags::from_extent_t,
			typename vec_t::const_pass_t src,
			typename vec_t::const_pass_t size) noexcept
			: src(src){
			this->set_size(size);
		}

		constexpr rect_ortho(tags::from_extent_t,
			typename vec_t::const_pass_t src, const T width, const T height) noexcept
			: src(src){
			this->set_size(width, height);
		}

		constexpr explicit rect_ortho(const T size) noexcept{
			this->set_size(size, size);
		}

		constexpr rect_ortho() noexcept = default;

		friend constexpr bool operator==(const rect_ortho& lhs, const rect_ortho& rhs) noexcept = default;

		friend std::ostream& operator<<(std::ostream& os, const rect_ortho& obj) noexcept{
			return std::print(os, "[{}, {}, {}, {}]", obj.src.x, obj.src.y, obj.size_.x, obj.size_.y);
		}

		[[nodiscard]] FORCE_INLINE constexpr T get_src_x() const noexcept{
			return src.x;
		}

		[[nodiscard]] FORCE_INLINE constexpr vec_t get_src() const noexcept{
			return src;
		}

		[[nodiscard]] FORCE_INLINE constexpr vec_t get_end() const noexcept{
			return src + size_;
		}

		[[nodiscard]] FORCE_INLINE constexpr rect_ortho& snap_to_wrap_bound_of(T tile_size) const noexcept{
			auto end = get_end();
			src.floor(tile_size);
			end.ceil(tile_size);
			size_ = end - src;
			return *this;
		}


		/**
		 * @brief set this to the minimum wrapper rect for original this and other
		 * @param other another rectangle
		 * @return *this
		 */
		FORCE_INLINE constexpr rect_ortho& expand_by(const rect_ortho& other) noexcept{
			vec_t min{math::min(get_src_x(), other.get_src_x()), math::min(get_src_y(), other.get_src_y())};
			vec_t max{math::max(get_end_x(), other.get_end_x()), math::max(get_end_y(), other.get_end_y())};

			src = min;
			size_ = max - min;

			return *this;
		}

		[[nodiscard]] FORCE_INLINE constexpr T get_src_y() const noexcept{
			return src.y;
		}

		[[nodiscard]] FORCE_INLINE constexpr T width() const noexcept{
			return size_.x;
		}

		[[nodiscard]] FORCE_INLINE constexpr vec_t extent() const noexcept{
			return size_;
		}

		[[nodiscard]] FORCE_INLINE constexpr T height() const noexcept{
			return size_.y;
		}

		template <mo_yanxi::arithmetic T_>
		[[nodiscard]] FORCE_INLINE constexpr rect_ortho<T_> as() const noexcept{
			return rect_ortho<T_>{
					static_cast<T_>(src.x),
					static_cast<T_>(src.y),
					static_cast<T_>(size_.x),
					static_cast<T_>(size_.y),
				};
		}


		[[nodiscard]] FORCE_INLINE constexpr auto as_signed() const noexcept{
			if constexpr(std::is_unsigned_v<T>){
				using S = std::make_signed_t<T>;
				return rect_ortho<S>{
						static_cast<S>(src.x),
						static_cast<S>(src.y),
						static_cast<S>(size_.x),
						static_cast<S>(size_.y)
					};
			} else{
				return *this;
			}
		}

		template <mo_yanxi::arithmetic N>
		FORCE_INLINE constexpr auto& set_width(const N w) noexcept{
			if constexpr(std::is_unsigned_v<N>){
				size_.x = static_cast<T>(w);
			} else{
				if(w >= 0){
					size_.x = static_cast<T>(w);
				} else{
					T abs = -static_cast<T>(w);
					src.x -= abs;
					size_.x = abs;
				}
			}

			return *this;
		}

		template <mo_yanxi::arithmetic N>
		FORCE_INLINE constexpr auto& set_height(const N h) noexcept{
			if constexpr(std::is_unsigned_v<N>){
				size_.y = static_cast<T>(h);
			} else{
				if(h >= 0){
					size_.y = static_cast<T>(h);
				} else{
					T abs = -static_cast<T>(h);
					src.y -= abs;
					size_.y = abs;
				}
			}

			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& add_size(const T x, const T y) noexcept requires (mo_yanxi::signed_number<T>){
			this->set_width<T>(size_.x + x);
			this->set_height<T>(size_.y + y);

			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& add_width(const T x) noexcept{
			this->set_width<T>(size_.x + x);

			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& add_height(const T y) noexcept{
			this->set_height<T>(size_.y + y);

			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& shrink_by(typename vec_t::const_pass_t directionAndSize) noexcept{
			const T minX = math::min(math::abs(directionAndSize.x), size_.x);
			size_.x -= minX;

			if(directionAndSize.x > 0){
				src.x += minX;
			}

			const T minY = math::min(math::abs(directionAndSize.y), size_.y);
			size_.y -= minY;

			if(directionAndSize.y > 0){
				src.y += minY;
			}

			return *this;
		}

		template <mo_yanxi::arithmetic N>
		FORCE_INLINE constexpr rect_ortho& add_size(const N x, const N y) noexcept{
			using S = std::make_signed_t<T>;
			this->set_width<S>(static_cast<S>(size_.x) + static_cast<S>(x));
			this->set_height<S>(static_cast<S>(size_.y) + static_cast<S>(y));

			return *this;
		}

		FORCE_INLINE constexpr void set_larger_width(const T v) noexcept{
			if constexpr(std::is_unsigned_v<T>){
				if(v > size_.x){
					this->set_width<T>(v);
				}
			} else{
				T abs = static_cast<T>(v < 0 ? -v : v);
				if(abs > size_.x){
					this->set_width<T>(v);
				}
			}
		}

		FORCE_INLINE constexpr void set_larger_height(const T v) noexcept{
			if constexpr(std::is_unsigned_v<T>){
				if(v > size_.y){
					this->set_height<T>(v);
				}
			} else{
				T abs = static_cast<T>(v < 0 ? -v : v);
				if(abs > size_.y){
					this->set_height<T>(v);
				}
			}
		}

		FORCE_INLINE constexpr void set_shorter_width(const T v) noexcept{
			if constexpr(std::is_unsigned_v<T>){
				if(v < size_.x){
					this->set_width<T>(v);
				}
			} else{
				T abs = static_cast<T>(v < 0 ? -v : v);
				if(abs < size_.x){
					this->set_width<T>(v);
				}
			}
		}

		FORCE_INLINE constexpr void set_shorter_height(const T v) noexcept{
			if constexpr(std::is_unsigned_v<T>){
				if(v < size_.y){
					this->set_height<T>(v);
				}
			} else{
				T abs = static_cast<T>(v < 0 ? -v : v);
				if(abs < size_.y){
					this->set_height<T>(v);
				}
			}
		}

		[[nodiscard]] FORCE_INLINE constexpr bool contains_strict(const rect_ortho& other) const noexcept{
			return
				other.src.x > src.x && other.src.x + other.size_.x < src.x + size_.x &&
				other.src.y > src.y && other.src.y + other.size_.y < src.y + size_.y;
		}

		[[nodiscard]] FORCE_INLINE constexpr bool contains_loose(const rect_ortho& other) const noexcept{
			return
				other.src.x >= src.x && other.get_end_x() <= get_end_x() &&
				other.src.y >= src.y && other.get_end_y() <= get_end_y();
		}

		[[nodiscard]] FORCE_INLINE constexpr bool contains(const rect_ortho& other) const noexcept{
			return
				other.src.x >= src.x && other.get_end_x() < get_end_x() &&
				other.src.y >= src.y && other.get_end_y() < get_end_y();
		}

		[[nodiscard]] FORCE_INLINE constexpr bool contains_strict(typename vec_t::const_pass_t v) const noexcept{
			return v.x > src.x && v.y > src.y && v.x < get_end_x() && v.y < get_end_y();

		}

		[[nodiscard]] FORCE_INLINE constexpr bool contains_loose(typename vec_t::const_pass_t v) const noexcept{
			return v.x >= src.x && v.y >= src.y && v.x <= get_end_x() && v.y <= get_end_y();
		}

		[[nodiscard]] FORCE_INLINE constexpr bool contains(typename vec_t::const_pass_t v) const noexcept{
			return v.x >= src.x && v.y >= src.y && v.x < get_end_x() && v.y < get_end_y();
		}

		[[nodiscard]] FORCE_INLINE constexpr bool overlap_exclusive(this const rect_ortho& l, const rect_ortho& r) noexcept{
			if constexpr(std::floating_point<T>){
				if !consteval{
					const float a_src_x = l.get_src_x();
					const float a_end_x = l.get_end_x();
					const float a_src_y = l.get_src_y();
					const float a_end_y = l.get_end_y();

					// 提取矩形r的坐标
					const float b_src_x = r.get_src_x();
					const float b_end_x = r.get_end_x();
					const float b_src_y = r.get_src_y();
					const float b_end_y = r.get_end_y();

					// 构造比较向量：
					// vec1 = [a_src_x, a_src_y, b_src_x, b_src_y]
					// vec2 = [b_end_x, b_end_y, a_end_x, a_end_y]
					__m128 vec1 = _mm_set_ps(b_src_y, b_src_x, a_src_y, a_src_x);
					__m128 vec2 = _mm_set_ps(a_end_y, a_end_x, b_end_y, b_end_x);

					// 并行比较所有条件：vec1 <= vec2
					__m128 cmp = _mm_cmplt_ps(vec1, vec2);

					// 将比较结果转换为位掩码
					int mask = _mm_movemask_ps(cmp);

					// 检查所有四个条件是否均满足（掩码为0b1111）
					return (mask == 0x0F);
				}
			}

			return
					l.get_src_x() < r.get_end_x() &&
					l.get_end_x() > r.get_src_x() &&
					l.get_src_y() < r.get_end_y() &&
					l.get_end_y() > r.get_src_y();
		}

		[[nodiscard]] FORCE_INLINE constexpr bool overlap_inclusive(
			this const rect_ortho& l,
			const rect_ortho& r) noexcept{
			if constexpr(std::floating_point<T>){
				if !consteval{
					const float a_src_x = l.get_src_x();
					const float a_end_x = l.get_end_x();
					const float a_src_y = l.get_src_y();
					const float a_end_y = l.get_end_y();

					// 提取矩形r的坐标
					const float b_src_x = r.get_src_x();
					const float b_end_x = r.get_end_x();
					const float b_src_y = r.get_src_y();
					const float b_end_y = r.get_end_y();

					// 构造比较向量：
					// vec1 = [a_src_x, a_src_y, b_src_x, b_src_y]
					// vec2 = [b_end_x, b_end_y, a_end_x, a_end_y]
					__m128 vec1 = _mm_set_ps(b_src_y, b_src_x, a_src_y, a_src_x);
					__m128 vec2 = _mm_set_ps(a_end_y, a_end_x, b_end_y, b_end_x);

					// 并行比较所有条件：vec1 <= vec2
					__m128 cmp = _mm_cmple_ps(vec1, vec2);

					// 将比较结果转换为位掩码
					int mask = _mm_movemask_ps(cmp);

					// 检查所有四个条件是否均满足（掩码为0b1111）
					return (mask == 0x0F);
				}
			}

			return
				l.get_src_x() <= r.get_end_x() &&
				l.get_end_x() >= r.get_src_x() &&
				l.get_src_y() <= r.get_end_y() &&
				l.get_end_y() >= r.get_src_y();
		}

		[[nodiscard]] FORCE_INLINE constexpr bool overlap_exclusive(
			const rect_ortho& r, typename vec_t::const_pass_t selfTrans,
		                                               typename vec_t::const_pass_t otherTrans) const noexcept{
			return
				get_src_x() + selfTrans.x < r.get_end_x() + otherTrans.x &&
				get_end_x() + selfTrans.x > r.get_src_x() + otherTrans.x &&
				get_src_y() + selfTrans.y < r.get_end_y() + otherTrans.y &&
				get_end_y() + selfTrans.y > r.get_src_y() + otherTrans.y;
		}

		[[nodiscard]] FORCE_INLINE constexpr T get_end_x() const noexcept{
			return src.x + size_.x;
		}

		[[nodiscard]] FORCE_INLINE constexpr T get_end_y() const noexcept{
			return src.y + size_.y;
		}

		[[nodiscard]] FORCE_INLINE constexpr T get_center_x() const noexcept{
			return src.x + size_.x / TWO;
		}

		[[nodiscard]] FORCE_INLINE constexpr T get_center_y() const noexcept{
			return src.y + size_.y / TWO;
		}

		[[nodiscard]] FORCE_INLINE constexpr vec_t get_center() const noexcept{
			return src + size_ / TWO;
		}

		[[nodiscard]] FORCE_INLINE constexpr T diagonal_len2() const noexcept{
			return size_.length2();
		}

		FORCE_INLINE constexpr rect_ortho& set_src(const T x, const T y) noexcept{
			src.x = x;
			src.y = y;

			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& set_end(const T x, const T y) noexcept{
			this->set_end_x(x);
			this->set_end_y(y);

			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& set_end_x(const T x) noexcept{
			this->set_width(x - src.x);

			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& set_end_y(const T y) noexcept{
			this->set_height(y - src.y);

			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& set_src(typename vec_t::const_pass_t v) noexcept{
			src = v;
			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& set_size(const T x, const T y) noexcept{
			this->set_width(x);
			this->set_height(y);

			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& set_size(const rect_ortho& other) noexcept{
			this->set_width(other.size_.x);
			this->set_height(other.size_.y);

			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& move_y(const T y) noexcept{
			src.y += y;

			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& move_x(const T x) noexcept{
			src.x += x;

			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& move(const T x, const T y) noexcept{
			src.x += x;
			src.y += y;

			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& move(typename vec_t::const_pass_t vec) noexcept{
			src += vec;

			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& set_src(const rect_ortho& other) noexcept{
			src = other.src;

			return *this;
		}

		template <mo_yanxi::arithmetic T1, mo_yanxi::arithmetic T2>
		FORCE_INLINE constexpr rect_ortho& scl_size(const T1 xScl, const T2 yScl) noexcept{
			if constexpr (std::unsigned_integral<T>){
				CHECKED_ASSUME(xScl >= 0);
				CHECKED_ASSUME(yScl >= 0);
			}
			
			size_.template scl<std::common_type_t<T1, T2>>(xScl, yScl);
			
			if(xScl < 0){
				src.x += size_.x;
				size_.x = -size_.x;
			}
			
			if(yScl < 0){
				src.y += size_.y;
				size_.y = -size_.y;
			}

			return *this;
		}

		template <mo_yanxi::arithmetic T1, mo_yanxi::arithmetic T2>
		FORCE_INLINE constexpr rect_ortho& scl_pos(const T1 xScl, const T2 yScl) noexcept{
			src.scl(xScl, yScl);

			return *this;
		}

		template <mo_yanxi::arithmetic T1, mo_yanxi::arithmetic T2>
		FORCE_INLINE constexpr rect_ortho& scl(const T1 xScl, const T2 yScl) noexcept{
			(void)this->scl_pos<T1, T2>(xScl, yScl);
			(void)this->scl_size<T1, T2>(xScl, yScl);

			return *this;
		}

		template <mo_yanxi::arithmetic N>
		FORCE_INLINE constexpr rect_ortho& scl(const vector2<N>& scl) noexcept{
			(void)this->scl_pos<N, N>(scl.x, scl.y);
			(void)this->scl_size<N, N>(scl.x, scl.y);

			return *this;
		}

		FORCE_INLINE constexpr void set(const T srcx, const T srcy, const T width, const T height) noexcept{
			src.x = srcx;
			src.y = srcy;

			this->set_width<T>(width);
			this->set_height<T>(height);
		}

		FORCE_INLINE constexpr rect_ortho& trunc() noexcept{
			src.trunc();
			size_.trunc();
			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& trunc(T step) noexcept{
			src.trunc(step);
			size_.trunc(step);
			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& trunc(const vector2<T>& step) noexcept{
			src.trunc(step);
			size_.trunc(step);
			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& trunc_vert() noexcept{
			auto v00 = vert_00();
			auto v11 = vert_11();
			v00.trunc();
			v11.trunc();
			return this->set_vert(v00, v11);;
		}

		FORCE_INLINE constexpr rect_ortho& trunc_vert(T step) noexcept{
			auto v00 = vert_00();
			auto v11 = vert_11();
			v00.trunc(step);
			v11.trunc(step);
			return this->set_vert(v00, v11);
		}

		FORCE_INLINE constexpr rect_ortho& trunc_vert(const vector2<T>& step) noexcept{
			auto v00 = vert_00();
			auto v11 = vert_11();
			v00.trunc(step);
			v11.trunc(step);
			return this->set_vert(v00, v11);
		}

		template <std::integral N>
		[[nodiscard]] FORCE_INLINE rect_ortho<N> round() const noexcept{
			return rect_ortho<N>{
					math::round<N>(src.x), math::round<N>(src.y), math::round<N>(size_.x), math::round<N>(size_.y)
			};
		}

		FORCE_INLINE constexpr rect_ortho& set_size(typename vec_t::const_pass_t v) noexcept{
			return this->set_size(v.x, v.y);
		}

		FORCE_INLINE constexpr rect_ortho& set_center(const T x, const T y) noexcept{
			this->set_src(x - size_.x / TWO, y - size_.y / TWO);

			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& set_center(typename vec_t::const_pass_t v) noexcept{
			this->set_src(v.x - size_.x / TWO, v.y - size_.y / TWO);

			return *this;
		}

		template <std::floating_point Fp>
		[[nodiscard]] FORCE_INLINE constexpr Fp x_offset_ratio(const Fp x) const noexcept{
			return math::curve(x, static_cast<Fp>(src.x), static_cast<Fp>(src.x + size_.x));
		}
		
		template <std::floating_point Fp>
		[[nodiscard]] FORCE_INLINE constexpr Fp y_offset_ratio(const Fp y) const noexcept{
			return math::curve(y, static_cast<float>(src.y), static_cast<float>(src.y + size_.y));
		}

		template <std::floating_point Fp>
		[[nodiscard]] FORCE_INLINE constexpr vector2<Fp> offset_ratio(typename vector2<Fp>::const_pass_t v) noexcept{
			return {this->x_offset_ratio(v.x), this->y_offset_ratio(v.y)};
		}

		[[nodiscard]] FORCE_INLINE constexpr vec_t vert_00() const noexcept{
			return src;
		}

		[[nodiscard]] FORCE_INLINE constexpr vec_t vert_10() const noexcept{
			return {src.x + size_.x, src.y};
		}

		[[nodiscard]] FORCE_INLINE constexpr vec_t vert_01() const noexcept{
			return {src.x, src.y + size_.y};
		}

		[[nodiscard]] FORCE_INLINE constexpr vec_t vert_11() const noexcept{
			return src + size_;
		}

		[[nodiscard]] FORCE_INLINE constexpr vec_t operator[](const std::integral auto i) const noexcept{
			switch(i & 0b11){
			case 0 : return vert_00();
			case 1 : return vert_10();
			case 2 : return vert_11();
			case 3 : return vert_01();
			default : std::unreachable();
			}
		}

		[[nodiscard]] FORCE_INLINE constexpr T area() const noexcept{
			return size_.area();
		}

		[[nodiscard]] FORCE_INLINE constexpr T is_point(T margin) const noexcept{
			return size_.is_zero(margin);
		}

		[[nodiscard]] FORCE_INLINE constexpr T is_point() const noexcept{
			return size_.is_zero();
		}

		template <std::floating_point Fp = float>
		[[nodiscard]] FORCE_INLINE constexpr Fp ratio() const noexcept{
			return static_cast<Fp>(size_.x) / static_cast<Fp>(size_.y);
		}

		FORCE_INLINE constexpr rect_ortho& set_vert(const T srcX, const T srcY, const T endX, const T endY) noexcept{
			const auto [minX, maxX] = math::minmax(srcX, endX);
			const auto [minY, maxY] = math::minmax(srcY, endY);
			this->src.x = minX;
			this->src.y = minY;
			size_.x = maxX - minX;
			size_.y = maxY - minY;

			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& set_vert(typename vec_t::const_pass_t src, typename vec_t::const_pass_t end) noexcept{
			return this->set_vert(src.x, src.y, end.x, end.y);
		}

		FORCE_INLINE constexpr rect_ortho& max_src(typename vec_t::const_pass_t src) noexcept{
			this->src.max(src);
			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& expand_x(T x) noexcept{
			if constexpr(std::unsigned_integral<T>){
				src.x -= x;
				size_.x += x * TWO;
			} else{
				auto doubleX = x * TWO;
				if(-doubleX > size_.x){
					src.x += size_.x / TWO;
					size_.x = 0;
				}else{
					src.x -= x;
					size_.x += doubleX;
				}

			}

			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& expand_y(T y) noexcept{
			if constexpr(std::unsigned_integral<T>){
				src.y -= y;
				size_.y += y * TWO;
			} else{
				auto doubleY = y * TWO;
				if(-doubleY > size_.y){
					src.y += size_.y / TWO;
					size_.y = 0;
				}else{
					src.y -= y;
					size_.y += doubleY;
				}

			}

			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& expand(const T x, const T y) noexcept{
			(void)this->expand_x(x);
			(void)this->expand_y(y);

			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& expand(const T v) noexcept{
			(void)this->expand_x(v);
			(void)this->expand_y(v);

			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& expand(const vector2<T> vec) noexcept{
			(void)this->expand_x(vec.x);
			(void)this->expand_y(vec.y);

			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& shrink_x(T x) noexcept{
			auto doubleX = x * TWO;
			if(doubleX > size_.x){
				src.x += size_.x / TWO;
				size_.x = 0;
			}else{
				src.x += x;
				size_.x -= doubleX;
			}

			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& shrink_y(T y) noexcept{
			auto doubleY = y * TWO;
			if(doubleY > size_.y){
				src.y += size_.y / TWO;
				size_.y = 0;
			}else{
				src.y += y;
				size_.y -= doubleY;
			}

			return *this;
		}

		FORCE_INLINE constexpr rect_ortho& shrink(const T marginX, const T marginY) noexcept{
			(void)this->shrink_x(marginX);

			return this->shrink_y(marginY);
		}

		FORCE_INLINE constexpr rect_ortho& shrink(typename vec_t::const_pass_t margin) noexcept{
			(void)this->shrink_x(margin.x);

			return this->shrink_y(margin.y);
		}

		FORCE_INLINE constexpr rect_ortho& shrink(const T margin) noexcept{
			return this->shrink(margin, margin);
		}

		[[nodiscard]] FORCE_INLINE constexpr bool within_circle(vec_t pos, T radius) const noexcept{
			auto r2 = radius * radius;
			return
				pos.dst2(vert_00()) <= r2 &&
					pos.dst2(vert_11()) <= r2 &&
						pos.dst2(vert_01()) <= r2 &&
							pos.dst2(vert_10()) <= r2;
		}

		[[nodiscard]] FORCE_INLINE constexpr bool within_ring(vec_t pos, T radius_from, T radius_to) const noexcept{
			const auto [near, far] = math::minmax(
				pos.dst2(vert_00()),
				pos.dst2(vert_11()),
				pos.dst2(vert_01()),
				pos.dst2(vert_10())
			);

			return near >= radius_from * radius_from && far <= radius_to * radius_to;
		}

		[[nodiscard]] FORCE_INLINE constexpr bool overlap_ring(vec_t pos, T radius_from, T radius_to) const noexcept{
			const auto [near, far] = math::minmax(
				pos.dst2(vert_00()),
				pos.dst2(vert_11()),
				pos.dst2(vert_01()),
				pos.dst2(vert_10())
			);

			return far >= radius_from * radius_from && near <= radius_to * radius_to;
		}

		[[nodiscard]] FORCE_INLINE constexpr rect_ortho intersection_with(const rect_ortho& r) const noexcept{
			T maxSrcX = math::max(get_src_x(), r.get_src_x());
			T maxSrcY = math::max(get_src_y(), r.get_src_y());

			T minEndX = math::min(get_end_x(), r.get_end_x());
			T minEndY = math::min(get_end_y(), r.get_end_y());

			// ADAPTED_ASSUME(minEndX >= maxSrcX);
			// ADAPTED_ASSUME(minEndY >= maxSrcY);

			return rect_ortho{tags::unchecked,
					maxSrcX, maxSrcY, math::clamp_positive(minEndX - maxSrcX), math::clamp_positive(minEndY - maxSrcY)
				};
		}

		[[nodiscard]] FORCE_INLINE constexpr rect_ortho copy() const noexcept{
			return *this;
		}

		template <std::invocable<vec_t> Fn>
		FORCE_INLINE constexpr void each(Fn func) const noexcept(std::is_nothrow_invocable_v<Fn, vec_t>)
			requires std::integral<T>
		{
			for(T y = src.y; y < get_end_y(); ++y){
				for(T x = src.x; x < get_end_x(); ++x){
					std::invoke(func, vec_t{x, y});
				}
			}
		}

		template <std::invocable<vec_t> Fn>
		FORCE_INLINE constexpr void each_jump_src(Fn func) const noexcept(std::is_nothrow_invocable_v<Fn, vec_t>) requires
			std::integral<T>{
			if(area() == 0)return;

			for(T x = src.x + 1; x < get_end_x(); ++x){
				std::invoke(func, vec_t{x, src.y});
			}

			for(T y = src.y + 1; y < get_end_y(); ++y){
				for(T x = src.x; x < get_end_x(); ++x){
					if(x != src.x && y != src.y) std::invoke(func, vec_t{x, y});
				}
			}
		}


		[[nodiscard]] FORCE_INLINE constexpr std::array<rect_ortho, 4> split() const noexcept{
			const T x = src.x;
			const T y = src.y;
			const T w = size_.x / static_cast<T>(2);
			const T h = size_.y / static_cast<T>(2);

			return std::array<rect_ortho, 4>{
					rect_ortho{tags::unchecked, x, y, w, h},
					rect_ortho{tags::unchecked, x + w, y, w, h},
					rect_ortho{tags::unchecked, x, y + h, w, h},
					rect_ortho{tags::unchecked, x + w, y + h, w, h},
				};
		}
	};


	export using frect = rect_ortho<float>;
	export using irect = rect_ortho<int>;
	export using urect = rect_ortho<unsigned int>;

	//TODO provide trivial rect with no class invariant
	export
	template <typename T>
	using raw_rect = rect_ortho_trivial<T>;

	export using raw_frect = raw_rect<float>;
	export using raw_irect = raw_rect<int>;
	export using raw_urect = raw_rect<unsigned int>;

	export
	template <std::floating_point T>
	constexpr frect InfRect{
			-std::numeric_limits<T>::infinity(),
			-std::numeric_limits<T>::infinity(),
			std::numeric_limits<T>::infinity(),
			std::numeric_limits<T>::infinity(),
		};
}
