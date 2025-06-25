//
// Created by Matrix on 2024/10/7.
//

export module mo_yanxi.ui.style;

export import mo_yanxi.ui.comp.drawer;

import mo_yanxi.ui.basic;
import mo_yanxi.graphic.image_region;
import mo_yanxi.graphic.image_multi_region;
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

			[[nodiscard]] constexpr palette copy() const noexcept{
				return *this;
			}

			constexpr palette& mul_alpha(const float alpha) noexcept{
				general.mul_a(alpha);
				on_focus.mul_a(alpha);
				on_press.mul_a(alpha);

				disabled.mul_a(alpha);
				activated.mul_a(alpha);

				return *this;
			}

			constexpr palette& mul_rgb(const float alpha) noexcept{
				general.mul_rgb(alpha);
				on_focus.mul_rgb(alpha);
				on_press.mul_rgb(alpha);

				disabled.mul_rgb(alpha);
				activated.mul_rgb(alpha);

				return *this;
			}

			[[nodiscard]] graphic::color on_instance(const elem& element) const;
		};

		constexpr palette general_palette{
			graphic::colors::light_gray,
			graphic::colors::white,
			graphic::colors::aqua,
			graphic::colors::gray,
			graphic::colors::white,
		};

		template <typename T>
		struct palette_with : public T{
			palette pal{};

			using T::operator=;

			[[nodiscard]] palette_with() = default;

			[[nodiscard]] palette_with(const T& val, const palette& pal)
				requires (std::is_copy_constructible_v<T>)
				: T{val}, pal{pal}{}
		};

		struct round_style : style_drawer<elem>{
			align::spacing boarder{DefaultBoarder};
			palette_with<graphic::image_nine_region> base{};
			palette_with<graphic::image_nine_region> edge{};
			palette_with<graphic::image_nine_region> back{};

			float disabledOpacity{.5f};

			void draw(const elem& element, math::frect region, float opacityScl) const override;

			[[nodiscard]] float content_opacity(const elem& element) const override;

			bool apply_to(elem& element) const override;
		};
	}
}
