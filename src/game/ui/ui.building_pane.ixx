//
// Created by Matrix on 2025/7/18.
//

export module mo_yanxi.game.ui.building_pane;

export import mo_yanxi.ui.primitives;

import mo_yanxi.ui.primitives;
import mo_yanxi.ui.elem.list;

namespace mo_yanxi::game::ui{
	export using namespace mo_yanxi::ui;

	struct building_pane : list{
		[[nodiscard]] building_pane(scene* scene, group* group_)
			: list(scene, group_){
		}
	};
}