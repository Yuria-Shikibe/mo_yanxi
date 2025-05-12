module;

#include <gch/small_vector.hpp>

export module mo_yanxi.game.ecs.system.projectile;

export import mo_yanxi.game.ecs.component.manager;

export import mo_yanxi.game.ecs.component.projectile.drawer;
export import mo_yanxi.game.ecs.component.projectile.manifold;
export import mo_yanxi.game.ecs.component.manifold;

namespace mo_yanxi::game::ecs::system{
	export
	struct projectile{
		void run(component_manager& manager, world::graphic_context& graphic_context){
			manager.sliced_each([&](
				component_manager& manager,
				const chunk_meta& meta,
				const manifold& manifold,
				const mech_motion& motion,
				projectile_manifold& projectile_manifold,
				projectile_drawer& drawer){
					const auto trail_trans = drawer.trail_style.trans | motion.trans;
					drawer.trail.update(manager.get_update_delta(), trail_trans.offset);

					drawer.clip = manifold.hitbox.max_wrap_bound().expand_by(drawer.trail.get_bound());

					if(projectile_manifold.current_damage_group.sum() <= 0.f){
						manager.mark_expired(meta.id());
						drawer.trail_style.dump_to_effect(graphic_context.create_efx(), std::move(drawer.trail), trail_trans, motion.vel.vec.length());
					}
				});
		}
	};
}
