module;


module mo_yanxi.game.ecs.component.manifold;

import mo_yanxi.game.ecs.component.projectile.manifold;
import mo_yanxi.game.ecs.component.chamber;
import mo_yanxi.game.ecs.component.faction;

import mo_yanxi.game.ecs.entitiy_decleration;


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


	collision_result projectile_collider::try_collide_to(
		const collision_object& projectile, const collision_object& chamber_grid,
		const intersection& intersection) noexcept{

		auto tgt_faction = chamber_grid.id->try_get<faction_data>();
		auto self_faction = projectile.id->at<faction_data>();
		if(tgt_faction && tgt_faction->faction == self_faction.faction){
			return collision_result::none;
		}

		auto chambers = chamber_grid.id->try_get<chamber::chamber_manifold>();
		if(!chambers)return collision_result::passed;

		auto& dmg = projectile.id->at<projectile_manifold>();
		auto rst = getCollidedTileWith(projectile, chamber_grid, intersection, *chambers);
		if(rst.empty())return collision_result::none;

		bool any{false};
		
		for (auto && under_hit : rst){
			auto& data = under_hit.building.data();
			switch(data.consume_damage(under_hit.tile_pos, dmg.current_damage_group)){
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

	entity_id projectile_collider::get_group_identity(entity_id self, const manifold& manifold){
		return ecs::chunk_neighbour_of<projectile_manifold, desc::projectile>(manifold).owner;
	}


	bool projectile_collider::collide_able_to(
		const collision_object& sbj,
		const collision_object& obj) noexcept{

		auto& projectile_chunk = ecs::chunk_of<desc::projectile>(*sbj.manifold);

		if(projectile_chunk.projectile_manifold::owner == obj.manifold->get_hit_group_id(obj.id))return false;

		if(const auto faction = obj.id->try_get<faction_data>()){
			if(faction->faction.get_id() == projectile_chunk.faction_data::faction.get_id())return false;
		}

		return true;
	}

}
