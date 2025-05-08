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

					drawer.trail.update(manager.get_update_delta(), (drawer.trail_trans | motion.trans).offset);

					drawer.clip = manifold.hitbox.max_wrap_bound().expand_by(drawer.trail.get_bound());

					if(projectile_manifold.damage_group.sum() <= 0.f){
						manager.mark_expired(meta.id());
						graphic_context.create_efx().set_data(fx::effect_data{
							.style = fx::trail_effect{
								.trail = std::move(drawer.trail),
								.color = {
									.in = {drawer.trail_color.from, drawer.trail_color.from},
									.out = {drawer.trail_color.to, drawer.trail_color.to.copy().set_a(0)}
								}
							},
							.trans = {motion.pos(), drawer.trail_radius},
							.depth = motion.depth,
							.duration = std::max<float>(30, drawer.trail.estimate_duration(motion.vel.vec.length())),
						});
					}
				});
		}
	};
}