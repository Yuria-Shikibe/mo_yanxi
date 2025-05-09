//
// Created by Matrix on 2025/5/7.
//

export module mo_yanxi.game.ecs.component.ui.builder;

import mo_yanxi.ui.table;

namespace mo_yanxi::game::ecs{
	export
	struct ui_builder{
		using table_handle = ui::create_handle<ui::table, ui::table_cell_adaptor::cell_type>;

		virtual ~ui_builder() = default;

		virtual void build_hud(table_handle hdl) const {

		}
	};
}
