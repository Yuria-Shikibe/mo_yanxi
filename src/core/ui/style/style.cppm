//
// Created by Matrix on 2024/10/7.
//

export module mo_yanxi.ui.style;

export import mo_yanxi.ui.comp.drawer;

import mo_yanxi.graphic.image_region;
import mo_yanxi.graphic.image_nine_region;
export import mo_yanxi.graphic.color;
export import Align;

import std;

namespace mo_yanxi::ui{
	constexpr inline align::spacing DefaultBoarder{BoarderStroke, BoarderStroke, BoarderStroke, BoarderStroke};

	export struct elem;

	export
	namespace style{
		struct palette{
			graphic::color general{};
			graphic::color onFocus{};
			graphic::color onPress{};

			graphic::color disabled{};
			graphic::color activated{};

			constexpr palette& mul_alpha(const float alpha) noexcept{
				general.mulA(alpha);
				onFocus.mulA(alpha);
				onPress.mulA(alpha);

				disabled.mulA(alpha);
				activated.mulA(alpha);

				return *this;
			}

			[[nodiscard]] graphic::color onInstance(const elem& element) const;
		};

		template <typename T>
		struct palette_with : public T{
			palette pal{};

			[[nodiscard]] palette_with() = default;

			[[nodiscard]] explicit palette_with(const T& val, const palette& pal)
				requires (std::is_copy_constructible_v<T>)
				: T{val}, pal{pal}{}
		};

		struct round_style : style_drawer<elem>{
			align::spacing boarder{DefaultBoarder};
			palette_with<graphic::image_nine_region> base{};
			palette_with<graphic::image_nine_region> edge{};

			float disabledOpacity{.5f};

			void draw(const elem& element, math::frect region, float opacityScl) const override;

			[[nodiscard]] float content_opacity(const elem& element) const override;

			bool apply_to(elem& element) const override;
		};
	}

	namespace pal{
		export inline
		constexpr style::palette Clear{
			.general = graphic::colors::clear,
			.onFocus = graphic::colors::clear,
			.onPress = graphic::colors::clear,
			.disabled = graphic::colors::clear,
			.activated = graphic::colors::clear
		};

		export inline
		constexpr style::palette front{
			.general = graphic::colors::AQUA,
			.onFocus = graphic::colors::AQUA.create_lerp(graphic::colors::white, 0.3f),
			.onPress = graphic::colors::white,
			.disabled = graphic::colors::gray,
			.activated = graphic::colors::light_gray
		};

		export inline
		constexpr style::palette front_clear{[]{
			auto pal = Clear;
			pal.activated = graphic::colors::light_gray;
			pal.onFocus = graphic::colors::white;
			pal.onPress = graphic::colors::white;
			return pal;
		}()};

		export inline
		constexpr style::palette back{
			.general = graphic::colors::black,
			.onFocus = graphic::colors::dark_gray,
			.onPress = graphic::colors::gray,
			.disabled = graphic::colors::dark_gray.copy().mulA(.226f),
			.activated = graphic::colors::light_gray.copy().mulA(.126f)
		};
	}
}
