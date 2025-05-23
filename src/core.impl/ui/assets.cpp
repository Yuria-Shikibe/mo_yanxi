module mo_yanxi.ui.assets;

import mo_yanxi.graphic.image_manage;
import mo_yanxi.graphic.msdf;
import mo_yanxi.assets.directories;

mo_yanxi::ui::theme::icons::icon_raw_present load_svg(
	mo_yanxi::graphic::image_page& ui_page,
	const std::filesystem::path& dir,
	std::string_view name,
	int boarder = mo_yanxi::graphic::msdf::sdf_image_boarder,
	mo_yanxi::math::usize2 extent = {64, 64}
	){
	const auto path = dir / std::format("{}.svg", name);
	auto& rst = ui_page.register_named_region(
		name,
		mo_yanxi::graphic::sdf_load{
			mo_yanxi::graphic::msdf::msdf_generator{
				path.string().c_str(),
				mo_yanxi::graphic::msdf::sdf_image_range,
				boarder
			}, extent
		}).first;

	ui_page.mark_protected(name);
	return static_cast<mo_yanxi::ui::theme::icons::icon_raw_present>(rst);
}

void load_icons(mo_yanxi::graphic::image_page& ui_page){
	using namespace mo_yanxi;
	namespace icons = ui::theme::icons;

	auto dir = assets::dir::svg / "ui";

	auto load = [&](std::string_view stem_name, int boarder = mo_yanxi::graphic::msdf::sdf_image_boarder){
		return load_svg(ui_page, dir, stem_name, boarder);
	};

#define NAMED_LOAD(name) icons::name = load(#name);
#define NAMED_LOAD_B(name, boarder) icons::##name = load(#name, boarder);

	NAMED_LOAD(file_general);
	NAMED_LOAD(folder_general);
	NAMED_LOAD(left);
	NAMED_LOAD(right);
	NAMED_LOAD(up);
	NAMED_LOAD(down);

	NAMED_LOAD(check);
	NAMED_LOAD(close);
	NAMED_LOAD(plus);

	NAMED_LOAD(blender_icon_pivot_active);
	NAMED_LOAD(blender_icon_pivot_individual);
	NAMED_LOAD(blender_icon_pivot_median);

#undef NAMED_LOAD
}

void mo_yanxi::ui::theme::load(void* erased_image_atlas){
	graphic::image_atlas& atlas = *static_cast<graphic::image_atlas*>(erased_image_atlas);
	auto& page = atlas.create_image_page("ui");

	using namespace std::literals;

	{
		graphic::allocated_image_region& boarder = page.register_named_region(
			"edge"s,
			graphic::sdf_load{
				graphic::msdf::msdf_generator{graphic::msdf::create_boarder(12.f, 2.5f), 8.}, math::usize2{96, 96}, 2
			}, true
		).first;

		graphic::allocated_image_region& boarder_thin = page.register_named_region(
			"edge_thin"s,
			graphic::sdf_load{
				graphic::msdf::msdf_generator{graphic::msdf::create_boarder(12.f, 1.85f), 8.}, math::usize2{96, 96}, 2
			},
			true
		).first;


		graphic::allocated_image_region& base = page.register_named_region(
			"base"s,
			graphic::sdf_load{
				graphic::msdf::msdf_generator{graphic::msdf::create_solid_boarder(12.f), 8.},
				math::usize2{96, 96}, 5
			},
			true
		).first;

		graphic::allocated_image_region& line = page.register_named_region(
			"line"s,
			graphic::sdf_load{
				graphic::msdf::msdf_generator{graphic::msdf::svg_to_shape((assets::dir::svg.path() / "ui/line.svg").string().data())},
				{40u, 24u}
			},
			true).first;

		shapes::edge = {
				boarder,
				align::padding<std::uint32_t>{}.set(12).expand(graphic::msdf::sdf_image_boarder),
				graphic::msdf::sdf_image_boarder
			};

		shapes::edge_thin = {
				boarder_thin,
				align::padding<std::uint32_t>{}.set(12).expand(graphic::msdf::sdf_image_boarder),
				graphic::msdf::sdf_image_boarder
			};

		shapes::base = {
				base,
				align::padding<std::uint32_t>{}.set(12).expand(graphic::msdf::sdf_image_boarder),
				graphic::msdf::sdf_image_boarder
			};

		shapes::line = graphic::image_caped_region{line, line.get_region(), 12, 12, graphic::msdf::sdf_image_boarder};


		using namespace styles;

		general.edge = style::palette_with{shapes::edge, style_pal::front};
		general.base = style::palette_with{shapes::base, style_pal::base};
		general.back = style::palette_with{shapes::base, style_pal::back_black};

		whisper = general;
		whisper.edge.pal = style_pal::front_whisper;

		humble = whisper;
		humble.edge = shapes::edge_thin;
		humble.edge.pal.mul_rgb(.5f);
		humble.edge.pal.activated = whisper.edge.pal.activated;
		humble.base.pal.mul_rgb(.6f);
		humble.base.pal.activated = whisper.base.pal.activated;

		accent = general;
		accent.edge.pal = style_pal::front_accent;

		hint_valid = general;
		hint_valid.edge.pal = style_pal::front_valid;

		hint_invalid = general;
		hint_invalid.edge.pal = style_pal::front_invalid;

		no_edge.edge = style::palette_with{shapes::edge, style_pal::front_clear};
		no_edge.edge.pal.on_focus = {};

		no_edge.base = style::palette_with{shapes::base, style::palette{style_pal::front_clear}.mul_alpha(0.25f)};
		no_edge.base.pal.activated.set_a(.075f);

		general_static = general;
		general_static.base.pal.on_focus = {};
		general_static.base.pal.on_press = {};
		// general_static.back.pal = pal::back_black;

		global_style_drawer = &general;

		load_icons(page);
	}


}
