module;

#include <cassert>
#include <plf_hive.h>

export module mo_yanxi.game.ui.grid_editor;


export import mo_yanxi.game.meta.grid;

import mo_yanxi.ui.basic;
import mo_yanxi.ui.table;
import mo_yanxi.game.ui.viewport;
import mo_yanxi.ui.elem.button;
import mo_yanxi.ui.manual_table;
import mo_yanxi.ui.elem.text_elem;
import mo_yanxi.ui.elem.scroll_pane;
import mo_yanxi.ui.menu;
import mo_yanxi.ui.creation.generic;

import mo_yanxi.ui.assets;
import mo_yanxi.ui.creation.file_selector;
import mo_yanxi.ui.creation.generic;


import mo_yanxi.game.meta.hitbox;
import mo_yanxi.game.meta.chamber;
import mo_yanxi.type_map;
import mo_yanxi.game.ecs.component.hitbox;


import std;

namespace mo_yanxi::game{
	mo_yanxi::game::meta::hitbox_transed load_hitbox_from(const std::filesystem::path& path);

	template <typename T, std::invocable<math::vector2<T>> Fn>
	void each_tile(const math::rect_ortho<T>& region, const math::vector2<T> stride, Fn fn){
		for(int y = region.get_src_y(); y < region.get_end_y(); y += stride.y){
			for(int x = region.get_src_x(); x < region.get_end_x(); x += stride.x){
				std::invoke(fn, math::vector2<T>{x, y});
			}
		}
	}


	using chamber_meta = const meta::chamber::basic_chamber*;
	namespace ui{

		export using namespace mo_yanxi::ui;

		export struct grid_editor_viewport final : ui::viewport<ui::manual_table>{
		private:

			struct grid_info_bar : ui::elem{
				[[nodiscard]] grid_info_bar(ui::scene* scene, ui::group* group)
					: elem(scene, group){
				}

			private:
				grid_editor_viewport& get_editor() const{
					return *static_cast<grid_editor_viewport*>(get_parent());
				}

				void draw_content(const ui::rect clipSpace) const override;
			};

			meta::hitbox_transed reference{};
			math::optional_vec2<float> last_click_{math::vectors::constant2<float>::SNaN};

			bool overview{false};

			scroll_pane* current_selected_building_info_pane_{};
			grid_info_bar* grid_info_bar_{};

		public:
			using index_coord = math::upoint2;

			meta::chamber::grid grid{};

			chamber_meta current_chamber{};
			meta::chamber::grid_building* selected_building{};
		private:
			meta::chamber::ui_edit_context edit_context{};


		public:
			[[nodiscard]] grid_editor_viewport(scene* scene, group* group)
				: viewport(scene, group){
				camera.set_scale_range({0.25f, 2});
				property.maintain_focus_until_mouse_drop = true;
				{
					auto pane = emplace<ui::scroll_pane>();
					pane.cell().region_scale = rect{0, 0, 0.3, 0.5f};
					pane.cell().align = align::pos::bottom_left;
					pane->set_elem([](ui::table& table){
						table.set_style();
						table.template_cell.pad.top = 8;
					});
					current_selected_building_info_pane_ = &pane.elem();
					current_selected_building_info_pane_->visible = false;
				}

				{
					auto pane = emplace<grid_info_bar>();
					pane.cell().region_scale = rect{0, 0, 0.1, 0.8f};
					pane.cell().align = align::pos::top_right;

					grid_info_bar_ = &pane.elem();
				}

			}

			void draw_content(const rect clipSpace) const override;

			void set_reference(const meta::hitbox_transed& hitbox_transed){
				reference = hitbox_transed;
				reference.apply();
				grid = meta::chamber::grid{reference};
			}

			[[nodiscard]] math::frect get_region_at(math::irect region_in_world) const noexcept;
			[[nodiscard]] math::frect get_region_at(math::urect region_in_index) const noexcept{
				return get_region_at(region_in_index.as<int>().move(grid.get_origin_offset()));
			}

			[[nodiscard]] static math::point2 get_world_pos_to_tile_coord(math::vec2 coord) noexcept;


		private:
			void update_selected_building(meta::chamber::grid_building* building){
				auto& t = current_selected_building_info_pane_->get_item_unchecked<table>();
				if(building){
					if(selected_building != building){
						t.clear_children();
						building->get_meta_info().build_ui(t);
						if(auto ist = building->get_instance_data()){
							auto hdl = ist->build_ui(t, edit_context);
							if(hdl.has_ui()){
								t.end_line();
								auto spl = t.create(creation::general_seperator_line{.stroke = 20, .palette =  ui::theme::style_pal::front_white.copy().mul_alpha(.25f)});
								spl.cell().pad.set_vert(8);
								spl.cell().margin.set_hori(8);
								t.end_line();
								hdl.resume();
							}
						}

					}
					current_selected_building_info_pane_->visible = true;
				}else{
					t.clear_children();
					current_selected_building_info_pane_->visible = false;
				}
				selected_building = building;
			}

			void update(float delta_in_ticks) override{
				viewport::update(delta_in_ticks);

				if(overview){
					camera.set_scale_range({0.025f, 0.25f});
					camera.clamp_target_scale();
					viewport_region = {viewport_default_radius * 12, viewport_default_radius * 12};
				}else{
					camera.set_scale_range({0.5f, 2});
					camera.clamp_target_scale();
					viewport_region = {viewport_default_radius * 4, viewport_default_radius * 4};
				}
			}

			void on_focus_changed(bool is_focused) override{
				viewport::on_focus_changed(is_focused);
				last_click_.reset();
			}

			input_event::click_result on_click(const input_event::click click_event) override{


				if(last_click_ && click_event.code.on_release()){
					switch(click_event.code.key()){
					case core::ctrl::mouse::LMB:
						if(current_chamber){
							math::irect place_region = get_selected_place_region(current_chamber->extent, get_select_box());
							each_tile(place_region, current_chamber->extent.as<int>(), [&, this](math::point2 pos){
								if(const auto idx = grid.coord_to_index(pos)){
									if(auto* b = grid.try_place_building_at(idx.value(), *current_chamber)){
										update_selected_building(b);
									}
								}
							});
						}else{
							auto p = get_current_tile_coord();
							if(auto idx = grid.coord_to_index(p)){
								update_selected_building(grid[*idx].building);
							}
						}
						break;
					case core::ctrl::mouse::RMB: {
						const math::irect place_region = get_selected_place_region({1, 1}, get_select_box());
						if(selected_building){
							auto reg = selected_building->get_indexed_region().as<int>().move(grid.get_origin_offset());
							if(reg.overlap_exclusive(place_region)){
								update_selected_building(nullptr);
							}
						}
						each_tile(place_region, {1, 1}, [&, this](math::point2 pos){
							if(const auto idx = grid.coord_to_index(pos)){
								grid.erase_building_at(idx.value());
							}
						});
					}
						break;
					default: break;
					}
				}

				if(click_event.code.on_press()){
					last_click_ = get_transferred_pos(click_event.pos);
				}else{
					last_click_.reset();
				}

				return viewport::on_click(click_event);
			}

			esc_flag on_esc() override{
				if(last_click_){
					last_click_.reset();
					return esc_flag::intercept;
				}

				if(current_chamber){
					current_chamber = nullptr;
					return esc_flag::intercept;
				}

				return viewport::on_esc();
			}

			void input_key(const core::ctrl::key_code_t key, const core::ctrl::key_code_t action, const core::ctrl::key_code_t mode) override{
				if(action == core::ctrl::act::release){
					if(key == core::ctrl::key::M){
						overview = !overview;
					}
				}
			}

			[[nodiscard]] math::rect_ortho_trivial<float> get_select_box() const noexcept{
				auto cur = get_transferred_cursor_pos();

				if(last_click_){
					if((get_scene()->get_input_mode() & core::ctrl::mode::ctrl_shift) == core::ctrl::mode::ctrl_shift){
						return  math::rect_ortho_trivial<float>::from_vert(last_click_, cur);
					}
					if(get_scene()->get_input_mode() & core::ctrl::mode::ctrl){
						auto line = math::vectors::snap_segment_to_angle(last_click_, cur, math::pi_half);
						return math::rect_ortho_trivial<float>::from_vert(last_click_, line);
					}
				}

				return {cur};
			}

			[[nodiscard]] math::irect get_selected_place_region(mo_yanxi::math::usize2 extent, mo_yanxi::math::rect_ortho_trivial<float> rect) const noexcept;
			[[nodiscard]] math::point2 get_current_tile_coord() const noexcept;
			[[nodiscard]] math::point2 get_tile_coord(math::vec2 world_pos, math::usize2 extent) const noexcept;
		};

		export struct grid_editor : table{
		private:
			table* side_menu{};
			menu* menu_{};
			grid_editor_viewport* viewport{};

			type_map<std::vector<chamber_meta>, meta::chamber::chamber_types> chambers{};

			struct chamber_info_elem : ui::table{
			private:
				grid_editor* editor_{};
				chamber_meta chamber{};

				void build();

			public:
				[[nodiscard]] chamber_info_elem(scene* scene, group* group, grid_editor& editor, const meta::chamber::basic_chamber& chamber)
					: table(scene, group), editor_{&editor}, chamber(&chamber){

					build();
					interactivity = interactivity::intercepted;
				}

				ui::input_event::click_result on_click(const ui::input_event::click click_event) override{
					if (click_event.code.matches(core::ctrl::lmb_click)){
						if(activated){
							editor_->viewport->current_chamber = nullptr;
						}else{
							editor_->viewport->current_chamber = chamber;
						}
					}

					return table::on_click(click_event);
				}

				void update(float delta_in_ticks) override{
					table::update(delta_in_ticks);
					activated = editor_->viewport->current_chamber == chamber;
				}
			};

			void load_chambers();

			void build_menu();

		public:
			grid_editor(const grid_editor& other) = delete;
			grid_editor(grid_editor&& other) noexcept = delete;
			grid_editor& operator=(const grid_editor& other) = delete;
			grid_editor& operator=(grid_editor&& other) noexcept = delete;

			[[nodiscard]] grid_editor(scene* scene, group* group)
				: table(scene, group){

				auto m = emplace<table>();
				m.cell().set_width(300);
				m.cell().pad.right = 8;
				m->template_cell.set_height(60);
				m->template_cell.pad.bottom = 6;
				side_menu = &m.elem();

				const auto editor_region = emplace<table>();
				editor_region->set_style();

				auto vp = editor_region->emplace<grid_editor_viewport>();
				vp->set_style(ui::theme::styles::general_static);
				vp.cell().pad.bottom = 8;

				viewport = std::to_address(vp);

				auto menu = editor_region->end_line().emplace<ui::menu>();
				menu->get_button_group_pane().set_scroll_bar_stroke(10);
				// menu->get_button_group().template_cell.margin.set(4);
				menu->set_button_group_height(60);
				menu.cell().set_height(300);
				menu_ = std::to_address(menu);


				// rst.elem.set_style();

				load_chambers();
				build_menu();
			}
		};
	}
}
