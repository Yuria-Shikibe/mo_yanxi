module;

#include "../src/srl/srl.hpp"

export module test;

import mo_yanxi.ui.basic;
import mo_yanxi.ui.style;
import mo_yanxi.graphic.image_atlas;
import mo_yanxi.graphic.msdf;
import mo_yanxi.graphic.color;
import mo_yanxi.graphic.image_nine_region;

import std;

namespace test{
	export void foot(){
		// using namespace mo_yanxi;
		// math::vec2 v{114, 514};
		// math::vec2 v2;
		// io::pb::math::vector2 buf;
		// io::store(buf, v);
		// io::load(buf, v2);
		// std::println("{} -> {}", v, v2);
	}
	using namespace mo_yanxi;

	export graphic::image_nine_region nineRegion_edge{};
	export graphic::image_nine_region nineRegion_base{};
	export ui::style::round_style default_style{};

	export void load_tex(graphic::image_atlas& atlas){
		auto& page = atlas.create_image_page("ui");

		using namespace std::literals;


		graphic::allocated_image_region& boarder = page.register_named_region(
			"edge"s,
			graphic::sdf_load{graphic::msdf::msdf_generator{graphic::msdf::create_boarder(12.f), 8.}, math::usize2{128, 128}}
		).first;

		nineRegion_edge = {
			boarder,
			align::padding<std::uint32_t>{}.set(20).expand(graphic::msdf::sdf_image_boarder),
			graphic::msdf::sdf_image_boarder
		};

		graphic::allocated_image_region& base = page.register_named_region(
			"base"s,
			graphic::sdf_load{graphic::msdf::msdf_generator{graphic::msdf::create_solid_boarder(12.f), 8.},
			math::usize2{128, 128}}
		).first;

		nineRegion_base = {
			base,
			align::padding<std::uint32_t>{}.set(20).expand(graphic::msdf::sdf_image_boarder),
			graphic::msdf::sdf_image_boarder
		};



		default_style.edge = ui::style::palette_with{nineRegion_edge, ui::pal::front};
		default_style.base = ui::style::palette_with{nineRegion_base, ui::style::palette{ui::pal::back}.mul_alpha(0.65f)};

		ui::global_style_drawer = &default_style;
	}
}
