module mo_yanxi.game.ecs.component.chamber.radar;

import mo_yanxi.game.ecs.component.physical_property;
import mo_yanxi.game.ecs.entitiy_decleration;

import mo_yanxi.graphic.draw;

import mo_yanxi.ui.assets;
import mo_yanxi.ui.creation.generic;
import mo_yanxi.ui.elem.label;
import mo_yanxi.ui.elem.image_frame;
import mo_yanxi.ui.elem.progress_bar;
import mo_yanxi.ui.elem.collapser;

import mo_yanxi.game.ui.chamber_ui_elem;
import mo_yanxi.game.ui.building_pane;

import mo_yanxi.ui.graphic;

import std;


namespace mo_yanxi::game::ecs::chamber{
	struct radar_ui : ui::default_building_ui_elem{
		[[nodiscard]] radar_ui(ui::scene* scene, group* parent, const building_entity_ref& e)
			: default_building_ui_elem(scene, parent, e){
		}

		void update(float delta_in_ticks) override{
			default_building_ui_elem::update(delta_in_ticks);

			if(!entity)return;
			auto& build = entity->at<radar_build>();
			get_bar_pane().info.set_reload(build.reload / build.meta.reload_duration);
		}
	};

	void radar_build::draw_hud_on_building_selection(graphic::renderer_ui_ref renderer) const{
		using namespace graphic;

		auto acquirer{mo_yanxi::ui::get_draw_acquirer(renderer)};

		auto trs = get_local_to_global_trans(meta.transform);
		auto center = trs.vec;

		draw::fancy::draw_targeting_range_auto(acquirer, trs, meta.targeting_range_radius, meta.targeting_range_angular, 5, colors::gray, colors::aqua);

		for (const auto & candidate : data().grid().grid_targets.get_candidates()){
			if(!candidate.ref)continue;
			auto pos = candidate.ref->at<mech_motion>().pos();
			auto radius = candidate.ref->at<manifold>().hitbox.get_wrap_radius();

			draw::fancy::select_rect(acquirer, {pos, radius * 2}, 4, ui::theme::colors::theme, radius / 4);
		}

		if(auto tgt = data().grid().grid_targets.get_optimal()){
			auto pos = tgt->at<mech_motion>().pos();
			auto radius = tgt->at<manifold>().hitbox.get_wrap_radius();

			draw::line::square(acquirer, {pos, 45 * math::deg_to_rad}, radius * 1.5f, 4, ui::theme::colors::accent);
		}
	}

	void radar_build::build_hud(ui::list& where, const entity_ref& eref) const{
		auto hdl = where.emplace<radar_ui>(eref);
	}

	void radar_build::update(world::entity_top_world& top_world){
		auto cpt = data().hit_point.get_capability_factor();
		if(cpt > 0){
			if(const auto rst =
			math::forward_approach_then(reload, meta.reload_duration, get_update_delta() * cpt * data().get_efficiency())){
				if(active){
					reload = 0;

					auto& selfE = data().grid().self_entity();

					auto faction = selfE.at<faction_data>().faction;

					data().grid().grid_targets.index_candidates_by_distance(
						top_world.collision_system.quad_tree(),
						selfE.id(),
						get_local_to_global(meta.transform.vec),
						meta.targeting_range_radius, [&](const collision_object& obj){
							return obj.id->has<chamber_manifold>() && faction->is_hostile_to(obj.id->at<faction_data>().faction.get_id());
						});
				}
			}else{
				reload = rst;
			}
		}else{
			math::lerp_inplace(reload, 0.f, get_update_delta());
		}

	}
}
