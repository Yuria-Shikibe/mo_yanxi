export module mo_yanxi.ui.selection;

export import mo_yanxi.math.vector2;
export import mo_yanxi.math.rect_ortho;
import std;

export namespace mo_yanxi::ui::util{
	template <typename T = float>
	struct box_selection{
		using point_t = math::vector2<T>;
		using rect_t = math::rect_ortho<T>;

		static constexpr point_t invalid_pos{[]{
			if constexpr (std::floating_point<T>){
				return math::vectors::constant2<T>::SNaN;
			}else{
				return math::vectors::constant2<T>::lowest_vec2;
			}
		}()};

		point_t src{invalid_pos};

		constexpr void set_invalid() noexcept{
			src = invalid_pos;
		}

		constexpr void begin_select(const typename point_t::const_pass_t srcPos) noexcept{
			src = srcPos;
		}

		constexpr [[nodiscard]] bool is_selecting() const noexcept{
			if constexpr (std::floating_point<T>){
				return !src.is_NaN();
			}else{
				return src != invalid_pos;
			}
		}

		[[nodiscard]] explicit operator bool() const noexcept{
			return is_selecting();
		}

		[[nodiscard]] constexpr rect_t get_region(const typename point_t::const_pass_t curPos) const noexcept{
			return rect_t{src, curPos};
		}

		rect_t end_select(typename point_t::const_pass_t curPos){
			auto region = this->get_region(curPos);

			set_invalid();

			return region;
		}
	};
}
