//
// Created by Matrix on 2025/6/21.
//

export module mo_yanxi.game.ecs.component.chamber.general;

namespace mo_yanxi::game::ecs::chamber{
	struct building_;
	struct manifold_;

	export using build_ref = building_&;
	export using build_ptr = building_*;

	export using const_build_ref = const building_&;

	export using manifold_ref = manifold_&;

	export
	struct energy_status{
		int power;
		float charge_duration;

		bool reserve_energy_if_power_off;
		bool reserve_charge_reload_if_power_off;
		bool safe_under_damage;
		// bool
	};

}