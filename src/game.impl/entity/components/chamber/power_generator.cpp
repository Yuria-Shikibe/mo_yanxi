module mo_yanxi.game.ecs.component.chamber.power_generator;

import mo_yanxi.game.ui.building_pane;

namespace mo_yanxi::game::ecs::chamber{

	void mo_yanxi::game::ecs::chamber::power_generator_build::build_hud(ui::list& where, const entity_ref& eref) const{
		where.emplace<ui::default_building_ui_elem>(eref);
	}

}