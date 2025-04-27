//
// Created by Matrix on 2024/10/7.
//

export module mo_yanxi.ui.style;

export import mo_yanxi.ui.comp.drawer;

import mo_yanxi.ui.basic;
import mo_yanxi.graphic.image_region;
import mo_yanxi.graphic.image_nine_region;
export import mo_yanxi.graphic.color;
export import align;

import std;

namespace mo_yanxi::ui{
	export
	namespace style{
		struct palette{
			graphic::color general{};
			graphic::color on_focus{};
			graphic::color on_press{};

			graphic::color disabled{};
			graphic::color activated{};

			constexpr palette& mul_alpha(const float alpha) noexcept{
				general.mulA(alpha);
				on_focus.mulA(alpha);
				on_press.mulA(alpha);

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
}
