module;

export module mo_yanxi.game.ui.hitbox_editor;

export import mo_yanxi.ui.basic;
export import mo_yanxi.ui.table;
export import mo_yanxi.ui.elem.button;
export import mo_yanxi.ui.manual_table;
export import mo_yanxi.ui.elem.text_elem;
export import mo_yanxi.ui.creation.file_selector;

export import mo_yanxi.game.quad_tree;
export import mo_yanxi.game.meta.hitbox;

import mo_yanxi.graphic.camera;
import mo_yanxi.graphic.image_manage;

import mo_yanxi.ui.elem.check_box;
import mo_yanxi.ui.graphic;
import mo_yanxi.ui.assets;
import mo_yanxi.ui.selection;
import mo_yanxi.graphic.layers.ui.grid_drawer;

import mo_yanxi.char_filter;
import mo_yanxi.snap_shot;
import mo_yanxi.history_stack;

import std;

namespace mo_yanxi::game{
	using editor_box_meta = meta::hitbox::comp;

	struct editor_scalable_box_meta : editor_box_meta{
		math::vec2 scale{1, 1};

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
	struct box_wrapper{
		editor_scalable_box_meta base{};
		snap_shot<editor_scalable_box_meta> edit{};

		void apply_scaled() noexcept{
			edit.base.box *= edit.base.scale;
			edit.base.scale.set(1);
			edit.resume();
			base = edit.base;
		}

		void apply() noexcept{
			base = edit.base;
		}

		void reset_rot() noexcept{
			base.trans.rot = 0;
			edit.base.trans.rot = 0;
			edit.temp.trans.rot = 0;
		}

		void reset_pos() noexcept{
			base.trans.vec.set_zero();
			edit.base.trans.vec.set_zero();
			edit.temp.trans.vec.set_zero();
		}

		[[nodiscard]] box_wrapper() noexcept = default;

		[[nodiscard]] explicit(false) box_wrapper(const editor_scalable_box_meta& base)
			noexcept : base(base), edit(base){
		}
	};

	export
	template <>
	struct quad_tree_trait_adaptor<box_wrapper*, float> : quad_tree_adaptor_base<box_wrapper*, float>{
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

	enum struct edit_op{
		none,
		move,
		move_eccentrically,
		move_center_eccentrically,
		rotate,
		scale,
	};

	constexpr float editor_line_width = 8;
	constexpr float editor_radius = 5000;
	constexpr float editor_margin = 2000;
	constexpr char_filter cmd_filter{"1234567890+-Ss"};

	struct basic_op{
		math::vec2 initial_pos{};
		edit_op operation{};

		math::bool2 constrain{true, true};

		bool precision_mode{};
		std::string cmd{};


		void reset(){
			start({});
		}

		void start(const basic_op& op){
			this->operator=(op);
		}

		explicit operator bool() const noexcept{
			return this->operation != edit_op::none;
		}




		[[nodiscard]] float get_scale() const noexcept{
			return precision_mode ? 0.2f : 1.f;
		}

		[[nodiscard]] bool has_clamp() const noexcept{
			return !constrain.area();
		}

		bool set_constrain(const math::bool2 b){
			if(constrain == b){
				return false;
			} else{
				constrain = b;
				return true;
			}
		}


		[[nodiscard]] bool has_snap() const noexcept{
			return cmd.contains('S');
		}

		std::optional<float> get_num_from_cmd() const{
			float val{};
			auto ptr = cmd.c_str();
			auto end = cmd.data() + cmd.size();

			while(ptr != end){
				auto rst = std::from_chars(ptr, end, val);
				switch(rst.ec){
				case std::errc::result_out_of_range : ptr = rst.ptr;
					break;
				case std::errc::invalid_argument : ++ptr;
					break;
				case std::errc{} : return val;
				default : return {};
				}
			}

			return std::nullopt;
		}

	protected:
		float get_rotation(
				const math::vec2 dst,
				const math::vec2 avg) const noexcept{
			float ang;

			auto cmdv = get_num_from_cmd().transform([](const float f){ return math::deg_to_rad * -f; });
			bool snap = has_snap();

			if(!snap && cmdv){
				ang = cmdv.value();
			} else{
				const auto approach_src = initial_pos - avg;
				const auto approach_dst = dst - avg;

				ang = approach_src.angle_between_rad(approach_dst) * get_scale();
				if(snap && cmdv){
					ang = math::snap_to<float>(ang, cmdv.value(), 0);
				}
			}
			return ang;
		}

		math::vec2 get_scale(
			const math::vec2 dst,
			const math::vec2 avg) const noexcept{
			float scl;
			if(auto cmd_scl = get_num_from_cmd()){
				scl = cmd_scl.value();
			} else{
				const auto src_dst = math::max<float>(avg.dst(initial_pos), 15);
				const auto cur_dst = avg.dst(dst);
				scl = cur_dst / src_dst;
			}

			scl = std::max((scl - 1) * get_scale() + 1, 0.01f);

			return math::vec2{}.set(scl - 1) * constrain.as<float>() + math::vec2{1, 1};
		}

		[[nodiscard]] math::vec2 get_move(const math::vec2 dst) const{
			math::vec2 rst;

			if(has_clamp()){
				if(const auto mov = get_num_from_cmd()){
					rst = math::vec2{}.set(mov.value()) * constrain.as<float>();
					goto ret;
				}
			}

			rst = (dst - initial_pos) * constrain.as<float>();

		ret:
			return rst * get_scale();
		}

	};

	struct reference_image_channel{
		graphic::allocated_image_region image_region{};

		box_wrapper wrapper{};
		edit_op op{};

		void set_region(graphic::allocated_image_region&& region) noexcept{
			image_region = std::move(region);
			wrapper.base = {editor_box_meta{
				.box = {region.uv.size.as<float>().scl(-.5f), region.uv.size.as<float>()},
				.trans = {}
			}};
		}

		void draw(ui::draw_acquirer& acquirer) const{
			if(image_region){
				auto last = acquirer.get_region();
				auto mode = acquirer.proj;

				acquirer << static_cast<const ui::draw_acquirer::region_type&>(image_region);

				acquirer.proj.set_layer(ui::draw_layers::base);
				graphic::draw::fill::quad(acquirer.get(), wrapper.base.crop().view_as_quad());

				acquirer << last;
				acquirer.proj = mode;
				graphic::draw::line::quad(acquirer, wrapper.base.crop().view_as_quad(), editor_line_width, graphic::colors::gray.copy().set_a(.5f));
			}
		}
	};

	struct hitbox_edit_channel{

		struct history_entry : editor_box_meta{
			bool selected{};
			bool main_selected{};
		};

		enum struct reference_frame{
			global,
			local,
			COUNT
		};

		enum struct reference_center{
			local,
			average,
			selected
		};

		enum struct select_mode{
			single,
			join,
			intersection,
			subtract,
			difference,
		};

		struct op : basic_op{
			reference_frame frame{};
			reference_center center{};

			void switch_frame(){
				frame = reference_frame{(std::to_underlying(frame) + 1) % std::to_underlying(reference_frame::COUNT)};
			}
		private:
			[[nodiscard]] math::vec2 get_move(const box_wrapper& wrap,
			                                  const math::vec2 dst) const{
				auto mov = get_num_from_cmd();
				bool has_snap = this->has_snap();
				if(has_clamp() && !has_snap && mov){
					auto rst = math::vec2{}.set(mov.value());
					switch(frame){
					case reference_frame::local : rst.rotate_rad(-wrap.base.trans.rot);
						rst *= constrain.as<float>();
						rst.rotate_rad(wrap.base.trans.rot);
						return rst;
					default : return rst * constrain.as<float>();
					}
				}

				math::vec2 rst{};
				switch(frame){
				case reference_frame::global :
					rst = (dst - initial_pos) * constrain.as<float>() * get_scale();
					if(mov && has_snap)rst.snap_to({*mov, *mov});
					break;
				case reference_frame::local :
					rst = (dst - initial_pos).rotate_rad(-wrap.base.trans.rot);
					rst *= constrain.as<float>() * get_scale();
					if(mov && has_snap)rst.snap_to({*mov, *mov});
					rst.rotate_rad(wrap.base.trans.rot);
					break;
				default : break;
				}

				return rst;
			}

		public:
			void apply_eccentric_move(box_wrapper& wrap, const math::vec2 dst) const{
				auto mov = get_move(wrap, dst);
				wrap.edit.temp.box.offset = wrap.edit.base.box.offset + mov.rotate_rad(-wrap.base.trans.rot);
			}

			void apply_eccentric_center_move(box_wrapper& wrap, const math::vec2 dst) const{
				auto mov = get_move(wrap, dst);
				wrap.edit.temp.trans.vec = wrap.edit.base.trans.vec + mov;
				wrap.edit.temp.box.offset = wrap.edit.base.box.offset - mov.rotate_rad(-wrap.base.trans.rot);
			}

			void apply_move(box_wrapper& wrap, const math::vec2 dst) const noexcept{
				auto mov = get_move(wrap, dst);
				wrap.edit.temp.trans.vec = wrap.edit.base.trans.vec + mov; //.rotate_rad(wrap.base.trans.rot);
			}

			void apply_scale(box_wrapper& wrap,
				const math::vec2 dst,
				const math::vec2 avg) const noexcept{
				wrap.edit.temp.scale = wrap.edit.base.scale * get_scale(dst, avg);
			}

			void apply_rotate(box_wrapper& wrap,
				const math::vec2 dst,
				const math::vec2 avg) const noexcept{
				float ang = get_rotation(dst, avg);

				wrap.edit.temp.trans.rot = wrap.edit.base.trans.rot + ang;
			}
		};

		op operation{};

		std::vector<box_wrapper> comps{};
		history_stack<std::vector<history_entry>> history{12};

		box_wrapper* main_selected{};
		std::unordered_set<box_wrapper*> selected{};
		quad_tree<box_wrapper*> quad_tree{math::frect{math::vec2{}, (editor_radius + editor_margin) * 2}};
		bool quad_tree_changed{};

		[[nodiscard]] hitbox_edit_channel() = default;

		void update(math::vec2 cur_pos){
			if(std::exchange(quad_tree_changed, false)){
				quad_tree->reserved_clear();
				for(auto& comp : comps){
					quad_tree->insert(&comp);
				}
			}

			apply_op_to_temp(cur_pos);
			update_boxes();
		}

		void draw(ui::draw_acquirer& acquirer) const noexcept{
			using namespace graphic;

			acquirer.proj.set_layer(ui::draw_layers::base);
			for(const auto& comp : comps){
				draw::fill::quad(acquirer.get(), comp.base.crop().view_as_quad(), colors::gray.copy().set_a(.5));
			}

			for(const auto& comp : comps){
				draw::line::quad_expanded(acquirer, comp.edit.temp.crop().view_as_quad(), -editor_line_width, colors::light_gray);
			}
			acquirer.proj.set_layer(ui::draw_layers::def);

			for(auto comp : selected){
				auto box = comp->edit.temp.crop();
				draw::line::quad(acquirer, box, editor_line_width, colors::aqua);
				draw::line::square(acquirer, {comp->edit.temp.get_src_pos(), math::pi / 4}, 8, editor_line_width, colors::ORANGE);
				draw::line::line_angle(acquirer.get(), math::trans2{16} | comp->edit.temp.trans, 64, editor_line_width, colors::ORANGE,
				                       colors::ORANGE);
				draw::line::line(acquirer.get(), box.v00(), comp->edit.temp.get_src_pos(), editor_line_width, colors::aqua,
				                 colors::ORANGE);

				if(operation && operation.has_clamp() && operation.operation != edit_op::rotate){
					float angle = operation.frame == reference_frame::local ? comp->edit->trans.rot : 0;
					if(operation.constrain.x){
						draw::line::line_angle_center(acquirer.get(), {comp->edit.temp.trans.vec, angle}, 10000, editor_line_width,
						                              colors::RED, colors::RED);
					}
					if(operation.constrain.y){
						draw::line::line_angle_center(acquirer.get(), {comp->edit.temp.trans.vec, angle + math::pi / 2},
						                              10000, editor_line_width, colors::GREEN, colors::GREEN);
					}
				}
			}

			if(main_selected){
				draw::line::quad(acquirer, main_selected->edit.temp.crop(), editor_line_width, ui::theme::colors::accent);
			}

		}

		void add_comp(const meta::hitbox& meta, const bool add_selected = false){
			add_comp(meta.components | std::views::transform([](const meta::hitbox::comp& a){
				return box_wrapper{{a}};
			}), false);
		}

		template <std::ranges::input_range Rng>
			requires (std::convertible_to<std::ranges::range_value_t<Rng>, box_wrapper>)
		void add_comp_impl(Rng&& rng, const bool add_selected = false){
			if(std::ranges::empty(rng)) return;

			std::vector<std::size_t> selected_comps{};
			for(auto hitbox_comp_wrap : selected){
				auto itr = std::ranges::find(comps, hitbox_comp_wrap, [](const struct box_wrapper& b){
					return std::addressof(b);
				});
				selected_comps.push_back(std::distance(comps.begin(), itr));
			}

			std::optional<std::size_t> mainSelect{};
			if(auto itr = std::ranges::find(comps, main_selected, [](const struct box_wrapper& b){
				return std::addressof(b);
			}); itr != comps.end()){
				mainSelect = std::distance(comps.begin(), itr);
			}

			//Make sure no dangling
			clear_selected();
			quad_tree->reserved_clear();

			auto cur_sz = comps.size();
			comps.append_range(std::forward<Rng>(rng));

			for(auto selected_comp : selected_comps){
				selected.insert(&comps[selected_comp]);
			}

			if(mainSelect){
				main_selected = &comps[mainSelect.value()];
			}

			if(add_selected){
				for(auto i = cur_sz; i < comps.size(); ++i){
					selected.insert(&comps[i]);
				}
			}

			quad_tree_changed = true;
		}

		template <std::ranges::input_range Rng>
			requires (std::convertible_to<std::ranges::range_value_t<Rng>, box_wrapper>)
		void add_comp(Rng&& rng, const bool add_selected = false){
			this->add_comp_impl(std::move(rng), add_selected);
			push_history();
		}

		void add_comp(const editor_box_meta& meta, const bool add_selected = false){
			add_comp(std::views::single(box_wrapper{{meta}}), add_selected);
		}

		void add_comp(const math::rect_box_identity<float> box, const math::trans2 trans2 = {}, const bool add_selected = false){
			add_comp({box, trans2}, add_selected);
		}

		void clear(){
			// push_history();

			comps.clear();
			clear_selected();
			quad_tree_changed = true;

			push_history();
		}

		void erase_selected(){
			// push_history();

			std::erase_if(comps, [this](const struct box_wrapper& b){
				return selected.contains(const_cast<struct box_wrapper*>(&b));
			});
			clear_selected();
			quad_tree_changed = true;

			push_history();
		}

		meta::hitbox export_to_meta() const{
			meta::hitbox rst{};
			rst.components = {
					std::from_range, comps | std::views::transform([](const box_wrapper& wrap){
						return static_cast<editor_box_meta>(wrap.base);
					})
				};
			return rst;
		}

		void on_click(ui::events::click click_event, ui::util::box_selection<>& box_select, math::vec2 transed_pos){
			if(click_event.code.action() == core::ctrl::act::press){
				box_select.begin_select(transed_pos);
			} else if(click_event.code.action() == core::ctrl::act::release){
				auto region = box_select.end_select(transed_pos);
				if(operation){
					apply_op(transed_pos);
				} else{
					select_mode select_mode_{};
					switch(click_event.code.mode()){
					case core::ctrl::mode::shift : select_mode_ =
							select_mode::join;
						break;
					default : select_mode_ = select_mode::single;
						break;
					}
					std::vector<box_wrapper*> hit{};
					quad_tree->intersect_then(
						region,
						[](const ui::rect region, const ui::rect w){
							return region.overlap_exclusive(w);
						},
						[&, this](const ui::rect r, box_wrapper* w){
							auto b = w->base.crop();
							if(!b.overlap_rough(r) || !b.overlap_exact(r)) return;

							hit.push_back(w);
						});
					if(hit.empty()){
						clear_selected();
					} else{
						switch(select_mode_){
						case select_mode::single :{
							if(hit.size() == 1 && region.area() < 10){
								if(main_selected == hit.back()){
									clear_selected();
								} else{
									clear_selected();
									try_add_select(*hit.back());
								}
							} else{
								selected = {std::from_range, std::move(hit)};
								if(!selected.contains(main_selected)){
									main_selected = nullptr;
								}
							}
							break;
						}
						case select_mode::join :{
							for(auto&& hitbox_comp_wrap : hit){
								selected.insert(hitbox_comp_wrap);
							}
							if(hit.size() == 1){
								main_selected = hit.back();
							}
						}
						default : break;
						}
					}
				}
			}
		}

		ui::esc_flag on_esc(){
			if(operation){
				if(!operation.cmd.empty()){
					operation.cmd.clear();
				} else if(operation.has_clamp()){
					operation.constrain.set(true);
				} else{
					reset_op();
				}

				return ui::esc_flag::intercept;
			}

			if(!selected.empty() || main_selected){
				clear_selected();
				return ui::esc_flag::intercept;
			}

			return ui::esc_flag::fall_through;
		}


		std::string_view get_current_op_cmd() const noexcept{
			return operation ? operation.cmd : std::string_view{};
		}


		void input_key(
			const core::ctrl::key_code_t key,
			const core::ctrl::key_code_t action,
			const core::ctrl::key_code_t mode,

			const math::vec2 cur_pos

			){
			using namespace core::ctrl;

			switch(key){
			case key::Z :{
				if(action == act::press && mode == mode::ctrl){
					undo();
					return;
				}else if(action == act::press && mode == mode::ctrl_shift){
					redo();
					return;
				}
				break;
			}
			case key::D :{
				if(action == act::press && mode == mode::shift){
					copy_selected();
					return;
				}
				break;
			}
			case key::Backspace :{
				if(operation && action
					==
					act::press
				){
					if(!operation.cmd.empty()) operation.cmd.pop_back();
					return;
				}
				break;
			}
			case key::Delete :{
				if(!selected.empty()){
					erase_selected();
					return;
				}
				break;
			}
			case key::A :{
				if(action == act::press){
					switch(mode){
					case mode::shift :{
						clear_selected();
						add_comp({{-100, -100, 200, 200}}, true);
						return;
					}
					case mode::ctrl :{
						selected = {std::from_range, comps | std::views::transform([](auto& v){return std::addressof(v);})};
						return;
					}
					default : break;
					}
				}
			}
			case key::M :{
				if(mode == mode::ctrl && action == act::press){
					generate_mirrow({});
					return;
				}
			}
				default: break;
			}

			if(action == act::release){
				switch(key){
				case key::G :{
					if(mode & mode::alt){
						std::ranges::for_each(selected, &box_wrapper::reset_pos);
						push_history();
						break;
					}
					if(operation) break;
					switch(mode){
					case mode::ctrl_shift :{
						operation.start({cur_pos, edit_op::move_center_eccentrically});
						break;
					}
					case mode::ctrl :{
						operation.start({cur_pos, edit_op::move_eccentrically});
						break;
					}
					default : operation.start({cur_pos, edit_op::move});
					}

					break;
				}
				case key::S :{
					if(operation) break;
					operation.start({cur_pos, edit_op::scale});
					break;
				}
				case key::R :{
					if(mode & mode::alt){
						std::ranges::for_each(selected, &box_wrapper::reset_rot);
						push_history();
						break;
					}
					if(operation) break;
					operation.start({cur_pos, edit_op::rotate});
					break;
				}
				case key::Enter :{
					apply_op(cur_pos);
					break;
				}

				default : break;
				}
			}

			if(operation){
				switch(key){
				case key::X : if(action == act::release){
						if(!operation.set_constrain({true, false})){
							operation.switch_frame();
						}
					}
					break;
				case key::Y : if(action == act::release){
						if(!operation.set_constrain({false, true})){
							operation.switch_frame();
						}
					}
					break;
				case key::F : if(action == act::release){
						operation.switch_frame();
					}
					break;
				case key::Right_Shift :
				case key::Left_Shift :{
					switch(action){
					case act::press : operation.precision_mode = true;
						break;
					case act::release : operation.precision_mode = false;
						break;
					default : break;
					}

					save_edit_mid_data(cur_pos);
					break;
				}
				default : break;
				}
			}
		}

		void input_unicode(const char32_t val){
			if(!operation) return;
			auto v = static_cast<char>(val);
			if(v != val) return;
			if(!cmd_filter[v]) return;
			operation.cmd.push_back(std::toupper(static_cast<char>(val)));
		}

		void push_history(){
			std::vector<history_entry> history{};
			history.reserve(comps.size());

			for (const auto & comp : comps){
				history.push_back({
					static_cast<editor_box_meta>(comp.base),
					selected.contains(const_cast<box_wrapper*>(&comp)),
					main_selected == &comp
				});
			}

			this->history.push(std::move(history));
		}

		void apply_history(const decltype(history)::value_type& entry){
			comps.clear();
			clear_selected();

			add_comp_impl(entry | std::views::transform([](const history_entry& v) -> box_wrapper{
				return box_wrapper{{static_cast<editor_box_meta>(v)}};
			}), false);

			for (const auto & [e, c] : std::views::zip(entry, comps)){
				if(e.selected)add_select(c);
				if(e.main_selected)main_selected = &c;
			}
		}

		void undo(){
			history.to_prev();
			if(const auto cur = history.try_get()){
				apply_history(*cur);
			}
		}

		void redo(){
			history.to_next();

			if(const auto cur = history.try_get()){
				apply_history(*cur);
			}
		}

	private:
		void copy_selected(){
			std::vector<box_wrapper> metas{};
			metas.reserve(selected.size());

			for(auto wrap : selected){
				auto& rst = metas.emplace_back(*wrap);
				rst.base.trans.vec += math::vec2{200, 0};
				rst.edit = rst.base;
			}

			clear_selected();
			add_comp(std::move(metas), true);
		}

		void generate_mirrow(const math::trans2 axis){
			std::vector<box_wrapper> metas{};
			metas.reserve(selected.size());

			for(auto hitbox_comp_wrap : selected){
				auto axis_local = axis.apply_inv_to(hitbox_comp_wrap->base.trans);

				auto src_offset_dst = axis.apply_inv_to(
					hitbox_comp_wrap->base.box.offset | hitbox_comp_wrap->base.trans);

				auto mirrowed_box = hitbox_comp_wrap->edit->box;

				axis_local.vec.flip_y();
				axis_local.rot = -axis_local.rot;
				src_offset_dst.flip_y();

				mirrowed_box.offset = axis_local.apply_inv_to(src_offset_dst);
				mirrowed_box.size.flip_y();

				metas.push_back({{mirrowed_box, axis_local | axis}});
			}

			add_comp(std::move(metas), true);
		}

		void save_edit_mid_data(math::vec2 cur_pos){
			apply_op_to_temp_base();
			operation.initial_pos = cur_pos;
		}

		void reset_op(){
			operation.reset();
			for(auto hitbox_comp_wrap : selected){
				hitbox_comp_wrap->edit = hitbox_comp_wrap->base;
			}
		}

		void apply_op(math::vec2 cur_pos){
			apply_op_to_temp(cur_pos);
			apply_op_to_temp_base();
			apply_op_to_base();
			operation.reset();
		}

		void clear_selected() noexcept{
			selected.clear();
			main_selected = nullptr;
		}

		void try_add_select(box_wrapper& w){
			if(main_selected == &w){
				main_selected = nullptr;
				selected.erase(&w);
			} else{
				main_selected = &w;
				selected.insert(&w);
			}
		}

		void add_select(box_wrapper& w){
			selected.insert(&w);
		}

		void remove_select(box_wrapper& w){
			if(main_selected == &w){
				main_selected = nullptr;
			}
			selected.erase(&w);
		}

		void apply_op_to_temp(math::vec2 cur_pos){
			if(!main_selected && selected.empty()) return;


			// const auto main_pos = main_selected->edit.temp.trans.vec;
			const auto avg_pos = selected.empty()
				                     ? math::vec2{}
				                     : (std::ranges::fold_left(selected | std::views::transform(
					                                               [](const box_wrapper* w){
						                                               return w->base.trans.vec;
					                                               }), math::vec2{}, std::plus<>{}) / selected.size());

			switch(operation.operation){
			case edit_op::move :{
				for(auto wrap : selected){
					operation.apply_move(*wrap, cur_pos);
				}
				break;
			}
			case edit_op::move_eccentrically :{
				for(auto wrap : selected){
					operation.apply_eccentric_move(*wrap, cur_pos);
				}
				break;
			}
			case edit_op::move_center_eccentrically :{
				for(auto wrap : selected){
					operation.apply_eccentric_center_move(*wrap, cur_pos);
				}
				break;
			}
			case edit_op::scale :{
				for(auto wrap : selected){
					operation.apply_scale(*wrap, cur_pos, avg_pos);
				}
				break;
			}
			case edit_op::rotate :{
				for(auto wrap : selected){
					operation.apply_rotate(*wrap, cur_pos, avg_pos);
				}
				break;
			}
			default : break;
			}
		}

		void apply_op_to_temp_base(){
			for(auto wrap : selected){
				wrap->edit.apply();
			}
		}

		void apply_op_to_base(){
			for(auto wrap : selected){
				wrap->apply_scaled();
			}
			push_history();
			quad_tree_changed = true;
		}

		void update_boxes() noexcept{
			if(operation && !selected.empty()){
				quad_tree_changed = true;
			}
		}
	};

	namespace ui{
		export using namespace mo_yanxi::ui;

		export
		struct hit_box_editor : table{
		private:
			struct editor_viewport : elem{
				math::vec2 last_camera_pos{};
				graphic::camera2 camera{};
				ui::util::box_selection<> box_select{};

				hitbox_edit_channel channel_hitbox{};
				reference_image_channel channel_reference_image{};


				[[nodiscard]] editor_viewport(scene* scene, group* group)
					: elem(scene, group, "hitbox_viewport"){
					set_style(ui::theme::styles::general_static);

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

					register_event([](const ui::events::scroll e, editor_viewport& self){
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
					camera.set_scale_range({0.125f, 2});
					camera.flip_y = true;

					channel_hitbox.add_comp({{0, 0, 200, 200}});
				}

				void update(const float delta_in_ticks) override{
					elem::update(delta_in_ticks);
					camera.clamp_position({math::vec2{}, editor_radius * 2});
					camera.update(delta_in_ticks);

					channel_hitbox.update(getTransferredPos(get_scene()->get_cursor_pos()));
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

					channel_reference_image.draw(acquirer);
					channel_hitbox.draw(acquirer);


					if(box_select){
						auto p = getTransferredPos(get_scene()->get_cursor_pos());
						draw::fill::rect_ortho(acquirer.get(), box_select.get_region(p), colors::aqua.copy().set_a(.15f));
						draw::line::rect_ortho(acquirer, box_select.get_region(p), 2, colors::aqua);
					}

					get_renderer().batch.pop_scissor();
					get_renderer().batch.pop_viewport();
					get_renderer().batch.pop_projection();
				}

				ui::events::click_result on_click(const ui::events::click click_event) override{
					if(click_event.code.key() == core::ctrl::mouse::CMB){
						last_camera_pos = camera.get_stable_center();
					}

					if(click_event.code.key() == core::ctrl::mouse::LMB){
						channel_hitbox.on_click(click_event, box_select, getTransferredPos(get_scene()->get_cursor_pos()));
					}
					return ui::events::click_result::intercepted;
				}

				esc_flag on_esc() override{
					if(esc_flag::intercept == channel_hitbox.on_esc()){
						return esc_flag::fall_through;
					}

					return esc_flag::intercept;
				}


				void input_key(const core::ctrl::key_code_t key, const core::ctrl::key_code_t action,
				               const core::ctrl::key_code_t mode) override{
					using namespace core::ctrl;

					channel_hitbox.input_key(key, action, mode, getTransferredPos(get_scene()->get_cursor_pos()));
				}

				void input_unicode(const char32_t val) override{
					channel_hitbox.input_unicode(val);
				}

			private:
				[[nodiscard]] math::vec2 getTransferredPos(const math::vec2 pos) const{
					return camera.get_screen_to_world(pos, content_src_pos(), true);
				}
			};

			table* menu{};
			editor_viewport* viewport{};

			label* cmd_text{};
			check_box* checkbox{};

			void build_menu(){
				menu->clear_children();
				menu->template_cell.set_external({false, true}).set_pad({.bottom = 8});

				auto box = menu->end_line().emplace<ui::check_box>();
				box.cell().set_height(60);
				box->set_style(ui::theme::styles::no_edge);
				box->set_drawable<ui::icon_drawable>(0, ui::theme::icons::blender_icon_pivot_individual);
				box->set_drawable<ui::icon_drawable>(1, ui::theme::icons::blender_icon_pivot_median);
				box->set_drawable<ui::icon_drawable>(2, ui::theme::icons::blender_icon_pivot_active);
				box->add_multi_select_tooltip({
						.follow = tooltip_follow::owner,
						.align_owner = align::pos::top_right,
						.align_tooltip = align::pos::top_left,
					});

				checkbox = std::to_address(box);


				{
					auto b = menu->end_line().emplace<ui::button<ui::label>>();

					b->set_style(ui::theme::styles::no_edge);
					b->set_scale(.6f);
					b->set_text("save");
					b->set_button_callback(ui::button_tags::general, [this]{
						auto& selector = creation::create_file_selector(creation::file_selector_create_info{
							.requester = *this,
							.checker = [](const creation::file_selector& s, const ui::hit_box_editor&){
								return s.get_current_main_select().has_value();
							},
							.yielder = [](const creation::file_selector& s, ui::hit_box_editor& self){
								self.write_to(s.get_current_main_select().value());
								return true;
							},
							.add_file_create_button = true
						});
						selector.set_cared_suffix({".hbox"});
					});
				}


				{
					auto b = menu->end_line().emplace<ui::button<ui::label>>();

					b->set_style(ui::theme::styles::no_edge);
					b->set_scale(.6f);
					b->set_text("load");
					b->set_button_callback(ui::button_tags::general, [this]{
						auto& selector = creation::create_file_selector(creation::file_selector_create_info{
							*this, [](const creation::file_selector& s, const ui::hit_box_editor&){
								return s.get_current_main_select().has_value();
							}, [](const creation::file_selector& s, ui::hit_box_editor& self){
								self.load_from(s.get_current_main_select().value());
								return true;
							}});
						selector.set_cared_suffix({".hbox"});
					});
				}

				{
					auto b = menu->end_line().emplace<ui::button<ui::label>>();

					b->set_style(ui::theme::styles::no_edge);
					b->set_scale(.6f);
					b->set_text("set ref");
					b->set_button_callback(ui::button_tags::general, [this]{
						auto& selector = creation::create_file_selector(creation::file_selector_create_info{
							*this, [](const creation::file_selector& s, const ui::hit_box_editor&){
								return s.get_current_main_select().has_value();
							}, [](const creation::file_selector& s, ui::hit_box_editor& self){
								self.set_image_ref(s.get_current_main_select().value());
								return true;
							}});
						selector.set_cared_suffix({".png"});
					});
				}
			}

		public:
			[[nodiscard]] hit_box_editor(scene* scene, group* group)
				: table(scene, group){
				auto m = emplace<table>();
				m.cell().set_width(200);
				m.cell().pad.right = 8;

				menu = std::to_address(m);

				auto editor_region = emplace<manual_table>();
				editor_region->set_style();

				auto vp = editor_region->emplace<editor_viewport>();
				vp.cell().region_scale = {tags::from_extent, math::vec2{}, math::vec2{1.f, 1.f}};
				vp->set_style(ui::theme::styles::general_static);
				// vp->prop().fill_parent.set(true);
				viewport = std::to_address(vp);

				auto hint = editor_region->emplace<label>();
				hint.cell().region_scale = {tags::from_extent, math::vec2{}, math::vec2{1.f, 0.2f}};
				hint.cell().align = align::pos::top_left;
				hint.cell().margin.set(20);
				hint->set_style();
				hint->set_policy(font::typesetting::layout_policy::auto_feed_line);
				hint->set_scale(.5f);
				hint->interactivity = interactivity::disabled;
				cmd_text = std::to_address(hint);

				build_menu();
			}

			void update(float delta_in_ticks) override{
				table::update(delta_in_ticks);

				cmd_text->set_text(viewport->channel_hitbox.get_current_op_cmd());
				viewport->channel_hitbox.operation.center = static_cast<hitbox_edit_channel::reference_center>(checkbox->
					get_frame_index());
			}

			hit_box_editor(const hit_box_editor& other) = delete;
			hit_box_editor(hit_box_editor&& other) noexcept = delete;
			hit_box_editor& operator=(const hit_box_editor& other) = delete;
			hit_box_editor& operator=(hit_box_editor&& other) noexcept = delete;

		private:
			void set_image_ref(const std::filesystem::path& path);

			void write_to(const std::filesystem::path& path);
			void load_from(const std::filesystem::path& path);
		};

	}
}
