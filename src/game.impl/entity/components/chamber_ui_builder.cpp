module;

module mo_yanxi.game.ecs.component.chamber.ui_builder;

import mo_yanxi.game.ecs.component.physical_property;
import mo_yanxi.game.ecs.entitiy_decleration;

import mo_yanxi.graphic.draw;
import mo_yanxi.ui.assets;
import mo_yanxi.ui.creation.seperator_line;
import mo_yanxi.ui.elem.text_elem;
import mo_yanxi.ui.elem.image_frame;
import mo_yanxi.ui.elem.progress_bar;
import mo_yanxi.ui.elem.progress_bar;

import std;


namespace mo_yanxi::game::ecs::chamber{
	struct chamber_ui : entity_info_table{
		ui::progress_bar* hitpoint_bar{};

		[[nodiscard]] chamber_ui(ui::scene* scene, group* group, const entity_ref& ref)
			: entity_info_table(scene, group, ref){

			template_cell.set_external({false, true});
			function_init([](ui::label& elem){
				elem.set_style();
				elem.set_scale(.75);
				elem.set_policy(font::typesetting::layout_policy::auto_feed_line);
				elem.set_text("Complex");
			}).cell().set_pad({.top = 8});


			{
				auto txt = end_line().emplace<ui::label>();
				txt->set_style();
				txt->set_text(std::format("Faction: {}", ref->at<faction_data>().faction.get_id()));
				txt->set_scale(.5);
				txt->set_policy(font::typesetting::layout_policy::auto_feed_line);

				txt.cell().set_pad({.top = 8});
			}

			{
				auto bar = end_line().emplace<ui::progress_bar>();
				bar.cell().set_pad(4);
				bar.cell().set_height(40);
				bar->reach_speed = 0.125f;

				hitpoint_bar = std::to_address(bar);
			}


			{
				auto sep = end_line().create(ui::creation::general_seperator_line{
						.stroke = 20,
						.palette = ui::theme::style_pal::front_white.copy().mul_alpha(.25f)
					});
				sep.cell().pad.set_vert(4);
				sep.cell().margin.set_hori(8);
			}


			if(auto mf = this->ref->try_get<chamber_manifold>()){
				end_line().function_init([this, &manifold = *mf](ui::table& elem){
					elem.set_style();
					elem.template_cell.set_external({false, true});
					elem.template_cell.pad.bottom = 8;

					manifold.manager.sliced_each([&](
						const chunk_meta& meta,
						const ui_builder& builder){
						builder.build_hud(elem.end_line(), meta.id());
					});

					elem.set_edge_pad(0);
				}).cell().set_external({false, true});
			}
		}

		void update(float delta_in_ticks) override{
			entity_info_table::update(delta_in_ticks);

			if(!ref)return;

			auto& chunk = ref->unchecked_get<decl::grided_entity_desc>();

			hitpoint_bar->update_progress(chunk.hit_point.factor(), delta_in_ticks);
		}
	};

	void chamber_ui_builder::build_hud(ui::table& where, const entity_ref& eref) const{
		auto hdl = where.emplace<chamber_ui>(eref);
	}
}