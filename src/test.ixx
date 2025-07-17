export module test;

import mo_yanxi.ui.primitives;
import mo_yanxi.ui.style;
import mo_yanxi.ui.assets;
import mo_yanxi.graphic.image_manage;
import mo_yanxi.graphic.msdf;
import mo_yanxi.graphic.color;
import mo_yanxi.graphic.image_multi_region;

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

	export void load_tex(graphic::image_atlas& atlas){
		ui::theme::load(&atlas);
	}
}
