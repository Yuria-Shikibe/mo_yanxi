module mo_yanxi.game.ecs.component.chamber.turret;

import mo_yanxi.game.ecs.component.physical_property;
import mo_yanxi.game.ecs.entitiy_decleration;

import mo_yanxi.graphic.draw;
import mo_yanxi.ui.assets;
import mo_yanxi.ui.graphic;
import mo_yanxi.ui.creation.generic;
import mo_yanxi.ui.elem.text_elem;
import mo_yanxi.ui.elem.image_frame;
import mo_yanxi.ui.elem.progress_bar;
import mo_yanxi.game.ui.bars;

import mo_yanxi.game.meta.instancing;
import mo_yanxi.math.rand;

import std;


namespace mo_yanxi::game::ecs::chamber{
	struct turret_ui : entity_info_table{
		ui::progress_bar* reload_progress_bar{};

		[[nodiscard]] turret_ui(ui::scene* scene, group* group, const entity_ref& ref)
			: entity_info_table(scene, group, ref){

			template_cell.set_external({false, true});
			function_init([](ui::label& elem){
				elem.set_style();
				elem.set_scale(.5);
				elem.set_policy(font::typesetting::layout_policy::auto_feed_line);
				elem.set_text("Turret");
			}).cell().set_pad({.top = 8});

			{
				auto bar = end_line().emplace<ui::progress_bar>();
				bar.cell().set_pad(4);
				bar.cell().set_height(50);
				bar->approach_speed = 0.125f;

				reload_progress_bar = std::to_address(bar);
			}
		}

		void update(float delta_in_ticks) override{
			entity_info_table::update(delta_in_ticks);

			if(!ref)return;
			auto& build = ref->at<turret_build>();

			reload_progress_bar->update_progress(build.reload / build.body.reload_duration, delta_in_ticks);
		}
	};

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
			draw::line::line(acquirer.get(), pos, motion.pos(), 4, ui::theme::colors::theme, ui::theme::colors::accent);

			if(auto rst = estimate_shoot_hit_delay(target.local_pos(), motion.vel.vec, cache_.projectile_speed, body.shoot_type.offset.trunk.x)){
				auto mov = rst * motion.vel.vec;
				draw::line::line(acquirer.get(), pos, motion.pos() + mov, 4, ui::theme::colors::clear, ui::theme::colors::accent);
				draw::line::square(acquirer, {motion.pos() + mov, 45 * math::deg_to_rad}, 32, 3, ui::theme::colors::accent);

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
}
