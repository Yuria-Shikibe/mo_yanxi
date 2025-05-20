module mo_yanxi.game.ecs.component.chamber.radar;

import mo_yanxi.game.ecs.component.physical_property;
import mo_yanxi.game.ecs.entitiy_decleration;

import mo_yanxi.graphic.draw;

import mo_yanxi.ui.assets;
import mo_yanxi.ui.creation.seperator_line;
import mo_yanxi.ui.elem.text_elem;
import mo_yanxi.ui.elem.image_frame;
import mo_yanxi.ui.elem.progress_bar;

import mo_yanxi.ui.graphic;

import std;


namespace mo_yanxi::game::ecs::chamber{
	struct radar_ui : entity_info_table{
		ui::progress_bar* reload_progress_bar{};
		[[nodiscard]] radar_ui(ui::scene* scene, group* group, const entity_ref& ref)
			: entity_info_table(scene, group, ref){

			template_cell.set_external({false, true});
			function_init([](ui::label& elem){
				elem.set_style();
				elem.set_scale(.5);
				elem.set_policy(font::typesetting::layout_policy::auto_feed_line);
				elem.set_text("Radar");
			}).cell().set_pad({.top = 8});


			{
				auto bar = end_line().emplace<ui::progress_bar>();
				bar.cell().set_pad(4);
				bar.cell().set_height(50);
				bar->reach_speed = 0.125f;

				reload_progress_bar = std::to_address(bar);
			}
		}

		void update(float delta_in_ticks) override{
			entity_info_table::update(delta_in_ticks);

			if(!ref)return;
			auto& build = ref->at<radar_build>();

			reload_progress_bar->update_progress(build.reload / build.meta.reload_duration, delta_in_ticks);
		}
	};

	void radar_build::draw_hud(graphic::renderer_ui& renderer) const{
		using namespace graphic;

		ui::draw_acquirer acquirer{ui::get_draw_acquirer(renderer)};

		auto center = get_local_to_global(meta.local_center);

		if(meta.targeting_range.from > 0)draw::line::circle(acquirer, center, meta.targeting_range.from, 3);
		draw::line::circle(acquirer, center, meta.targeting_range.to, 5, ui::theme::colors::theme, ui::theme::colors::theme);

		for (const auto & candidate : data().grid().targets_primary.get_candidates()){
			if(!candidate.ref)continue;
			auto pos = candidate.ref->at<mech_motion>().pos();
			auto radius = candidate.ref->at<manifold>().hitbox.get_wrap_radius();

			draw::fancy::select_rect(acquirer, {pos, radius * 2}, 4, ui::theme::colors::theme, radius / 4);
		}

		if(auto tgt = data().grid().targets_primary.get_optimal()){
			auto pos = tgt->at<mech_motion>().pos();
			auto radius = tgt->at<manifold>().hitbox.get_wrap_radius();

			draw::line::square(acquirer, {pos, 45 * math::deg_to_rad}, radius * 1.5f, 4, ui::theme::colors::accent);
		}
	}

	void radar_build::build_hud(ui::table& where, const entity_ref& eref) const{
		auto hdl = where.emplace<radar_ui>(eref);
	}

	void radar_build::update(const chunk_meta& chunk_meta, world::entity_top_world& top_world){
		if(const auto rst = math::forward_approach_then(reload, meta.reload_duration, get_update_delta())){
			if(active){
				reload = 0;

				auto faction = chunk_meta.id()->at<faction_data>().faction;

				data().grid().targets_primary.index_candidates_by_distance(
					top_world.collision_system.quad_tree(),
					chunk_meta.id(),
					get_local_to_global(meta.local_center),
					meta.targeting_range, [&](const collision_object& obj){
						return faction->is_hostile_to(obj.id->at<faction_data>().faction.get_id());
					});
			}
		}else{
			reload = rst;
		}
	}
}
