module mo_yanxi.game.ecs.component.chamber.power_generator;

import mo_yanxi.ui.elem.label;
import mo_yanxi.game.ui.bars;
import mo_yanxi.ui.assets;

namespace mo_yanxi::game::ecs::chamber{
	struct power_generator_ui : entity_info_table{
		ui::energy_bar* energy_bar{};

		[[nodiscard]] power_generator_ui(ui::scene* scene, group* group, const entity_ref& ref)
			: entity_info_table(scene, group, ref){

			template_cell.set_external({false, true});
			function_init([](ui::label& elem){
				elem.set_style();
				elem.set_scale(.5);
				elem.set_policy(font::typesetting::layout_policy::auto_feed_line);
				elem.set_text("Generator");
			}).cell().set_pad({.top = 8});

			{
				auto bar = end_line().emplace<ui::energy_bar>();
				bar->set_style();
				bar.cell().set_pad(4);
				bar.cell().set_height(25);

				energy_bar = &bar.elem();
			}
		}

		void update(float delta_in_ticks) override{
			entity_info_table::update(delta_in_ticks);

			if(!ref)return;
			auto& build = ref->at<building_data>();

			energy_bar->drawer.set_bar_state({
				.power_current = build.get_energy_dynamic_status().power,
				.power_total = build.energy_status.power,
				.charge = build.get_energy_dynamic_status().charge,
				.charge_duration = build.energy_status.charge_duration,
			});
			energy_bar->drawer.update(delta_in_ticks);
		}
	};



	void mo_yanxi::game::ecs::chamber::power_generator_build::build_hud(ui::table& where, const entity_ref& eref) const{
		where.emplace<power_generator_ui>(eref);
	}

}