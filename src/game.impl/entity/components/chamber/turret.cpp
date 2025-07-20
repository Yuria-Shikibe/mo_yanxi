module mo_yanxi.game.ecs.component.chamber.turret;

import mo_yanxi.game.ecs.component.physical_property;
import mo_yanxi.game.ecs.entitiy_decleration;

import mo_yanxi.graphic.draw;
import mo_yanxi.ui.assets;
import mo_yanxi.ui.graphic;
import mo_yanxi.ui.creation.generic;
import mo_yanxi.ui.elem.label;
import mo_yanxi.ui.elem.image_frame;
import mo_yanxi.ui.elem.progress_bar;
import mo_yanxi.game.ui.bars;
import mo_yanxi.game.ui.building_pane;

import mo_yanxi.game.meta.instancing;
import mo_yanxi.math.rand;

import std;


namespace mo_yanxi::game::ecs::chamber{
	struct turret_ui : entity_info_table{
		ui::building_pane* pane;

		[[nodiscard]] turret_ui(ui::scene* scene, group* group, const entity_ref& e_ref)
			: entity_info_table(scene, group, e_ref){
			set_style();

			template_cell.set_external({false, true});

			{
				auto bar = end_line().emplace<ui::building_pane>(ref);
				bar.cell().set_pad(4);
				bar.cell().set_external({false, true});
				bar->set_style(ui::theme::styles::side_bar_whisper);

				if(ref){
					auto& build = ref->at<turret_build>();
					bar->get_bar_pane().info.set_reload(build.reload / build.body.reload_duration);
				}


				pane = std::to_address(bar);
			}
		}

		void update(float delta_in_ticks) override{
			entity_info_table::update(delta_in_ticks);

			if(!ref)return;
			auto& build = ref->at<turret_build>();
			auto& build_data = build.data();

			pane->get_bar_pane().info.set_reload(build.reload / build.body.reload_duration);

		}
	};

	math::optional_vec2<float> get_best_tile_of(const building_data& data) noexcept{
		if(data.get_capability_factor() <= 0.f)return math::nullopt_vec2<float>;

		unsigned idx{};
		float lastHp{};
		for (const auto & [i, tile_state] : data.tile_states | std::views::enumerate){
			if(tile_state.valid_hit_point > lastHp){
				lastHp = tile_state.valid_hit_point;
				idx = i;
				if(lastHp > data.get_tile_individual_max_hitpoint() * .5f)break;
			}
		}

		auto lpos = data.index_to_local(idx);
		return {grid_coord_to_real(lpos.as<int>() + data.get_src_coord())};
	}

	void turret_build::build_hud(ui::table& where, const entity_ref& eref) const{
		auto hdl = where.emplace<turret_ui>(eref);
	}

	void turret_build::draw_hud(graphic::renderer_ui& renderer) const{
		auto acquirer = ui::get_draw_acquirer(renderer);
		using namespace graphic;
		auto trs = get_local_to_global_trans(transform.vec);
		auto pos = trs.vec;

		if(body.range.from > 0){
			draw::line::circle(acquirer, pos, body.range.from, 3, ui::theme::colors::red_dusted);
		}
		draw::line::circle(acquirer, pos, body.range.to, 5, ui::theme::colors::pale_green, ui::theme::colors::pale_green);

		auto offset_dir = math::vec2::from_polar_rad(rotation.radians() + trs.rot);
		draw::line::line(acquirer.get(), pos + offset_dir * 32, pos + offset_dir * 96, 12, ui::theme::colors::accent, ui::theme::colors::accent);


		if(target){
			auto& motion = target.entity->at<mech_motion>();
			auto target_pos = get_local_to_global(target.local_pos());
			draw::line::line(acquirer.get(), pos, target_pos, 4, ui::theme::colors::theme, ui::theme::colors::accent);

			if(auto rst = estimate_shoot_hit_delay(target_pos, motion.vel.vec, cache_.projectile_speed, body.shoot_type.offset.trunk.x)){
				auto mov = rst * motion.vel.vec;
				draw::line::line(acquirer.get(), pos, target_pos + mov, 4, ui::theme::colors::clear, ui::theme::colors::accent);
				draw::line::square(acquirer, {target_pos + mov, 45 * math::deg_to_rad}, 32, 3, ui::theme::colors::accent);

			}
		}

	}

	void turret_build::shoot(math::trans2 shoot_offset, const chunk_meta& chunk_meta, world::entity_top_world& top_world) const{
		if(body.shoot_type.projectile){
			auto hdl = meta::create(top_world.component_manager, *body.shoot_type.projectile);
			auto& comp = hdl.get_components();
			shoot_offset.rot += rotation;
			const auto trs = get_local_to_global_trans(shoot_offset | transform);

			hdl.set_initial_trans(trs);
			hdl.set_initial_vel(trs.rot + math::rand{}(body.shoot_type.angular_inaccuracy));
			hdl.set_faction(chunk_meta.id()->at<faction_data>().faction);
		}

	}

	bool turret_build::update_target() noexcept{
		if(target){
			if(
				!target.update(transform.get_trans() | data().get_trans()) ||
				!validate_target(target.local_pos())){
				target = nullptr;
			}else{
				return true;
			}
		}
		return false;
	}

	game::target turret_build::get_new_target() noexcept{
		return {data().grid().grid_targets.get_optimal()};
	}

	void turret_build::target_subtarget() noexcept{
		if(update_subtarget()){
			return;
		}

		const auto& grid = target.entity->at<chamber_manifold>();

		const auto trs = get_local_to_global_trans(transform.get_trans());
		const auto target_local = grid.get_transform().apply_inv_to(trs);

		//TODO quad_tree to building
		if(!grid.local_grid.quad_tree()->intersect_then(target_local.vec, [this](math::vec2 pos, const math::frect& region){
			return region.overlap_ring(pos, body.range.from, body.range.to);
		}, [this](auto&, const tile& tile){
			if(!tile.building)return false;
			if(auto* build = tile.building.build_if()){
				auto& data = build->data();
				if(auto best = get_best_tile_of(data)){
					target.target_local_offset = best;
					sub_target = tile.building;
					return true;
				}

				return false;
			}

			return false;
		})){
			target = nullptr;
		}

	}

	bool turret_build::update_subtarget() noexcept{
		if(!sub_target)return false;
		if(auto best = get_best_tile_of(sub_target.data())){
			target.target_local_offset = best;
			return true;
		}

		return false;
	}
}
