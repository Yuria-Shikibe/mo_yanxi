// module;

export module mo_yanxi.game.ui.hitbox_editor;

import mo_yanxi.ui.primitives;
import mo_yanxi.ui.elem.table;
import mo_yanxi.ui.elem.button;
import mo_yanxi.ui.elem.manual_table;
import mo_yanxi.ui.elem.label;
import mo_yanxi.ui.creation.file_selector;

export import mo_yanxi.game.meta.hitbox;
import mo_yanxi.game.quad_tree;
import mo_yanxi.game.quad_tree_interface;

import mo_yanxi.graphic.camera;
import mo_yanxi.graphic.image_manage;

import mo_yanxi.game.ui.viewport;
import mo_yanxi.ui.elem.check_box;
import mo_yanxi.ui.graphic;
import mo_yanxi.ui.assets;
import mo_yanxi.ui.elem.text_input_area;
import mo_yanxi.ui.selection;
import mo_yanxi.graphic.layers.ui.grid_drawer;

import mo_yanxi.char_filter;
import mo_yanxi.snap_shot;
import mo_yanxi.history_stack;

import std;
using editor_box_meta = mo_yanxi::game::meta::hitbox::comp;

namespace mo_yanxi::game{

	struct editor_scalable_box_meta : editor_box_meta{
		math::vec2 scale{1, 1};

		[[nodiscard]] math::frect_box crop() const noexcept{
			math::rect_box<float> box;
			box.update(trans, this->box * scale);
			return box;
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
	// constexpr float editor_radius = 5000;
	constexpr float editor_margin = 2000;
	constexpr char_filter cmd_filter{".1234567890+-Ss"};

	struct basic_op{
		math::vec2 initial_pos{};
		edit_op operation{};

		math::bool2 constrain{true, true};

		bool precision_mode{};
		std::string string_cmd{};


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
			return string_cmd.contains('S');
		}

		std::optional<float> get_num_from_cmd() const{
			float val{};
			auto ptr = string_cmd.c_str();
			auto end = string_cmd.data() + string_cmd.size();

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
		[[nodiscard]] float get_rotation(
				const math::vec2 dst,
				const math::vec2 center) const noexcept{
			float ang;

			auto cmdv = get_num_from_cmd().transform([](const float f){ return math::deg_to_rad * -f; });
			bool snap = has_snap();

			if(!snap && cmdv){
				ang = cmdv.value();
			} else{
				const auto approach_src = initial_pos - center;
				const auto approach_dst = dst - center;

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

	private:
		box_wrapper wrapper{};

	public:

		float scale{1.f};
		edit_op op{};

		void set_region(graphic::allocated_image_region&& region) noexcept{
			image_region = std::move(region);
			wrapper.base = {editor_box_meta{
				.box = {region.uv.size.as<float>().scl(-.5f), region.uv.size.as<float>()},
				.trans = {}
			}};
			wrapper.base.scale.set(scale);
		}

		void update_scale(float scl) noexcept{
			scale = scl;
			wrapper.base.scale.set(scale);
		}

		void draw(mo_yanxi::ui::draw_acquirer& acquirer) const{
			if(image_region){
				auto last = acquirer.get_region();
				auto mode = acquirer.proj;

				auto region = static_cast<const mo_yanxi::ui::draw_acquirer::region_type&>(image_region);
				region.uv.flip_y();
				acquirer << region;

				acquirer.proj.set_layer(mo_yanxi::ui::draw_layers::base);
				graphic::draw::fill::quad(acquirer.get(), wrapper.base.crop().view_as_quad());

				acquirer << last;
				acquirer.proj = mode;
				graphic::draw::line::quad(acquirer, wrapper.base.crop().view_as_quad(), editor_line_width, graphic::colors::gray.copy().set_a(.5f));
			}
		}
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
		reference_center frame_center{};

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
			const math::vec2 ref_center) const noexcept{
			auto scl = get_scale(dst, ref_center);
			wrap.edit.temp.scale = wrap.edit.base.scale * scl;

			switch(frame_center){
				case reference_center::local : break;
				case reference_center::average : [[fallthrough]];
				case reference_center::selected :
					wrap.edit.temp.trans.vec = (wrap.edit.base.trans.vec - ref_center) * scl + ref_center;
				default: break;
			}
		}

		void apply_rotate(box_wrapper& wrap,
			const math::vec2 dst,
			const math::vec2 avg) const noexcept{
			float ang = get_rotation(dst, avg);

			wrap.edit.temp.trans.rot = wrap.edit.base.trans.rot + ang;
		}
};

	struct history_entry : editor_box_meta{
		bool selected{};
		bool main_selected{};
	};

	struct hitbox_edit_channel{

		op operation{};

		math::trans2 origin_trans{};

		std::vector<box_wrapper> comps{};
		history_stack<std::vector<history_entry>> history{12};

		box_wrapper* main_selected{};
		std::unordered_set<box_wrapper*> selected{};
		quad_tree<box_wrapper*> quad_tree{math::frect{math::vec2{}, (ui::viewport_default_radius + editor_margin) * 2}};
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

			draw::line::square(acquirer, origin_trans, 32, editor_line_width * .65f, ui::theme::colors::accent);
			draw::line::line_angle(acquirer.get(), math::trans2{32 - editor_line_width * .65f} | origin_trans, 512, editor_line_width, ui::theme::colors::accent);


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

		meta::hitbox_transed export_to_meta() const{
			meta::hitbox_transed rst{};
			rst.components = {
					std::from_range, comps | std::views::transform([](const box_wrapper& wrap){
						return static_cast<editor_box_meta>(wrap.base);
					})
				};
			rst.trans = origin_trans;
			return rst;
		}

		void on_click(ui::input_event::click click_event, ui::util::box_selection<>& box_select, math::vec2 transed_pos){
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
							if(region.is_point()){
								if(!b.overlap(r)) return;
							}else{
								if(!r.contains(b.get_bound())) return;
							}

							hit.push_back(w);
						});
					if(hit.empty()){
						clear_selected();
					} else{
						switch(select_mode_){
						case select_mode::single :{
							if(hit.size() == 1 && region.is_point()){
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
				if(!operation.string_cmd.empty()){
					operation.string_cmd.clear();
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
			return operation ? operation.string_cmd : std::string_view{};
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
					if(!operation.string_cmd.empty()) operation.string_cmd.pop_back();
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
			operation.string_cmd.push_back(std::toupper(static_cast<char>(val)));
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

			auto sz = std::min(comps.size(), entry.size());
			for(auto i = 0uz; i < sz; ++i){
				auto& e = entry[i];
				auto& c = comps[i];

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
			const auto avg_pos =
				selected.empty()
					? math::vec2{}
					: operation.frame_center == reference_center::selected
					? main_selected
						  ? main_selected->base.trans.vec
						  : math::vec2{}
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
		struct hitbox_editor : table{
		private:
			struct editor_viewport : viewport<elem>{
				ui::util::box_selection<> box_select{};

				hitbox_edit_channel channel_hitbox{};
				reference_image_channel channel_reference_image{};


				[[nodiscard]] editor_viewport(scene* scene, group* group)
					: viewport(scene, group){
					set_style(ui::theme::styles::general_static);

					camera.set_scale_range({0.125f, 2});

					channel_hitbox.add_comp({{0, 0, 200, 200}});
				}

				void update(const float delta_in_ticks) override{
					viewport::update(delta_in_ticks);

					channel_hitbox.update(get_transferred_pos(get_scene()->get_cursor_pos()));
				}


				void draw_content(const rect clipSpace) const override{
					viewport_begin();

					using namespace graphic;
					auto& r = renderer_from_erased(get_renderer());
					draw_acquirer acquirer{ui::get_draw_acquirer(r)};


					{
						batch_layer_guard guard(r.batch, std::in_place_type<layers::grid_drawer>);

						draw::fill::rect_ortho(acquirer.get(), camera.get_viewport(), colors::black.copy().set_a(.35));
					}

					channel_reference_image.draw(acquirer);
					channel_hitbox.draw(acquirer);


					if(box_select){
						auto p = get_transferred_pos(get_scene()->get_cursor_pos());
						draw::fill::rect_ortho(acquirer.get(), box_select.get_region(p), colors::aqua.copy().set_a(.15f));
						draw::line::rect_ortho(acquirer, box_select.get_region(p), 2, colors::aqua);
					}

					viewport_end();
				}

				ui::input_event::click_result on_click(const ui::input_event::click click_event) override{
					if(click_event.code.key() == core::ctrl::mouse::LMB){
						channel_hitbox.on_click(click_event, box_select, get_transferred_pos(get_scene()->get_cursor_pos()));
					}

					return viewport::on_click(click_event);
				}

				esc_flag on_esc() override{
					if(esc_flag::fall_through == channel_hitbox.on_esc()){
						return esc_flag::fall_through;
					}

					return esc_flag::intercept;
				}


				void on_key_input(const core::ctrl::key_code_t key, const core::ctrl::key_code_t action,
							   const core::ctrl::key_code_t mode) override{
					using namespace core::ctrl;

					channel_hitbox.input_key(key, action, mode, get_transferred_pos(get_scene()->get_cursor_pos()));
				}

				void on_unicode_input(const char32_t val) override{
					channel_hitbox.input_unicode(val);
				}

			};

			table* menu{};
			editor_viewport* viewport{};

			label* cmd_text{};
			button<icon_frame>* origin_point_modify{};
			check_box* checkbox{};

			void build_menu();

		public:
			[[nodiscard]] hitbox_editor(scene* scene, group* group)
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
				viewport->channel_hitbox.operation.frame_center = static_cast<reference_center>(checkbox->
					get_frame_index());
			}

			hitbox_editor(const hitbox_editor& other) = delete;
			hitbox_editor(hitbox_editor&& other) noexcept = delete;
			hitbox_editor& operator=(const hitbox_editor& other) = delete;
			hitbox_editor& operator=(hitbox_editor&& other) noexcept = delete;

		private:
			void set_image_ref(const std::filesystem::path& path);

			void write_to(const std::filesystem::path& path);
			void load_from(const std::filesystem::path& path);
		};
	}
}
