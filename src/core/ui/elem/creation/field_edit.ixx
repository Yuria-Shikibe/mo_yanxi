//
// Created by Matrix on 2025/6/12.
//

export module mo_yanxi.ui.creation.field_edit;

export import mo_yanxi.ui.basic;
import mo_yanxi.ui.elem.slider;
import mo_yanxi.ui.elem.text_input_area;
import mo_yanxi.ui.manual_table;
import mo_yanxi.ui.assets;

import std;

namespace mo_yanxi::ui{
	export
	struct edit_floating_point : ui::manual_table{
		using value_type = float;
	private:
		struct edit_slider : slider{
			using slider::slider;

			input_event::click_result on_click(const input_event::click click_event) override{
				if(click_event.code.mode() & core::ctrl::mode::alt){
					if(click_event.code.action() == core::ctrl::act::release)applyLast();
					return input_event::click_result::spread;
				}
				slider::on_click(click_event);
			}

			void on_drag(const input_event::drag event) override{
				if(event.code.mode() & core::ctrl::mode::alt)return;
				slider::on_drag(event);
			}
		};
		struct editor_text_input_area : text_input_area{
			[[nodiscard]] editor_text_input_area(scene* scene, group* group)
				: text_input_area(scene, group){
			}
		};
		value_type* target{};
		numeric_input_area* input_area{};
		label* unit_label{};
		slider* slider_{};

	public:
		[[nodiscard]] edit_floating_point(scene* scene, group* group)
			: manual_table(scene, group){

				{
					auto hdl = emplace<numeric_input_area>();
					input_area = &hdl.elem();
					hdl.cell().region_scale = {0, 0, 1, 1};
					hdl.cell().margin.set(8);
					hdl->set_style();
					hdl->set_fit();
					hdl->text_entire_align = align::pos::center_left;
				}

				{
					auto hdl = emplace<edit_slider>();
					slider_ = &hdl.elem();
					hdl.cell().region_scale = {0, 0, 1, 1};
					hdl->set_style(theme::styles::no_edge);
					hdl->set_hori_only();
					hdl->bar_base_size = {30, 30};
				}

				{
					auto hdl = emplace<label>();
					unit_label = &hdl.elem();
					hdl.cell().region_scale = {0, 0, 1, 1};
					hdl.cell().maximum_size = {std::numeric_limits<float>::infinity(), 80};
					hdl.cell().margin.set(8);
					hdl->set_style();
					hdl->set_fit();
					hdl->set_text("%");
					hdl->interactivity = interactivity::disabled;
					hdl->text_entire_align = align::pos::center_right;
				}
		}
	};
}