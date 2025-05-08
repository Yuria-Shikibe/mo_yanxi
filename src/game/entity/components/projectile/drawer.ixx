//
// Created by Matrix on 2025/4/22.
//

export module mo_yanxi.game.ecs.component.projectile.drawer;

export import mo_yanxi.game.ecs.component.damage;

export import mo_yanxi.game.ecs.component.drawer;
import mo_yanxi.graphic.trail;
import mo_yanxi.math;

namespace mo_yanxi::game::ecs{
	export struct projectile_drawer : drawer::entity_drawer{
		graphic::uniformed_trail trail{50};
		float trail_radius{10};
		math::section<graphic::color> trail_color{};
		drawer::part_pos_transform trail_trans{};

		drawer::local_drawer drawer{};

		void draw(const world::graphic_context& draw_ctx) const override;
	};


	template <>
	struct component_custom_behavior<projectile_drawer> : component_custom_behavior_base<projectile_drawer>{
		using base_types = drawer::entity_drawer;
	};
}

