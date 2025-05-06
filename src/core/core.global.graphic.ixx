//
// Created by Matrix on 2025/3/17.
//

export module mo_yanxi.core.global.graphic;

export import mo_yanxi.vk.context;

export import mo_yanxi.graphic.renderer;
export import mo_yanxi.graphic.renderer.world;
export import mo_yanxi.graphic.renderer.ui;
export import mo_yanxi.graphic.renderer.merger;

namespace mo_yanxi::core::global::graphic{
	export inline vk::context context{};

	mo_yanxi::graphic::renderer_export result_exports{};

	export mo_yanxi::graphic::renderer_world world;
	export mo_yanxi::graphic::renderer_ui ui;
	export mo_yanxi::graphic::renderer_merge merger;

	export void init_vk();

	export void init_renderers();

	export void dispose(){
		merger = {};
		ui = {};
		world = {};
		context = {};
	}
}