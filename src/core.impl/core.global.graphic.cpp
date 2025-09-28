module mo_yanxi.core.global.graphic;

import mo_yanxi.assets.graphic;
import mo_yanxi.vk.ext;
import std;

namespace mo_yanxi::core::global::graphic{
	void init_vk(){
		context = {vk::ApplicationInfo};
		vk::load_ext(context.get_instance());
	}

	void init_renderers(){
		std::destroy_at(&world);
		std::destroy_at(&ui);
		std::construct_at(&world, context, result_exports);
		std::construct_at(&ui, context, result_exports);
	}
}
