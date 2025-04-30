//
// Created by Matrix on 2024/9/13.
//

export module mo_yanxi.ui.clamped_size;

export import mo_yanxi.math.vector2;
import mo_yanxi.ui.util;
import std;

export namespace mo_yanxi::ui{
	template <typename T>
	struct clamped_size{
		using SizeType = math::vector2<T>;
	private:
		SizeType minimumSize{math::vectors::constant2<T>::zero_vec2};
		SizeType maximumSize{math::vectors::constant2<T>::max_vec2};
		SizeType size{};

	public:
		[[nodiscard]] constexpr clamped_size() = default;

		[[nodiscard]] constexpr explicit clamped_size(const SizeType& size)
			: size{size}{}

		[[nodiscard]] constexpr clamped_size(const SizeType& minimumSize, const SizeType& maximumSize, const SizeType& size)
			: minimumSize{minimumSize},
			  maximumSize{maximumSize},
			  size{size}{}

		[[nodiscard]] constexpr SizeType get_minimum_size() const noexcept{
			return minimumSize;
		}

		[[nodiscard]] constexpr SizeType get_maximum_size() const noexcept{
			return maximumSize;
		}

		[[nodiscard]] constexpr SizeType get_size() const noexcept{
			return size;
		}

		[[nodiscard]] constexpr T get_width() const noexcept{
			return size.x;
		}

		[[nodiscard]] constexpr T get_height() const noexcept{
			return size.y;
		}

		constexpr void set_size_unchecked(const SizeType size) noexcept{
			this->size = size;
		}

		/**
		 * @brief
		 * @return true if width has been changed
		 */
		constexpr bool set_width(T w) noexcept{
			w = std::clamp(w, minimumSize.x, maximumSize.x);
			return util::try_modify(size.x, w);
		}

		/**
		 * @brief
		 * @return true if height has been changed
		 */
		constexpr bool set_height(T h) noexcept{
			h = std::clamp(h, minimumSize.y, maximumSize.y);
			return util::try_modify(size.y, h);
		}

		/**
		 * @brief
		 * @return true if size has been changed
		 */
		constexpr bool set_size(const SizeType s) noexcept{
			bool b{};
			b |= set_width(s.x);
			b |= set_height(s.y);
			return b;
		}

		/**
		 * @brief
		 * @return true if size has been changed
		 */
		constexpr bool set_minimum_size(SizeType sz) noexcept{
			if(this->minimumSize != sz){
				this->minimumSize = sz;
				return util::try_modify(size, sz.max(size));
			}
			return false;
		}

		/**
		 * @brief
		 * @return true if size has been changed
		 */
		constexpr bool set_maximum_size(SizeType sz) noexcept{
			if(this->maximumSize != sz){
				this->maximumSize = sz;
				return util::try_modify(size, sz.min(size));
			}
			return false;
		}

		constexpr friend bool operator==(const clamped_size& lhs, const clamped_size& rhs) = default;
	};

	using clamped_fsize = clamped_size<float>;
}
