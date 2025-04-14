//
// Created by Matrix on 2025/4/14.
//

export module mo_yanxi.game.ecs.system.motion_system;

export import mo_yanxi.game.ecs.component_manager;
export import mo_yanxi.game.ecs.component.physical_property;

import std;

namespace mo_yanxi::game::ecs{
	export
	struct motion_system{
		void run(component_manager& manager){
			manager.sliced_each([](
				component_manager& m,
				component<mech_motion>& motion
				// component<physical_rigid>& rigid
			){

				motion->apply_and_reset(m.update_delta);
				if(auto rigid = m.get_entity_partial_chunk<physical_rigid>(motion.id())){
					motion->apply_drag(m.update_delta, rigid->val().drag);
				}
			});
		}
	};
}