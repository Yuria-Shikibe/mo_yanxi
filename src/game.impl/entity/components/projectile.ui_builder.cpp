module mo_yanxi.game.ecs.component.projectile.ui_builder;

import mo_yanxi.ui.elem.text_elem;

namespace mo_yanxi::game::ecs{
	void projectile_ui_builder::build_hud(ui::table& where, const entity_ref& eref) const{
		auto hdl = where.emplace<entity_info_table>(eref);
		hdl->template_cell.set_external({false, true});
		hdl->function_init([](ui::basic_text_elem& elem){
			elem.set_scale(.5);
			elem.set_policy(font::typesetting::layout_policy::auto_feed_line);
			elem.set_text("Projectile");
		});
	}
}