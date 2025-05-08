module;

#include <gch/small_vector.hpp>

module mo_yanxi.game.ecs.component.manifold;

import mo_yanxi.game.ecs.component.projectile.manifold;
import mo_yanxi.game.ecs.component.chamber;


namespace mo_yanxi::game::ecs{



	auto getCollidedTileWith(
		const collision_object& hitter,
		const collision_object& receiver,
		const intersection& intersection,
		chamber::chamber_manifold& obj_grid
		) noexcept{

		math::frect_box hitter_box = hitter.manifold->hitbox[intersection.index.obj_idx];

		const auto correction = -receiver.manifold->hitbox.get_back_trace_move();//receiver.manifold->is_under_correction ? -receiver.manifold->hitbox.get_back_trace_move() : receiver.motion->pos() - hitter.manifold->hitbox.get_trans().vec;
		const auto move = hitter.manifold->hitbox.get_back_trace_move_full();
		hitter_box.move(move - correction);

		hitter_box = math::wrap_striped_box(-move, hitter_box);
		return obj_grid.get_dst_sorted_tiles(hitter_box, intersection.pos, hitter.motion->vel.vec.copy().normalize());
	}


	collision_result chamber_collider::try_collide_to(
		const collision_object& projectile, const collision_object& chamber_grid,
		const intersection& intersection) noexcept{

		// return true;

		auto chambers = chamber_grid.id->try_get<chamber::chamber_manifold>();
		if(!chambers)return collision_result::passed;
		// std::println(std::cerr, "{}", "On Check");

		auto& dmg = projectile.id->at<projectile_manifold>();
		auto rst = getCollidedTileWith(projectile, chamber_grid, intersection, *chambers);
		if(rst.empty())return collision_result::none;

		bool any{false};

		using gch::erase_if;

		for (auto && under_hit : rst){
			auto& data = under_hit.building.data();
			switch(data.consume_damage(under_hit.tile_pos, dmg.damage_group)){
			case chamber::damage_consume_result::success:
				any = true;
				break;
			case chamber::damage_consume_result::damage_exhaust:
				if(!any){
					return collision_result::none;
				}
			case chamber::damage_consume_result::hitpoint_exhaust:
				any = true;
				break;
			default: break;
			}
		}

		return any ? collision_result::obj_passed : collision_result::none;
	}
}
