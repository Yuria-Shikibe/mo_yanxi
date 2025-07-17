module;

module mo_yanxi.game.ecs.component.projectile.ui_builder;

import mo_yanxi.game.ecs.component.physical_property;
import mo_yanxi.game.ecs.entitiy_decleration;

import mo_yanxi.graphic.draw;
import mo_yanxi.ui.assets;
import mo_yanxi.ui.creation.generic;
import mo_yanxi.ui.elem.label;
import mo_yanxi.ui.elem.image_frame;
import mo_yanxi.ui.elem.progress_bar;

import std;

namespace mo_yanxi::game::ecs{
	struct projectile_ui : entity_info_table{
		ui::label* position_info{};
		ui::label* team_info{};
		ui::progress_bar* damage_bar{};

		[[nodiscard]] projectile_ui(ui::scene* scene, group* group, const entity_ref& ref)
			: entity_info_table(scene, group, ref){

			template_cell.set_external({false, true});
			function_init([](ui::label& elem){
				elem.set_style();
				elem.set_scale(.75);
				elem.set_policy(font::typesetting::layout_policy::auto_feed_line);
				elem.set_text("Projectile");
			}).cell().set_pad({.top = 8});

			{
				auto txt = end_line().emplace<ui::label>();
				txt->set_style();
				txt->set_scale(.5);
				txt->set_policy(font::typesetting::layout_policy::auto_feed_line);

				txt.cell().set_pad({.top = 8});

				team_info = std::to_address(txt);
			}

			{
				auto bar = end_line().emplace<ui::progress_bar>();
				bar.cell().set_pad(4);
				bar.cell().set_height(50);
				bar->approach_speed = 0.125f;

				damage_bar = std::to_address(bar);
			}

			{
				auto sep = end_line().create(ui::creation::general_seperator_line{
						.stroke = 20,
						.palette = ui::theme::style_pal::front_white.copy().mul_alpha(.25f)
					});
				sep.cell().pad.set(4);
				sep.cell().margin.right = 16;
			}

			{
				auto txt = end_line().emplace<ui::label>();
				txt->set_style();
				txt->set_scale(.5);
				txt->set_policy(font::typesetting::layout_policy::auto_feed_line);

				position_info = std::to_address(txt);
			}
		}

		void update(float delta_in_ticks) override{
			entity_info_table::update(delta_in_ticks);

			if(!ref)return;

			auto& chunk = ref->unchecked_get<decl::projectile_entity_desc>();


			position_info->set_text(std::format("At {:.1f}", chunk.mech_motion::pos()));
			team_info->set_text(std::format("Faction: {}", chunk.faction_data::faction.get_id()));
			damage_bar->update_progress({
				.current = chunk.current_damage_group.sum() / chunk.max_damage_group.sum()
			}, delta_in_ticks);
		}
	};

	void projectile_ui_builder::build_hud(ui::table& where, const entity_ref& eref) const{
		auto hdl = where.emplace<projectile_ui>(eref);

	}
}