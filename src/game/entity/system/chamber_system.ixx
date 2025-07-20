module;

#include <cassert>

export module mo_yanxi.game.ecs.system.chamber;

export import mo_yanxi.game.ecs.component.manage;
export import mo_yanxi.game.ecs.component.chamber;

namespace mo_yanxi::game::ecs::system{
	export
	struct chamber{
		void run(world::entity_top_world& top_world) const{
			top_world.component_manager.sliced_each([&](
				const component_manager& manager,
				const chunk_meta& meta,
				ecs::chamber::chamber_manifold& grid,
				const mech_motion& motion
			){
					grid.update_transform(motion.trans);

					grid.manager.do_deferred();
					grid.manager.update_delta = manager.get_update_delta();

					grid.manager.sliced_each([&](
						ecs::chamber::building& building){
							building.update(meta, top_world);
						});


					float dt = manager.get_update_delta();
					unsigned valid_sum{};
					grid.manager.sliced_each(
						[&, dt](
						ecs::chamber::building_data& data, ecs::chamber::power_generator_building_tag){
							auto valid = data.update_energy_state(dt);
							assert(valid >= 0);
							valid_sum += valid;
						});

					grid.energy_allocator.update_allocation(valid_sum);
					grid.manager.sliced_each(
						[dt](
						ecs::chamber::building_data& data, ecs::chamber::power_consumer_building_tag){
							data.update_energy_state(dt);
						});

					bool inbound = grid.get_wrap_bound().overlap(top_world.graphic_context.viewport());
					grid.manager.sliced_each(
						[&, dt](
						ecs::chamber::building_data& data
					){
							// data.update_energy_state(dt);

							if(inbound)
								for(auto&& damage_event : data.damage_events){
									top_world.graphic_context.create_efx().set_data(fx::effect_data{
											.style = fx::poly_outlined_out{
												.radius = {5, 25, math::interp::pow3Out},
												.stroke = {4},
												.palette = {
													{graphic::colors::aqua.to_light()},
													{graphic::colors::aqua.to_light()},
													{graphic::colors::clear},
													{
														graphic::colors::aqua.create_lerp(
															                      graphic::colors::white, .5f).
														                      to_light().set_a(.5f),
														graphic::colors::clear,
													}
												}
											},
											.trans = {
												grid.local_to_global(
													damage_event.local_coord.as<int>() + data.region().src)
											},
											.depth = 0,
											.duration = {60}
										});
								}

							data.hit_point.accept(data.building_damage_take);
							grid.hit_point.accept(data.structural_damage_take);

							data.clear_hit_events();

							auto hp = data.get_tile_individual_max_hitpoint();
							float healed{};
							for(auto& tile_state : data.tile_states){
								math::approach_inplace_get_delta(tile_state.valid_structure_hit_point, hp,
								                                 .5 * dt);

								if(tile_state.valid_structure_hit_point > hp / 2){
									healed += math::approach_inplace_get_delta(tile_state.valid_hit_point, hp,
									                                           .5 * dt);
								}
							}
							data.hit_point.heal(healed);
						});
				});
		}
	};
}
