//
// Created by Matrix on 2025/6/12.
//

export module mo_yanxi.ui.creation.field_edit;

export import mo_yanxi.ui.primitives;
import mo_yanxi.ui.elem.slider;
export import mo_yanxi.ui.elem.text_input_area;
import mo_yanxi.ui.elem.manual_table;
import mo_yanxi.ui.assets;
import mo_yanxi.ui.elem.table;

import std;

namespace mo_yanxi::ui{
	export
	struct field_editor : ui::manual_table{
		using value_type = float;
	private:
		struct builtin_input_area final : numeric_input_area{
			using numeric_input_area::numeric_input_area;

			void on_target_updated() override{
				auto& slider = *get_editor().slider_;
				if(slider.disabled)return;

				float val = std::visit([]<typename T>(const T& t){
					if constexpr(spec_of<T, edit_target_range_constrained>){
						return t.get_ratio_of_current();
					}

					return 0.f;
				}, get_edit_target());
				get_editor().slider_->set_progress({math::clamp(val), 0});
			}

			[[nodiscard]] field_editor& get_editor() const noexcept{
				return static_cast<field_editor&>(*get_parent());
			}

		};
		struct builtin_slider final : slider{
			[[nodiscard]] builtin_slider(scene* scene, group* group)
				: slider(scene, group){

				set_tooltip_state({
					.layout_info = tooltip_layout_info{
						.follow = tooltip_follow::owner,
						.align_owner = align::pos::bottom_left,
						.align_tooltip = align::pos::top_left,
					},
					// .use_stagnate_time = true,
				}, [this](ui::table& t){
					auto str = std::visit([this]<typename T>(const T& t){
						return t.get_range_string();
					}, get_editor().input_area->get_edit_target());
					t.visible = !str.empty();
					t.function_init([&](label& label){
						label.set_scale(.5f);
						label.set_style();
						label.set_text(std::move(str));
					}).cell().set_external({true, true});
				});
			}

			input_event::click_result on_click(const input_event::click click_event) override{
				if(click_event.code.mode() & core::ctrl::mode::alt){
					if(click_event.code.action() == core::ctrl::act::release)applyLast();
					return input_event::click_result::spread;
				}
				return slider::on_click(click_event);
			}

			void on_drag(const input_event::drag event) override{
				if(event.code.mode() & core::ctrl::mode::alt)return;
				slider::on_drag(event);
			}

		private:

			void update(float delta_in_ticks) override{
				slider::update(delta_in_ticks);

				visible = std::visit([]<typename T>(const T&){
					return spec_of<T, ui::edit_target_range_constrained>;
				}, get_editor().input_area->get_edit_target());
				disabled = !visible;

				if(visible){
					if(is_sliding()){
						notify_update(bar_progress_.temp.x);
					}
				}
			}

			[[nodiscard]] field_editor& get_editor() const noexcept{
				return static_cast<field_editor&>(*get_parent());
			}

		public:
			void on_data_changed() override{
				notify_update(get_progress().x);
			}

		private:
			void notify_update(float p) const {
				if(std::visit([p]<typename T>(T& target){
					return target.set_from_progress(p);
				}, get_editor().input_area->get_edit_target())){
					get_editor().input_area->fetch_target();
				}
			}
		};

		struct editor_text_input_area : text_input_area{
			[[nodiscard]] editor_text_input_area(scene* scene, group* group)
				: text_input_area(scene, group){
			}
		};
		builtin_input_area* input_area{};
		// label* unit_label{};
		builtin_slider* slider_{};

	public:
		[[nodiscard]] field_editor(scene* scene, group* group)
			: manual_table(scene, group){

			{
				auto hdl = emplace<struct builtin_input_area>();
				input_area = &hdl.elem();
				hdl.cell().region_scale = {0, 0, 1, 1};
				hdl.cell().margin.set(8);
				hdl->set_style();
				hdl->set_fit();
				hdl->text_entire_align = align::pos::center;
			}

			{
				auto hdl = emplace<builtin_slider>();
				slider_ = &hdl.elem();
				hdl.cell().region_scale = {0, 0, 1, 1};
				hdl->set_style(theme::styles::no_edge);
				hdl->set_hori_only();
				hdl->bar_base_size = {20, 20};
			}

			// {
			// 	auto hdl = emplace<label>();
			// 	unit_label = &hdl.elem();
			// 	hdl.cell().region_scale = {0, 0, 1, 1};
			// 	hdl.cell().maximum_size = {std::numeric_limits<float>::infinity(), 80};
			// 	hdl.cell().margin.set(8);
			// 	hdl->set_style();
			// 	hdl->set_fit();
			// 	hdl->set_text("%");
			// 	hdl->interactivity = interactivity::disabled;
			// 	hdl->text_entire_align = align::pos::center_right;
			// }
		}

		// void set_unit_text(const std::string_view text){
		// 	unit_label->set_text(text);
		// }
		//
		// void set_unit_text(const char* text){
		// 	unit_label->set_text(text);
		// }
		//
		// void set_unit_text(std::string&& text){
		// 	unit_label->set_text(std::move(text));
		// }

		template <typename T>
		void set_edit_target(T&& tgt){
			input_area->set_target(std::forward<T>(tgt));
			input_area->on_target_updated();
		}

		void draw_content(const rect clipSpace) const override;
	};

	export
	struct named_field_editor final : table{

	private:
		field_editor* editor{};
		label* name{};
		label* unit{};
	public:
		[[nodiscard]] named_field_editor(scene* scene, group* group)
			: table(scene, group){
			{
				auto hdl = emplace<label>();
				hdl.cell().set_height(60);
				hdl.cell().saturate = true;
				hdl.cell().pad.bottom = 8;
				hdl->set_style();
				hdl->set_fit();
				hdl->prop().graphic_data.inherent_opacity = 0.5f;
				hdl->text_entire_align = align::pos::center_left;
				name = &hdl.elem();
			}


			{
				auto hdl = end_line().emplace<field_editor>();
				editor = &hdl.elem();
				hdl->set_style();
			}


			{
				auto hdl = emplace<label>();
				unit = &hdl.elem();
				hdl->set_fit();
				hdl->set_style();
				hdl->text_entire_align = align::pos::center_right;
				hdl.cell().set_width(120);
				hdl.cell().pad.left = 16;
			}
		}

		void set_name_height(float height){
			cells[0].cell.set_height(height);
			notify_isolated_layout_changed();
		}

		void set_editor_height(float height){
			cells[1].cell.set_height(height);
			cells[2].cell.set_height(height);
			notify_isolated_layout_changed();
		}

		void set_unit_label_size(math::vec2 size){
			cells[2].cell.set_size(size);
			notify_isolated_layout_changed();
		}



		[[nodiscard]] field_editor& get_editor() const{
			return *editor;
		}

		[[nodiscard]] label& get_name_label() const{
			return *name;
		}

		[[nodiscard]] label& get_unit_label() const{
			return *unit;
		}
	};
}