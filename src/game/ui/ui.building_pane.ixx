//
// Created by Matrix on 2025/7/18.
//

export module mo_yanxi.game.ui.building_pane;

export import mo_yanxi.ui.primitives;

import mo_yanxi.ui.primitives;
import mo_yanxi.ui.elem.list;

import mo_yanxi.game.ui.bars;

namespace mo_yanxi::game::ui{
	export using namespace mo_yanxi::ui;

	struct building_info_pane : ui::elem{
		using elem::elem;

		energy_bar_drawer energy_bar;
		stalled_hp_bar_drawer hp_bar;
		reload_bar_drawer reload_bar;

		void set_state(const ecs::chamber::building_data& data){
			energy_bar.set_bar_state({
				.power_current = data.get_energy_dynamic_status().power,
				.power_total = data.energy_status.power,
				.power_assigned = data.assigned_energy,
				.charge = data.get_energy_dynamic_status().charge,
				.charge_duration = data.energy_status.charge_duration
			});

			hp_bar.set_value(data.hit_point.factor());
			hp_bar.valid_range = data.hit_point.capability_range / data.hit_point.max;
		}

		void update(float delta_in_tick){
			energy_bar.update(delta_in_tick);
		}

		void draw_content(const rect clipSpace) const override;
	};

	struct building_pane : ui::list{
		using list::list;

	};
}