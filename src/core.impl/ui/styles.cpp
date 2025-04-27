module mo_yanxi.ui.assets;

import mo_yanxi.graphic.image_atlas;
import mo_yanxi.graphic.msdf;

void mo_yanxi::ui::assets::load(void* erased_image_atlas){
	graphic::image_atlas& atlas = *static_cast<graphic::image_atlas*>(erased_image_atlas);
	auto& page = atlas.create_image_page("ui");

	using namespace std::literals;

	graphic::allocated_image_region& boarder = page.register_named_region(
			"edge"s,
			graphic::sdf_load{graphic::msdf::msdf_generator{graphic::msdf::create_boarder(12.f, 2.5f), 8.}, math::usize2{128, 128}, 5}
		).first;


	shapes::edge = {
		boarder,
		align::padding<std::uint32_t>{}.set(32).expand(graphic::msdf::sdf_image_boarder),
		graphic::msdf::sdf_image_boarder
	};

	graphic::allocated_image_region& base = page.register_named_region(
		"base"s,
		graphic::sdf_load{graphic::msdf::msdf_generator{graphic::msdf::create_solid_boarder(12.f), 8.},
		math::usize2{128, 128}, 5}
	).first;

	shapes::base = {
		base,
		align::padding<std::uint32_t>{}.set(32).expand(graphic::msdf::sdf_image_boarder),
		graphic::msdf::sdf_image_boarder
	};

	page.mark_protected("base");
	page.mark_protected("edge");

	using namespace styles;

	general.edge = style::palette_with{shapes::edge, pal::front};
	general.base = style::palette_with{shapes::base, style::palette{pal::back}.mul_alpha(0.65f)};

	whisper = general;
	whisper.edge.pal = pal::front_whisper;

	accent = general;
	accent.edge.pal = pal::front_accent;

	hint_valid = general;
	hint_valid.edge.pal = pal::front_valid;

	hint_invalid = general;
	hint_invalid.edge.pal = pal::front_invalid;

	clear.edge = style::palette_with{shapes::edge, pal::front_clear};
	clear.edge.pal.on_focus = {};

	clear.base = style::palette_with{shapes::base, style::palette{pal::front_clear}.mul_alpha(0.25f)};
	clear.base.pal.activated.set_a(.075f);

	global_style_drawer = &general;
}
