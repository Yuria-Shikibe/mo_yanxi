module mo_yanxi.core.global.graphic;

import mo_yanxi.assets.graphic;
import mo_yanxi.vk.ext;
import std;

namespace mo_yanxi::core::global::graphic{
	void init(){
		context = {vk::ApplicationInfo};
		vk::load_ext(context.get_instance());
	}

	void post_init(){
		std::destroy_at(&world);
		std::destroy_at(&ui);
		std::destroy_at(&merger);
		std::construct_at(&world, context, result_exports);
		std::construct_at(&ui, context, result_exports);
		std::construct_at(&merger, context, result_exports, assets::graphic::shaders::comp::result_merge.get_create_info());
	}
}
