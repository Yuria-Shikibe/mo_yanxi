//
// Created by Matrix on 2025/3/17.
//

export module mo_yanxi.core.global.graphic;

export import mo_yanxi.vk.context;

namespace mo_yanxi::core::global::graphic{
	export inline vk::context context{};

	export void init(){
		context = {vk::ApplicationInfo};
	}

	export void dispose(){
		context = {};
	}
}