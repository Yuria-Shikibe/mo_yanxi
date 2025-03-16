//
// Created by Matrix on 2025/3/13.
//

export module mo_yanxi.ui.layout.policies;

import std;
import mo_yanxi.math.vector2;

namespace mo_yanxi::ui{
	export struct illegal_layout : std::exception{
		[[nodiscard]] explicit illegal_layout(char const* msg)
			: exception(msg){
		}

		[[nodiscard]] illegal_layout() : illegal_layout{"Illegal Layout"}{}
	};

	export enum class layout_policy{
		horizontal,
		vertical,
		none,
	};

	export enum class size_category{
		passive,
		scaling,
		mastering,
		external,
	};

	export struct stated_size{
		size_category type{size_category::passive};
		float value{1.};

		constexpr friend bool operator==(const stated_size& lhs, const stated_size& rhs) noexcept = delete;

		[[nodiscard]] constexpr bool mastering() const noexcept{
			return type == size_category::mastering;
		}

		[[nodiscard]] constexpr bool dependent() const noexcept{
			return type == size_category::external;
		}

		constexpr explicit(false) operator float() const noexcept{
			return mastering() ? value : 0.0f;
		}

		[[nodiscard]] constexpr stated_size decay() const noexcept{
			switch(type){
			case size_category::scaling : [[fallthrough]];
			case size_category::external : return {size_category::passive, value};
			default : return *this;
			}
		}

		constexpr void promote(float mastering_size) noexcept{
			if(mastering()){
				value = std::max(mastering_size, value);
			}else{
				type = size_category::mastering;
				value = mastering_size;
			}

		}

		constexpr void try_promote_by(const stated_size o) noexcept{
			switch(o.type){
			case size_category::mastering : if(mastering()){
					value = std::max(value, o.value);
				} else{
					*this = o;
				}
				return;
			case size_category::scaling : if(type == size_category::scaling){
					value = std::max(value, o.value);
				} else if(type == size_category::passive){
					*this = o;
				}
				return;
			case size_category::external : if(type == size_category::external){
					value = std::max(value, o.value);
				} else if(type == size_category::passive){
					type = size_category::external;
					value = std::max(value, o.value);
				}
				return;
			case size_category::passive : if(type == size_category::passive){
					value = std::max(value, o.value);
				}
				return;
			default : std::unreachable();
			}
		}
	};

	export struct stated_extent{
		stated_size width{};
		stated_size height{};

		constexpr friend bool operator==(const stated_extent& lhs, const stated_extent& rhs) noexcept = delete;

		[[nodiscard]] constexpr bool dependent() const noexcept{
			return width.type == size_category::external && height.type == size_category::external;
		}

		constexpr void collapse(const math::vec2 size) noexcept{
			if(!width.dependent()) width = {size_category::mastering, size.x};
			if(!height.dependent()) height = {size_category::mastering, size.y};
		}

		[[nodiscard]] constexpr bool mastering() const noexcept{
			if(width.mastering()){
				return height.mastering() || height.type == size_category::scaling;
			}

			if(height.mastering()){
				return width.type == size_category::scaling;
			}

			return false;
		}

		[[nodiscard]] constexpr stated_extent promote() const noexcept{
			if(width.mastering() && height.type == size_category::scaling){
				return {width, {size_category::mastering, width.value * height.value}};
			}
			if(height.mastering() && width.type == size_category::scaling){
				return {{size_category::mastering, width.value * height.value}, height};
			}
			return *this;
		}

		[[nodiscard]] math::vec2 potential_max_size() const noexcept{
			math::vec2 rst{width, height};
			if(width.dependent())rst.x = std::numeric_limits<math::vec2::value_type>::infinity();
			if(height.dependent())rst.y = std::numeric_limits<math::vec2::value_type>::infinity();
			return rst;
		}
	};

	export constexpr stated_extent extent_by_external{{size_category::external}, {size_category::external}};
}
