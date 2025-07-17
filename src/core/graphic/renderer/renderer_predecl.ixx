//
// Created by Matrix on 2025/6/4.
//

export module mo_yanxi.graphic.renderer.predecl;

namespace mo_yanxi::graphic{
	//Used only for speeding up compile.
	//maybe

	struct renderer_world_;
	struct renderer_ui_;
	export using renderer_world_ptr = renderer_world_*;
	export using renderer_ui_ptr = renderer_ui_*;

	export using renderer_world_ref = renderer_world_&;
	export using renderer_ui_ref = renderer_ui_&;
}
