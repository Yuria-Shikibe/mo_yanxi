module;

#include <gch/small_vector.hpp>

module mo_yanxi.game.ecs.component.projectile.drawer;

import mo_yanxi.game.ecs.entitiy_decleration;
import mo_yanxi.graphic.draw.trail;

namespace mo_yanxi::game::ecs{

	void projectile_drawer::draw(const world::graphic_context& draw_ctx) const{
		const auto& motion = chunk_neighbour_of<mech_motion, decl::projectile_entity_desc>(*this);
		drawer::part_transform drawer_trs{motion.trans, motion.depth};

		drawer.draw(draw_ctx, drawer_trs);

		drawer::draw_acquirer acquirer{draw_ctx.renderer().batch, graphic::draw::white_region};
		acquirer.proj.depth = trail_style.trans.z_offset + drawer_trs.z_offset;
		graphic::draw::fancy::trail(acquirer, trail, trail_style.radius, trail_style.color.from, trail_style.color.to);
	}
}