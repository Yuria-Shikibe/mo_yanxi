module mo_yanxi.game.content;

import mo_yanxi.math;
import mo_yanxi.game.meta.turret;
import mo_yanxi.core.global.assets;


void mo_yanxi::game::content::load(){
	auto& manager = chambers;
	manager = meta::content_manager{};


	using namespace meta::chamber;

	{
		auto& r = manager.emplace<basic_chamber, armor>("wall");
		r.extent = {1, 1};
	}

	{
		auto& b = manager.emplace<basic_chamber, radar>("radar");
		b.extent = {4, 4};
		b.targeting_range_radius = {500, 6000};
		// b.targeting_range_angular = {-math::pi_half / 3.f, math::pi_half / 3.f};
		b.transform.vec = {.5f, .5f};
		b.reload_duration = 60;

		static_cast<ecs::chamber::energy_status&>(b) = ecs::chamber::energy_status{
			.power = -4,
			.charge_duration = 45
		};
	}

	{
		auto& b = manager.emplace<basic_chamber, structural_joint>("structural_joint");
		b.extent = {2, 2};
	}

	{
		auto& b = manager.emplace<basic_chamber, turret_base>("turret");
		b.extent = {2, 4};
		b.power = -8;
		b.charge_duration = 180;
		// b. = {1200, 6000};
		// b.targeting_range_angular = {-math::pi_half / 3.f, math::pi_half / 3.f};
		b.transform.vec = {.5f, .5f};
	}

	{
		auto& b = manager.emplace<basic_chamber, thurster>("thurster");
		b.extent = {2, 2};
		b.power = -3;
		b.charge_duration = 60;
		// b. = {1200, 6000};
		// b.targeting_range_angular = {-math::pi_half / 3.f, math::pi_half / 3.f};
		b.maneuver = {
			.force_longitudinal = 50000,
			.force_transverse = 10000,
			.torque = 50,
			.torque_absolute = 50,
			.boost = 0
		};
	}

	{
		auto& b = manager.emplace<basic_chamber, energy_generator>("generator");
		b.extent = {3, 2};
		b.power = 8;
		b.charge_duration = 60;
		// b. = {1200, 6000};
		// b.targeting_range_angular = {-math::pi_half / 3.f, math::pi_half / 3.f};

	}

	manager.freeze();
}
