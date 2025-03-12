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
	constexpr inline Align::spacing DefaultBoarder{BoarderStroke, BoarderStroke, BoarderStroke, BoarderStroke};

	struct elem;

	export
	namespace Style{
		struct Palette{
			graphic::color general{};
			graphic::color onFocus{};
			graphic::color onPress{};

			graphic::color disabled{};
			graphic::color activated{};

			constexpr Palette& mulAlpha(const float alpha) noexcept{
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
			Palette palette{};

			[[nodiscard]] palette_with() = default;

			[[nodiscard]] explicit palette_with(const T& val, const Palette& palette)
				requires (std::is_copy_constructible_v<T>)
				: T{val}, palette{palette}{}
		};

		struct round_style : style_drawer<elem>{
			Align::spacing boarder{DefaultBoarder};
			palette_with<graphic::image_nine_region> base{};
			palette_with<graphic::image_nine_region> edge{};
			float disabledOpacity{.5f};

			void draw(const elem& element, math::frect region, float opacityScl) const override;

			float content_opacity(const elem& element) const override;

			bool apply_to(elem& element) const override;
		};
	}
}
