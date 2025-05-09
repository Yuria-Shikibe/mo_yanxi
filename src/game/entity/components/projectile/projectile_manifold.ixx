//
// Created by Matrix on 2025/4/22.
//

export module mo_yanxi.game.ecs.component.projectile.manifold;

export import mo_yanxi.game.ecs.component.damage;

import mo_yanxi.graphic.trail;
import mo_yanxi.graphic.color;
import mo_yanxi.math;


namespace mo_yanxi::game::ecs{
	export struct projectile_manifold{
		damage_group damage_group;

	};
}

