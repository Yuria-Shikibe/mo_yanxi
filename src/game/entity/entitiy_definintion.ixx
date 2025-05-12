module;

#include <gch/small_vector.hpp>

export module mo_yanxi.game.ecs.entitiy_decleration;

export import mo_yanxi.game.ecs.component.manifold;
export import mo_yanxi.game.ecs.component.chamber;
export import mo_yanxi.game.ecs.component.projectile.manifold;
export import mo_yanxi.game.ecs.component.hitbox;
export import mo_yanxi.game.ecs.component.faction;

export import mo_yanxi.game.ecs.component.projectile.drawer;
export import mo_yanxi.game.ecs.component.projectile.ui_builder;
export import mo_yanxi.game.ecs.component.drawer;

import std;

namespace mo_yanxi::game::ecs{
	namespace decl{
		export using grided_entity_desc = std::tuple<
			mech_motion,
			manifold,
			physical_rigid,
			faction_data,
			chamber::chamber_manifold
		>;

		export using projectile_entity_desc = std::tuple<
			mech_motion,
			manifold,
			physical_rigid,
			faction_data,
			projectile_manifold,
			projectile_drawer,
			projectile_ui_builder

		>;

		using View = tuple_to_comp_t<projectile_entity_desc>;
	}

	export
	template <>
	struct ecs::archetype_custom_behavior<decl::grided_entity_desc> : archetype_custom_behavior_base<decl::grided_entity_desc>{
		static void on_init(value_type& comps){
			auto [motion, mf] = get_unwrap_of<mech_motion, manifold>(comps);
			mf.hitbox.set_trans_unchecked(motion.trans);
			mf.hitbox.update(motion.trans);
		}
	};

	export
	template <>
	struct ecs::archetype_custom_behavior<decl::projectile_entity_desc> : archetype_custom_behavior_base<decl::projectile_entity_desc>{
		static void on_init(value_type& comps){
			auto [motion, mf, dmg] = get_unwrap_of<mech_motion, manifold, projectile_manifold>(comps);
			mf.hitbox.set_trans_unchecked(motion.trans);
			mf.hitbox.update(motion.trans);
			mf.collider = projectile_collider{};

			dmg.max_damage_group.material_damage.direct = 3000;
			dmg.current_damage_group = dmg.max_damage_group;

			comps.faction = faction_1;

			auto [drawer] = get_unwrap_of<projectile_drawer>(comps);
			drawer.trail_style.color.from = graphic::colors::aqua.to_light();
			drawer.trail_style.color.to = graphic::colors::aqua.to_light().set_a(0);//.to_light();
			drawer::rect_drawer d{
				.extent = {mf.hitbox[0].box.get_identity().size},
				.color_scl = graphic::colors::aqua.to_light(),
			};
			drawer.drawer = d;
		}
	};
}
