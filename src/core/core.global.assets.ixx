//
// Created by Matrix on 2025/5/5.
//

export module mo_yanxi.core.global.assets;

export import mo_yanxi.graphic.image_atlas;
export import mo_yanxi.font.manager;

namespace mo_yanxi::core::global::assets{
	export inline font::font_manager font_manager{};
	export inline graphic::image_atlas atlas{};

	export void init(void* vk_context_ptr);

	export void dispose();
}