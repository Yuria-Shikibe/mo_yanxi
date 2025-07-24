module;


export module mo_yanxi.game.ecs.system.grid_system;

export import mo_yanxi.game.ecs.component.manage;
export import mo_yanxi.game.ecs.component.chamber;
export import mo_yanxi.game.ecs.component.physical_property;
export import mo_yanxi.game.world.graphic;

import mo_yanxi.math;
import mo_yanxi.game.graphic.effect;

import std;

namespace mo_yanxi::game::ecs::system{
	export
	struct grid_system{
		void run(component_manager& manager, world::graphic_context& graphic){
			manager.sliced_each([&](
				const mech_motion& motion,
				chamber::chamber_manifold& grid
			){
				grid.update_transform(motion.trans);

				grid.manager.sliced_each([&](
					chamber::building_data& data,
					chamber::building& building
				){
					for (auto && damage_event : data.damage_events){
						graphic.create_efx().set_data(fx::effect_data{
							.style = fx::poly_outlined_out{
								.radius = {5, 25, math::interp::pow3Out},
								.stroke = {4},
								.palette = {
									{graphic::colors::aqua.to_light()},
									{graphic::colors::aqua.to_light()},
									{graphic::colors::clear},
									{
										graphic::colors::aqua.create_lerp(graphic::colors::white, .5f).to_light().set_a(.5f),
										graphic::colors::clear,
									}
								}
							},
							.trans = {grid.tile_coord_to_global(damage_event.local_coord.as<int>() + data.region().src)},
							.depth = 0,
							.duration = {60}
						});
					}

					data.clear_hit_events();

					auto hp = data.get_tile_individual_max_hitpoint();
					for (auto & tile_state : data.tile_states){
						math::approach_inplace(tile_state.valid_hit_point, hp, .05 * manager.get_update_delta());
					}
				});
			});
		}
	};
}