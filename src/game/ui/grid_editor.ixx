module;

#include <cassert>
#include "plf_hive.h"

export module mo_yanxi.game.ui.grid_editor;


export import mo_yanxi.game.ecs.component.chamber;

import mo_yanxi.ui.primitives;
import mo_yanxi.ui.elem.table;
import mo_yanxi.game.ui.viewport;
import mo_yanxi.ui.elem.button;
import mo_yanxi.ui.elem.manual_table;
import mo_yanxi.ui.elem.label;
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

import :sub_panels;

namespace mo_yanxi::game{

	template <typename T, std::invocable<math::vector2<T>> Fn>
	void each_tile(const math::rect_ortho<T>& region, const math::vector2<T> stride, Fn fn){
		for(int y = region.get_src_y(); y < region.get_end_y(); y += stride.y){
			for(int x = region.get_src_x(); x < region.get_end_x(); x += stride.x){
				std::invoke(fn, math::vector2<T>{x, y});
			}
		}
	}

	enum struct operation{
		none,
		move
	};

	using chamber_meta = const meta::chamber::basic_chamber*;

	struct mirrow_axis{
		bool at_mid{};
		int coord{};
	};

	using mirrow_axis_vec2 = math::vector2<std::optional<mirrow_axis>>;

	unsigned get_mirrowed_region_count(const mirrow_axis_vec2& vec){
		if(vec.x && vec.y)return 4;
		if(vec.x || vec.y)return 2;
		return 1;
	}

	mirrow_axis_vec2 get_moved_axis(mirrow_axis_vec2 mirrow, math::ivec2 move) noexcept{
		if(mirrow.x){
			mirrow.x->coord += move.y;
		}

		if(mirrow.y){
			mirrow.y->coord += move.x;
		}

		return mirrow;
	}

	auto get_mirrowed_region(const mirrow_axis_vec2& vec, const math::irect region){
		const auto v00 = region.vert_00();
		const auto v11 = region.vert_11();

		struct ret_v{
			std::array<math::irect, 4> array{};
			unsigned sz{};

			auto begin() const noexcept {
				return array.begin();
			}

			auto end() const noexcept {
				return array.begin() + sz;
			}
		} ret{.sz = get_mirrowed_region_count(vec)};

		ret.array[ret.sz - 1] = region;

		static constexpr auto flip = [](int& coord, mirrow_axis axis){
			auto dst = coord - axis.coord;
			coord = axis.coord - dst + axis.at_mid;
		};

		auto flip_x = [&](math::point2 src, math::point2 dst) -> math::irect{
			auto mirrowed_v00 = src;
			auto mirrowed_v11 = dst;
			flip(mirrowed_v00.y, *vec.x);
			flip(mirrowed_v11.y, *vec.x);
			return math::irect{mirrowed_v00, mirrowed_v11};
		};

		auto flip_y = [&](math::point2 src, math::point2 dst) -> math::irect{
			auto mirrowed_v00 = src;
			auto mirrowed_v11 = dst;
			flip(mirrowed_v00.x, *vec.y);
			flip(mirrowed_v11.x, *vec.y);
			return math::irect{mirrowed_v00, mirrowed_v11};
		};

		if(vec.x && vec.y){
			ret.array[0] = flip_x(v00, v11);
			ret.array[1] = flip_y(v00, v11);
			ret.array[2] = flip_y(ret.array[1].vert_00(), ret.array[1].vert_11());
		}else if(vec.x){
			ret.array[0] = flip_x(v00, v11);
		}else if(vec.y){
			ret.array[0] = flip_y(v00, v11);
		}

		return ret;
	}

	struct mirrow_mode_channel{
	private:
		enum mode{
			none,
			x,
			x_mid,

			y,
			y_mid,

			COUNT
		};


		unsigned current_mode_{};
		mirrow_axis_vec2 mirrow{};


		void apply_to_axis(mirrow_axis_vec2& vec, math::point2 pos) const{
			switch(current_mode_){
			case x:
				vec.x = mirrow_axis{false, pos.y};
				break;
			case x_mid:
				vec.x = mirrow_axis{true, pos.y};
				break;
			case y:
				vec.y = mirrow_axis{false, pos.x};
				break;
			case y_mid:
				vec.y = mirrow_axis{true, pos.x};
				break;
			default: break;
			}
		}

	public:
		[[nodiscard]] bool is_editing() const noexcept{
			return current_mode_ != 0;
		}

		void exit_edit() noexcept{
			current_mode_ = 0;
		}

		void switch_mode() noexcept{
			current_mode_ = (current_mode_ + 1) % COUNT;
		}

		void erase() noexcept{
			switch(current_mode_){
			case x :[[fallthrough]];
			case x_mid : mirrow.x = std::nullopt;
				break;
			case y :[[fallthrough]];
			case y_mid : mirrow.y = std::nullopt;
				break;
			default : break;
			}
		}

		void apply(math::point2 pos) noexcept{
			apply_to_axis(mirrow, pos);
		}

		void move(math::ivec2 move) noexcept{
			mirrow = get_moved_axis(mirrow, move);
		}

		[[nodiscard]] mirrow_axis_vec2 get_current_axis() const noexcept{
			return mirrow;
		}

		[[nodiscard]] mirrow_axis_vec2 get_temp_axis(math::point2 pos) const noexcept{
			mirrow_axis_vec2 axis{};
			apply_to_axis(axis, pos);
			return axis;
		}
	};

	namespace ui{

		export using namespace mo_yanxi::ui;

		export struct grid_editor_viewport;

		enum struct detail_pane_mode : unsigned{
			statistic,
			grid,
			power_state,
			structural_state,
			maneuvering,
			corridor,
			defense_active,
			defense_passive,
			targeting_sensor,
			turret,
			damage_control,
			storage,


			monostate
		};

		struct grid_info_bar : ui::elem{
			[[nodiscard]] grid_info_bar(ui::scene* scene, ui::group* group)
				: elem(scene, group){
			}

		private:
			[[nodiscard]] grid_editor_viewport& get_editor() const;

			void draw_content(rect clipSpace) const override;
		};

		struct grid_detail_pane final : ui::manual_table{
			[[nodiscard]] grid_detail_pane(scene* scene, group* group)
				: manual_table(scene, group){
				build();
				set_mode(detail_pane_mode::monostate);
			}

			[[nodiscard]] rect get_shadow_region() const noexcept{
				return current_mode_ == detail_pane_mode::monostate ? children[0]->get_bound() : get_bound();
			}

			[[nodiscard]] grid_editor_viewport& get_viewport() const noexcept;

			bool notify_update(const detail_pane_mode where) const{
				if(current_mode_ == where){
					clear_viewport_relevant_state();
					if(auto tgt = dynamic_cast<grid_editor_panel_base*>(children[1].get())){
						tgt->refresh(refresh_channel::all);
						return true;
					}
				}

				return false;
			}
			void notify_any_update() const{
				if(current_mode_ == detail_pane_mode::maneuvering){
					if(auto tgt = dynamic_cast<grid_editor_panel_base*>(children[1].get())){
						tgt->refresh(refresh_channel::indirect);
					}
				}
			}

			[[nodiscard]] std::optional<graphic::color> get_override_color(const meta::chamber::grid_building& b) const noexcept{
				switch(current_mode_){
				case detail_pane_mode::power_state : if(auto usg = b.get_meta_info().get_energy_usage()){
						return usg < 0 ? graphic::colors::red_dusted : graphic::colors::pale_green;
					} else return std::nullopt;
				case detail_pane_mode::maneuvering : if(b.get_meta_info().get_maneuver_comp()){
						return graphic::colors::ACID;
					} else return std::nullopt;
				default : return std::nullopt;
				}
			}

			void clear(){
				set_mode(detail_pane_mode::monostate);
			}

			bool is_focused_on(detail_pane_mode mode) const noexcept{
				return current_mode_ == mode;
			}

		private:
			void clear_viewport_relevant_state() const noexcept;
			void build();

			detail_pane_mode current_mode_{};

			elem_ptr prov();

			void set_mode(detail_pane_mode mode) noexcept{
				if(mode == current_mode_)return;

				current_mode_ = mode;
				clear_viewport_relevant_state();

				if(current_mode_ != detail_pane_mode::monostate){
					exchange_element(1, prov(), true);
					if(auto tgt = dynamic_cast<grid_editor_panel_base*>(children[1].get())){
						tgt->refresh(refresh_channel::all);
					}
				}
				this->children[1]->visible = current_mode_ != detail_pane_mode::monostate;
			}
		};

		struct building_state_changed_updator{
		private:
			std::bitset<std::to_underlying(detail_pane_mode::monostate)> types{};

		public:
			[[nodiscard]] building_state_changed_updator() = default;

			void add(const meta::chamber::grid_building& building) noexcept{
				if(building.get_meta_info().is_structural()){
					types[std::to_underlying(detail_pane_mode::structural_state)] = true;
				}

				if(building.get_meta_info().get_energy_usage()){
					types[std::to_underlying(detail_pane_mode::power_state)] = true;
				}

				if(building.get_meta_info().get_maneuver_comp()){
					types[std::to_underlying(detail_pane_mode::maneuvering)] = true;
				}
			}

			void submit(grid_detail_pane& pane) const{
				bool updated = false;
				for(auto i = 0u; i < types.size(); ++i){
					if(types[i]){
						updated |= pane.notify_update(detail_pane_mode{i});
					}
				}
				if(!updated && types.any()){
					pane.notify_any_update();
				}
			}
		};

		struct grid_editor_viewport final : ui::viewport<ui::manual_table>{
		private:


			meta::hitbox_transed reference{};
			math::optional_vec2<float> last_click_{math::nullopt_vec2<float>};

			bool overview{false};

			scroll_pane* current_selected_building_info_pane_{};
			grid_detail_pane* grid_detail_pane_{};

		public:
			using index_coord = math::upoint2;

			meta::chamber::grid grid{};
			meta::chamber::path_finder path_finder{};
			chamber_meta current_chamber{};
			meta::chamber::grid_building* selected_building{};


			std::move_only_function<void(const grid_editor_viewport&) const> subpanel_drawer{};

		private:
			std::optional<math::point2> path_src{};
			std::optional<math::point2> path_dst{};
			meta::chamber::ideal_path_finder_request* current_request{};

			mirrow_mode_channel mirrow_channel{};

			meta::chamber::ui_edit_context edit_context{};

			math::vec2 operation_initial{};
			operation grid_op_;

			[[nodiscard]] math::point2 get_grid_edit_offset(math::vec2 cur_in_screen) const{
				auto cur = get_transferred_pos(cur_in_screen);
				auto mov = cur - operation_initial;
				return mov.div(ecs::chamber::tile_size).round<int>();
			}

			[[nodiscard]] bool is_focused_on_grid() const noexcept{
				return grid_detail_pane_->is_focused_on(detail_pane_mode::grid);
			}

		public:
			[[nodiscard]] grid_editor_viewport(scene* scene, group* group)
				: viewport(scene, group){
				camera.set_scale_range({0.25f, 2});
				property.maintain_focus_until_mouse_drop = true;
				{
					auto pane = emplace<ui::scroll_pane>();
					pane.cell().region_scale = rect{0, 0, 0.3, 0.5f};
					pane.cell().align = align::pos::bottom_left;
					pane->function_init([](ui::table& table){
						table.set_style();
						table.template_cell.pad.top = 8;
					});
					current_selected_building_info_pane_ = &pane.elem();
					current_selected_building_info_pane_->visible = false;
				}

				{
					auto pane = emplace<grid_detail_pane>();
					pane.cell().region_scale = rect{0, 0, 0.3, 0.7f};
					pane.cell().align = align::pos::top_right;

					grid_detail_pane_ = &pane.elem();
				}

			}

			void draw_content(const rect clipSpace) const override;

			void set_reference(const meta::hitbox_transed& hitbox_transed, bool autoSetMesh){
				reference = hitbox_transed;
				reference.apply();
				if(autoSetMesh)grid = meta::chamber::grid{reference};
			}

			void reset_editor() noexcept{
				grid_detail_pane_->clear();
				selected_building = nullptr;
				subpanel_drawer = nullptr;
			}

			[[nodiscard]] math::frect get_region_at(math::irect region_in_world) const noexcept;
			[[nodiscard]] math::frect get_region_at(math::urect region_in_index) const noexcept{
				return get_region_at(region_in_index.as<int>().move(grid.get_origin_offset()));
			}

		private:


			void update_selected_building(meta::chamber::grid_building* building){
				auto& t = current_selected_building_info_pane_->get_item_unchecked<table>();
				if(building){
					if(selected_building != building){
						t.clear_children();
						building->get_meta_info().build_editor_ui(t);
						if(auto ist = building->get_instance_data()){
							auto hdl = ist->build_editor_ui(t, edit_context);
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

			bool apply_grid_op(math::vec2 cursorpos){
				switch(grid_op_){
				case operation::move:
					auto mov = get_grid_edit_offset(cursorpos);
					grid.move(mov);
					mirrow_channel.move(mov);
					grid_op_ = operation::none;

					return true;
				default: return false;
				}
			}

		private:
			template <typename Fn = std::identity, std::invocable<math::irect> RegionFn = std::identity>
			void selection_each(math::usize2 extent, Fn fn = {}, RegionFn rfn = {}) const {
				const auto raw_region = get_select_box(last_click_);
				const auto tiled_region = get_selected_place_region(extent, raw_region);
				const auto region = get_mirrowed_region(mirrow_channel.get_current_axis(), tiled_region);

				for(const math::irect& place_region : region){
					std::invoke(rfn, place_region);
					if constexpr(!std::same_as<Fn, std::identity>){
						game::each_tile(place_region, extent.as<int>(), [&, this](const math::point2 pos){
							if(const auto idx = grid.coord_to_index(pos)){
								if constexpr (std::invocable<Fn, math::upoint2>){
									std::invoke(fn, *idx);
								}else if constexpr (std::invocable<Fn, math::upoint2, math::point2>){
									std::invoke(fn, *idx, pos);
								}
							}
						});
					}
				}
			}

		public:
			input_event::click_result on_click(const input_event::click click_event) override{

				using namespace core::ctrl;

				if (mirrow_channel.is_editing()){
					if(click_event.code.matches(lmb_click)){
						mirrow_channel.apply(get_world_pos_to_tile_coord(get_transferred_pos(click_event.pos)));
					}

					if(click_event.code.matches(rmb_click)){
						mirrow_channel.erase();
					}
				}else if(is_focused_on_grid()){
					if(click_event.code.matches(lmb_click_no_mode)){
						if(apply_grid_op(click_event.pos))goto fbk;
					}

					if(click_event.code.on_release()){
						selection_each({1, 1}, [&, this](math::upoint2 idx){
							switch(click_event.code.key()){
								case mouse::LMB :{
									grid.set_placeable_at(idx, true);
									break;
								}
								case mouse::RMB :{
									grid.set_placeable_at(idx, false);
									break;
								}
								default : break;
							}
						});
					}
				}else if(grid_detail_pane_->is_focused_on(detail_pane_mode::corridor)){
					if(click_event.code.on_release()){
						if(click_event.code.mode() & mode::alt){
							auto p = get_world_pos_to_tile_coord(get_transferred_cursor_pos());
							setter:
							if(!path_src){
								path_src = p;
							}else{
								if(!path_dst){
									path_dst = p;

									if(
										auto
											srcidx = grid.coord_to_index(*path_src),
											dstidx = grid.coord_to_index(*path_dst);
										srcidx && dstidx){
										if(current_request){
											path_finder.reacquire_path(*current_request, grid, *srcidx, *dstidx);
										}else{
											current_request = path_finder.acquire_path(grid, *srcidx, *dstidx);
										}

									}
								}else{
									path_src = path_dst = std::nullopt;
									goto setter;
								}
							}


						}else{
							selection_each({1, 1}, [&, this](math::upoint2 idx){
								switch(click_event.code.key()){
									case mouse::LMB :{
										grid.set_corridor_at(idx, true);
										break;
									}
									case mouse::RMB :{
										grid.set_corridor_at(idx, false);
										break;
									}
									default : break;
								}
							});
						}

					}
				}else{
					if(last_click_ && click_event.code.on_release()){
						switch(click_event.code.key()){
						case mouse::LMB : if(current_chamber){
								const meta::chamber::grid_building* any{};

								selection_each(current_chamber->extent, [&, this](math::upoint2 idx){
									if(auto* b = grid.try_place_building_at(idx, *current_chamber)){
										update_selected_building(b);
										any = b;
									}
								});

								if(any){
									building_state_changed_updator upd{};
									upd.add(*any);
									upd.submit(*grid_detail_pane_);
								}
							} else{
								auto p = get_current_tile_coord();
								if(auto idx = grid.coord_to_index(p)){
									update_selected_building(grid[*idx].building);
								}
							}
							break;
						case mouse::RMB :{
							building_state_changed_updator upd{};

							selection_each(
								{1, 1},
								[&, this](math::upoint2 idx){
									auto cascade = grid.erase_building_at(
										idx, std::bind_front(&building_state_changed_updator::add, &upd));
									for(const auto& building_po : cascade.building_pos){
										grid.erase_building_at(
											building_po,
											std::bind_front(&building_state_changed_updator::add, &upd));
									}
								}, [this](math::irect place_region){
									if(selected_building){
										const auto reg =
											selected_building->get_indexed_region().as<int>().move(grid.get_origin_offset());
										if(reg.overlap_exclusive(place_region)){
											update_selected_building(nullptr);
										}
									}
								});
							upd.submit(*grid_detail_pane_);
						}
						break;
						default : break;
						}
					}

				}

				if(click_event.code.on_press()){
					last_click_ = get_transferred_pos(click_event.pos);
				}else{
					last_click_.reset();
				}

				fbk:

				return viewport::on_click(click_event);
			}

			esc_flag on_esc() override{
				if(mirrow_channel.is_editing()){
					mirrow_channel.exit_edit();
					return esc_flag::intercept;
				}else if(is_focused_on_grid()){
					if(grid_op_ != operation::none){
						grid_op_ = operation::none;
						return esc_flag::intercept;
					}

				}else{
					if(last_click_){
						last_click_.reset();
						return esc_flag::intercept;
					}

					if(current_chamber){
						current_chamber = nullptr;
						return esc_flag::intercept;
					}
				}


				return viewport::on_esc();
			}

			void on_key_input(const core::ctrl::key_code_t key, const core::ctrl::key_code_t action, const core::ctrl::key_code_t mode) override{
				if(action == core::ctrl::act::release){
					switch(key){
					case core::ctrl::key::Space:
						overview = !overview; break;
					case core::ctrl::key::G:{
						if(is_focused_on_grid()){
							if(grid_op_ == operation::move){
								grid_op_ = operation::none;
							}else{
								grid_op_ = operation::move;
								operation_initial = get_transferred_cursor_pos();
							}

						}
						break;
					}
					case core::ctrl::key::M:{
						mirrow_channel.switch_mode();
						break;
					}
					case core::ctrl::key::Enter:{
						apply_grid_op(get_scene()->get_cursor_pos());
						break;
					}
						default: break;
					}


				}
			}

			[[nodiscard]] math::rect_ortho_trivial<float> get_select_box(math::vec2 initial) const noexcept{
				auto cur = get_transferred_cursor_pos();

				if((get_scene()->get_input_mode() & core::ctrl::mode::ctrl_shift) == core::ctrl::mode::ctrl_shift){
					return  math::rect_ortho_trivial<float>::from_vert(initial, cur);
				}
				if(get_scene()->get_input_mode() & core::ctrl::mode::ctrl){
					auto line = math::vectors::snap_segment_to_angle(initial, cur, math::pi_half);
					return math::rect_ortho_trivial<float>::from_vert(initial, line);
				}

				return {cur};
			}

			[[nodiscard]] static math::point2 get_world_pos_to_tile_coord(math::vec2 pos_in_world) noexcept{

				return pos_in_world.div(ecs::chamber::tile_size).floor().round<int>();
			}

			[[nodiscard]] math::irect get_selected_place_region(mo_yanxi::math::usize2 extent, mo_yanxi::math::rect_ortho_trivial<float> rect) const noexcept;
			[[nodiscard]] math::point2 get_current_tile_coord() const noexcept;
			[[nodiscard]] static math::point2 get_tile_coord(math::vec2 world_pos, math::usize2 extent) noexcept;
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
