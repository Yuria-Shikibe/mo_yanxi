export module mo_yanxi.ui.assets;


export import :icons;

export import mo_yanxi.ui.style;
export import mo_yanxi.graphic.image_multi_region;
import std;

namespace mo_yanxi::ui::assets{
	namespace colors{
		using graphic::color;
		export using namespace graphic::colors;

		export constexpr inline color accent = color::from_rgba8888(0XF0D456FF);
	}
	
	namespace pal{
		constexpr auto lighten_focus = [](const graphic::color& color) constexpr {
			return color.create_lerp(colors::white, 0.3f);
		};

		constexpr auto lighten_press = [](const graphic::color& color) constexpr {
			return color.create_lerp(colors::white, 0.76f);
		};

		constexpr auto generic_from_color = [](const graphic::color& color) constexpr {
			return style::palette{
				.general = color,
				.on_focus = lighten_focus(color),
				.on_press = lighten_press(color),
				.disabled = colors::gray,
				.activated = colors::light_gray
			};
		};

		export inline
		constexpr style::palette clear{};

		export inline
		constexpr style::palette front{generic_from_color(colors::aqua)};
		constexpr style::palette front_whisper{generic_from_color(colors::gray)};
		constexpr style::palette front_accent{generic_from_color(colors::accent)};
		constexpr style::palette front_valid{generic_from_color(colors::pale_green)};
		constexpr style::palette front_invalid{generic_from_color(colors::red_dusted)};

		export inline
		constexpr style::palette front_clear{[]{
			auto pal = clear;
			pal.activated = colors::light_gray;
			pal.on_focus = colors::white.create_lerp(colors::light_gray, .55f);
			pal.on_press = colors::white;
			return pal;
		}()};

		export inline
		constexpr style::palette base{
			.general = colors::clear,
			.on_focus = colors::dark_gray.copy().set_a(.2f),
			.on_press = colors::gray.copy().set_a(.25f),
			.disabled = colors::clear,
			.activated = colors::dark_gray.copy().set_a(.25f)
		};

		export inline
		constexpr style::palette back{
			.general = colors::black,
			.on_focus = colors::dark_gray,
			.on_press = colors::gray,
			.disabled = colors::black,
			.activated = colors::dark_gray
		};

		export inline
		constexpr style::palette back_black{
			.general = colors::black,
			.on_focus = colors::black,
			.on_press = colors::black,
			.disabled = colors::black,
			.activated = colors::black
		};
	}
	
	namespace shapes{
		export inline graphic::image_nine_region edge{};
		export inline graphic::image_nine_region edge_thin{};
		export inline graphic::image_nine_region base{};
	}

	namespace styles{
		export inline style::round_style general{};
		export inline style::round_style general_static{};
		export inline style::round_style whisper{};
		export inline style::round_style humble{};
		export inline style::round_style accent{};
		export inline style::round_style hint_valid{};
		export inline style::round_style hint_invalid{};
		export inline style::round_style no_edge{};
	}


	/**
	 * @param erased_image_atlas MUST be graphic::image_atlas*
	 * TODO there should be some pre_decl improvement of modules in future?
	 *  if not, provide type safe pointers, or just import graphic::image_atlas
	 * used to improve compile speed
	 */
	export void load(void* erased_image_atlas);
}
