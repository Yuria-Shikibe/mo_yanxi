//
// Created by Matrix on 2025/3/16.
//

export module test;

import mo_yanxi.ui.elem;
import mo_yanxi.ui.style;
import mo_yanxi.graphic.image_atlas;
import mo_yanxi.graphic.msdf;
import mo_yanxi.graphic.color;
import mo_yanxi.graphic.image_nine_region;

import std;

namespace test{
	using namespace mo_yanxi;

	graphic::image_multi_region nineRegion_edge{};
	graphic::image_multi_region nineRegion_base{};
	ui::style::round_style default_style{};

	export void load_tex(graphic::image_atlas& atlas){
		auto& page = atlas.create_image_page("ui");

		graphic::allocated_image_region& boarder = page.register_named_region("edge", graphic::msdf::create_boarder(12.f)).first;
		nineRegion_edge = {
			boarder,
			align::padding<std::uint32_t>{}.set(16).expand(graphic::msdf::sdf_image_boarder),
			graphic::msdf::sdf_image_boarder / 2
		};

		graphic::allocated_image_region& base = page.register_named_region("base", graphic::msdf::create_solid_boarder(12.f)).first;
		nineRegion_base = {
			base,
			align::padding<std::uint32_t>{}.set(16).expand(graphic::msdf::sdf_image_boarder),
			graphic::msdf::sdf_image_boarder / 2
		};



		default_style.edge = ui::style::palette_with{nineRegion_edge, ui::pal::front};
		default_style.base = ui::style::palette_with{nineRegion_base, ui::style::palette{ui::pal::back}.mul_alpha(0.65f)};

		ui::global_style_drawer = &default_style;
	}
}
