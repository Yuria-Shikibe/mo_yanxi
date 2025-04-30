module;

#include <gch/small_vector.hpp>

export module mo_yanxi.game.ui.hitbox_editor;

export import mo_yanxi.ui.basic;
export import mo_yanxi.ui.table;
export import mo_yanxi.ui.manual_table;
export import mo_yanxi.ui.elem.text_elem;
export import mo_yanxi.ui.creation.file_selector;

export import mo_yanxi.game.ecs.component.hitbox;
export import mo_yanxi.game.ecs.quad_tree;

import mo_yanxi.graphic.camera;
import mo_yanxi.ui.graphic;
import mo_yanxi.ui.assets;
import mo_yanxi.graphic.layers.ui.grid_drawer;

import mo_yanxi.snap_shot;
import mo_yanxi.history_stack;

import std;

namespace mo_yanxi::game{
	struct editor_hitbox_meta{
		/** @brief Raw Box Data */
		math::rect_box_identity<float> box{};
		math::trans2 trans{};
		float scale{1};

		[[nodiscard]] math::frect_box crop() const noexcept{
			return static_cast<math::rect_box<float>>(math::rect_box_posed{box * scale, trans});
		}

		[[nodiscard]] math::vec2 get_box_v00() const noexcept{
			return box.offset | trans;
		}

		[[nodiscard]] math::vec2 get_src_pos() const noexcept{
			return trans.vec;
		}
	};

	//TODO mid operation snapshot
	struct hitbox_comp_wrap{
		editor_hitbox_meta base{};
		snap_shot<editor_hitbox_meta> edit{};

		[[nodiscard]] hitbox_comp_wrap() = default;

		[[nodiscard]] hitbox_comp_wrap(const editor_hitbox_meta& base)
			: base(base), edit(base){
		}
	};

	export
	template <>
	struct quad_tree_trait_adaptor<hitbox_comp_wrap*, float> : quad_tree_adaptor_base<hitbox_comp_wrap*, float>{
		[[nodiscard]] static rect_type get_bound(const_reference self) noexcept{
			return self->base.crop().get_bound();
		}

		[[nodiscard]] static bool intersect_with(const_reference self, const_reference other){
			const auto lhs = self->base.crop();
			const auto rhs = other->base.crop();
			return lhs.overlap_rough(rhs) && lhs.overlap_exact(rhs);
		}

		[[nodiscard]] static bool contains(const_reference self, vector_type::const_pass_t point){
			const auto lhs = self->base.crop();
			return lhs.contains(point);
		}

	};

	namespace ui{
		export using namespace mo_yanxi::ui;

		export
		struct hit_box_editor : table{
		private:

			struct editor_viewport : elem{
				enum struct edit_op{
					none,
					move,
					move_eccentrically,
					rotate,
					scale,
				};

				struct op{
					math::vec2 initial_pos{};
					edit_op operation{};
					bool precision_mode{};
					std::string cmd{};

					void reset(){
						cmd.clear();
						precision_mode = false;
						operation = {};
						initial_pos.set_NaN();
					}

					void start(op op){
						this->operator=(op);
					}

					explicit operator bool() const noexcept{
						return this->operation != edit_op::none;
					}

					[[nodiscard]] float get_scale() const noexcept{
						return precision_mode ? 0.2f : 1.f;
					}
				};


				static constexpr float editor_radius = 5000;
				static constexpr float editor_margin = 2000;

				math::vec2 last_camera_pos{};
				graphic::camera2 camera{};

				op operation{};

				std::vector<hitbox_comp_wrap> comps{};
				history_stack<hitbox_comp> history{12};

				hitbox_comp_wrap* main_selected{};
				std::unordered_set<hitbox_comp_wrap*> selected{};
				quad_tree<hitbox_comp_wrap*> quad_tree{math::frect{math::vec2{}, (editor_radius + editor_margin) * 2}};
				bool quad_tree_changed{};

				[[nodiscard]] editor_viewport(scene* scene, group* group)
					: elem(scene, group, "hitbox_viewport"){

					set_style(ui::assets::styles::general_static);

					register_event([](ui::events::focus_begin e, editor_viewport& self){
						self.get_scene()->set_camera_focus(&self.camera);
						self.set_focused_key(true);
						self.set_focused_scroll(true);
					});

					register_event([](ui::events::focus_end e, editor_viewport& self){
						self.get_scene()->set_camera_focus(nullptr);
						self.set_focused_key(false);
						self.set_focused_scroll(false);
					});

					register_event([](ui::events::scroll e, editor_viewport& self){
						self.camera.set_scale_by_delta(e.pos.y * 0.05f);
					});

					register_event([](const ui::events::drag& e, editor_viewport& self){
						if(e.code.key() == core::ctrl::mouse::CMB){
							auto src = self.getTransferredPos(e.pos);
							auto dst = self.getTransferredPos(e.dst);
							self.camera.set_center(self.last_camera_pos - (dst - src));
						}
					});

					camera.speed_scale = 0;
					camera.set_scale_range({0.25f, 2});
					camera.flip_y = true;

					add_comp({0, 0, 200, 200});
				}

				void update(float delta_in_ticks) override{
					elem::update(delta_in_ticks);
					camera.clamp_position({math::vec2{}, editor_radius * 2});
					camera.update(delta_in_ticks);

					if(std::exchange(quad_tree_changed, false)){
						quad_tree->reserved_clear();
						for (auto & comp : comps){
							quad_tree->insert(&comp);
						}
					}

					apply_op_to_temp();
					update_boxes();

				}

				bool resize(const math::vec2 size) override{
					if(elem::resize(size)){
						auto [x, y] = content_size();
						camera.resize_screen(x, y);
						return true;
					}

					return false;
				}

				void draw_content(const rect clipSpace) const override{
					const auto proj = camera.get_world_to_uniformed_flip_y();

					get_renderer().batch.push_projection(proj);
					get_renderer().batch.push_viewport(prop().content_bound_absolute());
					get_renderer().batch.push_scissor({camera.get_viewport()});

					auto& r = get_renderer();

					using namespace graphic;
					draw_acquirer acquirer{r.batch, draw::white_region};

					{
						batch_layer_guard guard(r.batch, std::in_place_type<layers::grid_drawer>);

						draw::fill::rect_ortho(acquirer.get(), camera.get_viewport(), colors::black.copy().set_a(.35));
					}

					for (const auto & comp : comps){
						draw::line::quad(acquirer, comp.base.crop(), 2, colors::gray.copy().set_a(.5));
					}

					for (const auto & comp : comps){
						draw::line::quad(acquirer, comp.edit.temp.crop(), 2, colors::light_gray);
						draw::line::square(acquirer, {comp.edit.temp.get_src_pos(), math::pi / 4}, 8, 2, colors::ORANGE);
						draw::line::line_angle(acquirer.get(), math::trans2{16} | comp.edit.temp.trans, 64, 2, colors::ORANGE, colors::ORANGE);

					}

					// for (const auto & comp : selected){
					// 	draw::line::quad(acquirer, comp->comp.temp.box, 2, colors::aqua);
					// }

					if(main_selected){
						draw::line::quad(acquirer, main_selected->edit.temp.crop(), 2, ui::assets::colors::accent);
					}


					get_renderer().batch.pop_scissor();
					get_renderer().batch.pop_viewport();
					get_renderer().batch.pop_projection();
				}

				ui::events::click_result on_click(const ui::events::click click_event) override{
					if(click_event.code.key() == core::ctrl::mouse::CMB){
						last_camera_pos = camera.get_stable_center();
					}

					if(click_event.code.matched({core::ctrl::mouse::LMB, core::ctrl::act::release})){
						if(operation){
							apply_op();
						}else{
							bool any{false};
							quad_tree->intersect_then(getTransferredPos(get_scene()->cursor_pos), [this, &any](hitbox_comp_wrap* w, auto){
								try_add_select(*w);
								any = true;
							});
							if(!any){
								clear_selected();
							}
						}
					}

					return ui::events::click_result::intercepted;
				}

				void add_comp(math::rect_box_identity<float> box, math::trans2 trans2 = {}){
					//Make sure no dangling
					clear_selected();
					quad_tree->reserved_clear();

					comps.push_back({{box, trans2}});
					quad_tree_changed = true;
				}

				esc_flag on_esc() override{
					if(operation){
						reset_op();
						return esc_flag::intercept;
					}

					return esc_flag::fall_through;
				}


				void input_key(const core::ctrl::key_code_t key, const core::ctrl::key_code_t action, const core::ctrl::key_code_t mode) override{

					if(action == core::ctrl::act::release){

						auto cpos = getTransferredPos(get_scene()->get_cursor_pos());
						switch(key){
						case core::ctrl::key::G:{
							if(operation)break;
							switch(mode){
							case core::ctrl::mode::ctrl :{
								operation.start({cpos, edit_op::move_eccentrically});
								break;
							}
								default:
								operation.start({cpos, edit_op::move});
							}

							break;
						}
						case core::ctrl::key::S:{
							if(operation)break;
							operation.start({cpos, edit_op::scale});
							break;
						}
						case core::ctrl::key::R:{
							if(operation)break;
							operation.start({cpos, edit_op::rotate});
							break;
						}
						case core::ctrl::key::Enter:{
							apply_op();
							break;
						}

						default: break;

						}
					}

					if(operation){
						switch(key){
						case core::ctrl::key::Right_Shift :
						case core::ctrl::key::Left_Shift :{
							switch(action){
							case core::ctrl::act::press:
								operation.precision_mode = true;
								break;
							case core::ctrl::act::release:
								operation.precision_mode = false;
								break;
							default: break;
							}

							save_edit_mid_data();
							break;
						}
						default:break;
						}
					}


				}


				std::string_view get_current_op_cmd() const noexcept{
					return operation ? operation.cmd : std::string_view{};
				}

				void input_unicode(const char32_t val) override{
					if(!operation)return;
					auto v = static_cast<char>(val);
					if(v != val)return;
					if(std::isspace(static_cast<char>(val)))return;
					operation.cmd.push_back(static_cast<char>(val));
				}

			private:
				void save_edit_mid_data(){
					apply_op_to_temp_base();
					operation.initial_pos = getTransferredPos(get_scene()->get_cursor_pos());
				}

				void reset_op(){
					operation.reset();
					for (auto hitbox_comp_wrap : selected){
						hitbox_comp_wrap->edit = hitbox_comp_wrap->base;
					}
				}

				void apply_op(){
					apply_op_to_temp();
					apply_op_to_temp_base();
					apply_op_to_base();
					operation.reset();
				}

				void clear_selected() noexcept{
					selected.clear();
					main_selected = nullptr;
				}

				void try_add_select(hitbox_comp_wrap& w){
					if(main_selected == &w){
						main_selected = nullptr;
						selected.erase(&w);
					}else{
						main_selected = &w;
						selected.insert(&w);
					}
				}

				void add_select(hitbox_comp_wrap& w){
					selected.insert(&w);
				}

				void remove_select(hitbox_comp_wrap& w){
					if(main_selected == &w){
						main_selected = nullptr;
					}
					selected.erase(&w);
				}

				void apply_eccentric_move(hitbox_comp_wrap& wrap, math::vec2 src, math::vec2 dst){
					auto mov = (dst - src) * operation.get_scale();
					wrap.edit.temp.box.offset = wrap.edit.base.box.offset + mov.rotate_rad(-wrap.base.trans.rot);
				}

				void apply_move(hitbox_comp_wrap& wrap, math::vec2 src, math::vec2 dst) noexcept{
					auto mov = (dst - src) * operation.get_scale();
					wrap.edit.temp.trans.vec = wrap.edit.base.trans.vec + mov;//.rotate_rad(wrap.base.trans.rot);
				}

				void apply_scale(hitbox_comp_wrap& wrap, math::vec2 src, math::vec2 dst, math::vec2 avg) noexcept{
					const auto src_dst = math::max<float>(wrap.base.trans.vec.dst(src), 15);
					const auto cur_dst = wrap.base.trans.vec.dst(dst);

					const auto scl = math::max(cur_dst / src_dst, 0.01f);
					wrap.edit.temp.scale = wrap.edit.base.scale * ((scl - 1) * operation.get_scale() + 1);
				}

				void apply_rotate(hitbox_comp_wrap& wrap, math::vec2 src, math::vec2 dst, math::vec2 avg) noexcept{
					const auto approach_src = src - wrap.base.trans.vec;
					const auto approach_dst = dst - wrap.base.trans.vec;

					wrap.edit.temp.trans.rot = wrap.edit.base.trans.rot + approach_src.angle_between_rad(approach_dst) * operation.get_scale();
				}

				void apply_op_to_temp(){
					if(!main_selected || selected.empty())return;

					const auto cur_pos = getTransferredPos(get_scene()->cursor_pos);
					const auto main_pos = main_selected->edit.temp.trans.vec;
					const auto avg_pos = std::ranges::fold_left(selected | std::views::transform([](const hitbox_comp_wrap* w){
							return w->base.trans.vec;
						}), math::vec2{}, std::plus<>{}) / selected.size();

					switch(operation.operation){
					case edit_op::move :{
						for (auto wrap : selected){
							apply_move(*wrap, operation.initial_pos, cur_pos);
						}
						break;
					}
					case edit_op::move_eccentrically :{
						for (auto wrap : selected){
							apply_eccentric_move(*wrap, operation.initial_pos, cur_pos);
						}
						break;
					}
					case edit_op::scale :{
						for (auto wrap : selected){
							apply_scale(*wrap, operation.initial_pos, cur_pos, avg_pos);
						}
						break;
					}
					case edit_op::rotate :{
						for (auto wrap : selected){
							apply_rotate(*wrap, operation.initial_pos, cur_pos, avg_pos);
						}
						break;
					}
					default : break;
					}

				}

				void apply_op_to_temp_base(){
					for (auto wrap : selected){
						wrap->edit.apply();
					}
				}

				void apply_op_to_base(){
					for (auto wrap : selected){
						wrap->base = wrap->edit.base;
					}
					quad_tree_changed = true;
				}

				[[nodiscard]] math::vec2 getTransferredPos(const math::vec2 pos){
					return camera.get_screen_to_world(pos, content_src_pos(), true);
				}

				void update_boxes() noexcept{
					if(operation && !selected.empty()){
						quad_tree_changed = true;
					}
				}
			};

			table* menu{};
			editor_viewport* viewport{};
			basic_text_elem* cmd_text{};

		public:
			[[nodiscard]] hit_box_editor(scene* scene, group* group)
				: table(scene, group){

				auto m = emplace<table>();
				m.cell().set_width(80);
				m.cell().pad.right = 8;

				menu = std::to_address(m);

				auto editor_region = emplace<manual_table>();
				editor_region->set_style();

				auto vp = editor_region->emplace<editor_viewport>();
				vp.cell().region_scale = {tags::from_extent, math::vec2{}, math::vec2{1.f, 1.f}};
				// vp->prop().fill_parent.set(true);
				viewport = std::to_address(vp);

				auto hint = editor_region->emplace<basic_text_elem>();
				hint.cell().region_scale = {tags::from_extent, math::vec2{}, math::vec2{1.f, 0.2f}};
				hint.cell().align = align::pos::top_left;
				hint->set_style();
				hint->set_policy(font::typesetting::layout_policy::auto_feed_line);
				hint->set_scale(.5f);
				hint->interactivity = interactivity::disabled;
				cmd_text = std::to_address(hint);

			}

			void update(float delta_in_ticks) override{
				table::update(delta_in_ticks);

				cmd_text->set_text(viewport->get_current_op_cmd());
			}
		};
	}
}