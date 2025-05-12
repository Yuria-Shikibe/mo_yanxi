//
// Created by Matrix on 2025/5/12.
//

export module mo_yanxi.game.meta.projectile;

export import mo_yanxi.game.meta.hitbox;
export import mo_yanxi.game.meta.graphic;
export import mo_yanxi.game.ecs.component.physical_property;
export import mo_yanxi.game.ecs.component.damage;
export import mo_yanxi.game.ecs.component.drawer;

import std;

namespace mo_yanxi::game::meta{
	export
	struct projectile{
		meta::hitbox hitbox{};
		ecs::physical_rigid rigid{};
		ecs::damage_group damage{};

		std::optional<float> initial_speed{};
		ecs::drawer::local_drawer drawer{};
		meta::trail_meta_style trail_style{};
	};
}