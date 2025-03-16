export module Align;

import mo_yanxi.concepts;
export import mo_yanxi.math.vector2;
export import mo_yanxi.math.rect_ortho;

import std;
import mo_yanxi.math;

namespace mo_yanxi{
	namespace align{
		template <number T1, number T2>
		constexpr float floating_div(const T1 a, const T2 b) noexcept{
			return static_cast<float>(a) / static_cast<float>(b);
		}

		template <number T1>
		constexpr T1 floating_mul(const T1 a, const float b) noexcept{
			if constexpr(std::is_floating_point_v<T1>){
				return a * b;
			} else{
				return math::round<T1>(static_cast<float>(a) * b);
			}
		}
	}

	export namespace align{
		template <typename T>
			requires (std::is_arithmetic_v<T>)
		struct padding{
			/**@brief Left Spacing*/
			T left{};
			/**@brief Right Spacing*/
			T right{};
			/**@brief Bottom Spacing*/
			T bottom{};
			/**@brief Top Spacing*/
			T top{};

			[[nodiscard]] constexpr math::vector2<T> bot_lft() const noexcept{
				return {left, bottom};
			}

			[[nodiscard]] constexpr math::vector2<T> top_rit() const noexcept{
				return {right, top};
			}

			[[nodiscard]] constexpr math::vector2<T> top_lft() const noexcept{
				return {left, top};
			}

			[[nodiscard]] constexpr math::vector2<T> bot_rit() const noexcept{
				return {right, bottom};
			}

			[[nodiscard]] friend constexpr bool operator==(const padding& lhs, const padding& rhs) noexcept = default;

			constexpr padding& expand(T x, T y) noexcept{
				x = align::floating_mul(x, 0.5f);
				y = align::floating_mul(y, 0.5f);

				left += x;
				right += x;
				top += y;
				bottom += y;
				return *this;
			}

			constexpr padding& expand(const T val) noexcept{
				return this->expand(val, val);
			}

			[[nodiscard]] constexpr T width() const noexcept{
				return left + right;
			}

			[[nodiscard]] constexpr T height() const noexcept{
				return bottom + top;
			}

			[[nodiscard]] constexpr math::vector2<T> get_size() const noexcept{
				return {width(), height()};
			}

			[[nodiscard]] constexpr T getRemainWidth(const T total = 1) const noexcept{
				return total - width();
			}

			[[nodiscard]] constexpr T getRemainHeight(const T total = 1) const noexcept{
				return total - height();
			}

			constexpr padding& set(const T val) noexcept{
				bottom = top = left = right = val;
				return *this;
			}

			constexpr padding& set(const T l, const T r, const T b, const T t) noexcept{
				left = l;
				right = r;
				bottom = b;
				top = t;
				return *this;
			}

			constexpr padding& setZero() noexcept{
				return set(0);
			}
		};

		template <typename T>
		padding<T> padBetween(const math::rect_ortho<T>& internal, const math::rect_ortho<T>& external){
			return padding<T>{
					internal.get_src_x() - external.get_src_x(),
					external.get_end_x() - internal.get_end_x(),
					internal.get_src_y() - external.get_src_y(),
					external.get_end_y() - internal.get_end_y(),
				};
		}

		using spacing = padding<float>;

		enum class pos : unsigned char{
			none,
			left = 0b0000'0001,
			right = 0b0000'0010,
			center_x = 0b0000'0100,

			top = 0b0000'1000,
			bottom = 0b0001'0000,
			center_y = 0b0010'0000,

			top_left = top | left,
			top_center = top | center_x,
			top_right = top | right,

			center_left = center_y | left,
			center = center_y | center_x,
			center_right = center_y | right,

			bottom_left = bottom | left,
			bottom_center = bottom | center_x,
			bottom_right = bottom | right,
		};

		enum class Scale : unsigned char{
			/** The source is not scaled. */
			none,

			/**
			 * Scales the source to fit the target while keeping the same aspect ratio. This may cause the source to be smaller than the
			 * target in one direction.
			 */
			fit,

			/**
			 * Scales the source to fit the target if it is larger, otherwise does not scale.
			 */
			clamped,

			/**
			 * Scales the source to fill the target while keeping the same aspect ratio. This may cause the source to be larger than the
			 * target in one direction.
			 */
			fill,

			/**
			 * Scales the source to fill the target in the x direction while keeping the same aspect ratio. This may cause the source to be
			 * smaller or larger than the target in the y direction.
			 */
			fillX,

			/**
			 * Scales the source to fill the target in the y direction while keeping the same aspect ratio. This may cause the source to be
			 * smaller or larger than the target in the x direction.
			 */
			fillY,

			/** Scales the source to fill the target. This may cause the source to not keep the same aspect ratio. */
			stretch,

			/**
			 * Scales the source to fill the target in the x direction, without changing the y direction. This may cause the source to not
			 * keep the same aspect ratio.
			 */
			stretchX,

			/**
			 * Scales the source to fill the target in the y direction, without changing the x direction. This may cause the source to not
			 * keep the same aspect ratio.
			 */
			stretchY,
		};

		template <number T>
		constexpr math::vector2<T> embedTo(const Scale stretch, math::vector2<T> srcSize, math::vector2<T> toBound){
			switch(stretch){
			case Scale::fit :{
				const float targetRatio = align::floating_div(toBound.y, toBound.x);
				const float sourceRatio =
					align::floating_div(srcSize.y, srcSize.x);
				const float scale = targetRatio > sourceRatio
					                    ? align::floating_div(toBound.x, srcSize.x)
					                    : align::floating_div(toBound.y, srcSize.y);

				return {align::floating_mul<T>(srcSize.x, scale), align::floating_mul<T>(srcSize.y, scale)};
			}
			case Scale::fill :{
				const float targetRatio =
					align::floating_div(toBound.y, toBound.x);
				const float sourceRatio =
					align::floating_div(srcSize.y, srcSize.x);
				const float scale = targetRatio < sourceRatio
					                    ? align::floating_div(toBound.x, srcSize.x)
					                    : align::floating_div(toBound.y, srcSize.y);

				return {align::floating_mul<T>(srcSize.x, scale), align::floating_mul<T>(srcSize.y, scale)};
			}
			case Scale::fillX :{
				const float scale = align::floating_div(toBound.x, srcSize.x);
				return {align::floating_mul<T>(srcSize.x, scale), align::floating_mul<T>(srcSize.y, scale)};
			}
			case Scale::fillY :{
				const float scale = align::floating_div(toBound.y, srcSize.y);
				return {align::floating_mul<T>(srcSize.x, scale), align::floating_mul<T>(srcSize.y, scale)};
			}
			case Scale::stretch : return toBound;
			case Scale::stretchX : return {toBound.x, srcSize.y};
			case Scale::stretchY : return {srcSize.x, toBound.y};
			case Scale::clamped : if(srcSize.y > toBound.y || srcSize.x > toBound.x){
					return align::embedTo<T>(Scale::fit, srcSize, toBound);
				} else{
					return align::embedTo<T>(Scale::none, srcSize, toBound);
				}
			case Scale::none : return srcSize;
			}

			std::unreachable();
		}

		constexpr bool operator &(const pos l, const pos r){
			return std::to_underlying(l) & std::to_underlying(r);
		}

		template <signed_number T>
		constexpr math::vector2<T> get_offset_of(
			const pos align,
			const math::vector2<T> bottomLeft,
			const math::vector2<T> topRight){
			math::vector2<T> move{};

			if(align & pos::top){
				move.y = -topRight.y;
			} else if(align & pos::bottom){
				move.y = bottomLeft.y;
			}

			if(align & pos::right){
				move.x = -topRight.x;
			} else if(align & pos::left){
				move.x = bottomLeft.x;
			}

			return move;
		}

		constexpr math::vec2 get_offset_of(const pos align, const spacing margin){
			float xMove = 0;
			float yMove = 0;

			if(align & pos::top){
				yMove = -margin.top;
			} else if(align & pos::bottom){
				yMove = margin.bottom;
			}

			if(align & pos::right){
				xMove = -margin.right;
			} else if(align & pos::left){
				xMove = margin.left;
			}

			return {xMove, yMove};
		}

		/**
		 * @brief
		 * @tparam T arithmetic type, does not accept unsigned type
		 * @return
		 */
		template <signed_number T>
		constexpr math::vector2<T> get_offset_of(const pos align, const math::vector2<T>& bound){
			math::vector2<T> offset{};

			if(align & pos::top){
				offset.y = -bound.y;
			} else if(align & pos::center_y){
				offset.y = -bound.y / static_cast<T>(2);
			}

			if(align & pos::right){
				offset.x = -bound.x;
			} else if(align & pos::center_x){
				offset.x = -bound.x / static_cast<T>(2);
			}

			return offset;
		}

		/**
		 * @brief
		 * @tparam T arithmetic type, does not accept unsigned type
		 * @return
		 */
		template <signed_number T>
		constexpr math::vector2<T> get_offset_of(const pos align, const math::rect_ortho<T>& bound){
			return align::get_offset_of<T>(align, bound.size());
		}


		template <signed_number T>
		[[nodiscard]] constexpr math::vector2<T> getVert(const pos align, const math::vector2<T>& size){
			math::vector2<T> offset{};


			if(align & pos::top){
				offset.y = size.y;
			} else if(align & pos::center_y){
				offset.y = size.y / static_cast<T>(2);
			}

			if(align & pos::right){
				offset.x = size.x;
			} else if(align & pos::center_x){
				offset.x = size.x / static_cast<T>(2);
			}

			return offset;
		}


		/**
		 * @brief
		 * @tparam T arithmetic type, does not accept unsigned type
		 * @return
		 */
		template <signed_number T>
		[[nodiscard]] constexpr math::vector2<T> getVert(const pos align, const math::rect_ortho<T>& bound){
			return align::getVert<T>(align, bound.size()) + bound.get_src();
		}

		/**
		 * @brief
		 * @tparam T arithmetic type, does not accept unsigned type
		 * @return
		 */
		template <signed_number T>
		[[nodiscard]] constexpr math::vector2<T> get_offset_of(
			const pos align,
			typename math::vector2<T>::const_pass_t internal_toAlignSize,
			const math::rect_ortho<T>& external){
			math::vector2<T> offset{};

			if(align & pos::bottom){
				offset.y = external.get_end_y() - internal_toAlignSize.y;
			} else if(align & pos::top){
				offset.y = external.get_src_y();
			} else{
				//centerY
				offset.y = external.get_src_y() + (external.height() - internal_toAlignSize.y) / static_cast<T>(2);
			}

			if(align & pos::right){
				offset.x = external.get_end_x() - internal_toAlignSize.x;
			} else if(align & pos::left){
				offset.x = external.get_src_x();
			} else{
				//centerX
				offset.x = external.get_src_x() + (external.width() - internal_toAlignSize.x) / static_cast<T>(2);
			}

			return offset;
		}


		/**
		 * @brief
		 * @return still offset to bottom left, but aligned to given align within the bound
		 */
		template <signed_number T>
		constexpr math::vector2<T> transform_offset(
			const pos align,
			math::vector2<T> scale
		){
			if(align & pos::bottom){
				scale.y *= -1;
			} else if(align & pos::center_y){
				scale.y = 0;
			}

			if(align & pos::right){
				scale.x *= -1;
			} else if(align & pos::center_x){
				scale.x = 0;
			}

			return scale;
		}

		template <signed_number T>
		constexpr math::vector2<T> transform_offset(
			const pos align,
			math::vector2<T> bound_size,
			math::rect_ortho<T> to_transform_inner
		){
			if(align & pos::bottom){
				to_transform_inner.src.y = bound_size.y - to_transform_inner.src.y - to_transform_inner.height();
			} else if(align & pos::center_y){
				to_transform_inner.src.y = (bound_size.y - to_transform_inner.height()) / static_cast<T>(2);
			}

			if(align & pos::right){
				to_transform_inner.src.x = bound_size.x - to_transform_inner.src.x - to_transform_inner.width();
			} else if(align & pos::center_x){
				to_transform_inner.src.x = (bound_size.x - to_transform_inner.width()) / static_cast<T>(2);
			}

			return to_transform_inner.src;
		}


		/**
		 * @brief
		 * @tparam T arithmetic type, does not accept unsigned type
		 * @return
		 */
		template <signed_number T>
		constexpr math::vector2<T> get_offset_of(const pos align, const math::rect_ortho<T>& internal_toAlign,
		                                        const math::rect_ortho<T>& external){
			return align::get_offset_of(align, internal_toAlign.size(), external);
		}
	}
}
