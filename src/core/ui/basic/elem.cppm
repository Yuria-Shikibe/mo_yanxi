module;

#include <cassert>

export module mo_yanxi.ui.primitives:elem;

export import mo_yanxi.math;
export import mo_yanxi.math.vector2;
export import mo_yanxi.math.rect_ortho;
export import mo_yanxi.event;

export import align;

export import mo_yanxi.graphic.color;

import :pre_decl;
import :elem_ptr;
import :tooltip_interface;

export import mo_yanxi.ui.flags;
export import mo_yanxi.ui.comp.drawer;
export import mo_yanxi.ui.event_types;
export import mo_yanxi.ui.clamped_size;
export import mo_yanxi.ui.util;
export import mo_yanxi.ui.layout.policies;
export import mo_yanxi.ui.action;
export import mo_yanxi.core.ctrl.constants;

//TODO isolate this in future
export import mo_yanxi.graphic.renderer.predecl;


import mo_yanxi.spreadable_event_handler;
import mo_yanxi.math.quad_tree.interface;


import mo_yanxi.meta_programming;
import mo_yanxi.handle_wrapper;
import mo_yanxi.owner;
import mo_yanxi.func_initialzer;

import std;


namespace mo_yanxi::ui{
	// export constexpr inline float preferred_size =
	export constexpr inline bool IgnoreClipWhenDraw = false;

	export using rect = math::frect;

	//TODO boarder requirement ?
	export struct debug_elem_drawer final : style_drawer<elem>{
		void draw(const elem& element, rect region, float opacityScl) const override;
	};

	export struct empty_drawer final : style_drawer<elem>{
		void draw(const elem& element, rect region, float opacityScl) const override{
		}
	};

	export constexpr inline align::spacing DefaultBoarder{BoarderStroke, BoarderStroke, BoarderStroke, BoarderStroke};

	export constexpr inline debug_elem_drawer DefaultStyleDrawer;
	export constexpr inline empty_drawer EmptyStyleDrawer;

	export inline const style_drawer<elem>* global_style_drawer;

	namespace input_event{

		export
		enum struct click_result{
			intercepted,
			spread,
		};

		// export
		// using ElementConnection = ConnectionEvent<elem>;
		// export
		// using GeneralCodeEvent = CodeEvent<>;
	}

	export using elem_event_manager =
	mo_yanxi::events::event_manager<
		std::move_only_function<void(elem&) const>,
		input_event::cursor_moved>;

	const style_drawer<elem>* getDefaultStyleDrawer(){
		return global_style_drawer ? global_style_drawer : &DefaultStyleDrawer;
	}

	export struct elem_graphic_data{
		const style_drawer<elem>* drawer{getDefaultStyleDrawer()};

		graphic::color style_color_scl{graphic::colors::white};

		float inherent_opacity{1.f};
		float context_opacity{1.f};

		[[nodiscard]] constexpr float get_opacity() const noexcept{
			return inherent_opacity * context_opacity;
		}

	};

	export
	struct independent_draw_state{
		float depth;

		constexpr auto operator<=>(const independent_draw_state& rhs) const noexcept{
			return depth <=> rhs.depth;
		}

		constexpr bool operator==(const independent_draw_state&) const = default;
	};

	export
	template <typename T>
	struct ElemDynamicChecker{
	private:
		template <typename Fn>
		static void setFn(std::move_only_function<bool(T&) const>& tgt, Fn&& fn){
			if constexpr(std::predicate<Fn>){
				tgt = [pred = std::forward<Fn>(fn)](T&){
					return std::invoke(pred);
				};
			} else if constexpr(std::predicate<Fn, T&>){
				tgt = std::forward<Fn>(fn);
			} else{
				static_assert(false, "Unsupported predicate type");
			}
		}

	public:
		std::move_only_function<bool(T&) const> visibilityChecker{nullptr};
		std::move_only_function<bool(T&) const> disableProv{nullptr};
		std::move_only_function<bool(T&) const> activatedChecker{nullptr};

		// std::move_only_function<void(T&)> appendUpdator{nullptr};

		template <std::predicate<> Fn>
		void setDisableProv(Fn&& fn){
			ElemDynamicChecker::setFn(disableProv, std::forward<Fn>(fn));
		}

		template <std::predicate<T&> Fn>
		void setDisableProv(Fn&& fn){
			ElemDynamicChecker::setFn(disableProv, std::forward<Fn>(fn));
		}

		template <std::predicate<> Fn>
		void setVisibleProv(Fn&& fn){
			ElemDynamicChecker::setFn(visibilityChecker, std::forward<Fn>(fn));
		}

		template <std::predicate<T&> Fn>
		void setVisibleProv(Fn&& fn){
			ElemDynamicChecker::setFn(visibilityChecker, std::forward<Fn>(fn));
		}

		template <std::predicate<> Fn>
		void setActivatedProv(Fn&& fn){
			ElemDynamicChecker::setFn(activatedChecker, std::forward<Fn>(fn));
		}

		template <std::predicate<T&> Fn>
		void setActivatedProv(Fn&& fn){
			ElemDynamicChecker::setFn(activatedChecker, std::forward<Fn>(fn));
		}
	};

	export
	struct cursor_states{
		float maximum_duration{60.};
		/**
		 * @brief in tick
		 */
		float time_inbound{};
		float time_focus{};
		float time_stagnate{};

		float time_tooltip{};

		bool inbound{};
		bool focused{};
		bool pressed{};

		void quit_focus() noexcept{
			focused = pressed = false;
		}

		void update(const float delta_in_ticks){
			if(inbound){
				math::approach_inplace(time_inbound, maximum_duration, delta_in_ticks);
			} else{
				math::approach_inplace(time_inbound, 0, delta_in_ticks);
			}

			if(focused){
				math::approach_inplace(time_focus, maximum_duration, delta_in_ticks);
				math::approach_inplace(time_stagnate, maximum_duration, delta_in_ticks);
				time_tooltip += delta_in_ticks;
			} else{
				math::approach_inplace(time_focus, 0, delta_in_ticks);
				time_tooltip = time_stagnate = 0.f;
			}
		}
	};



	export struct elem_prop{
		//position fields
		math::vec2 relative_src{};
		math::vec2 absolute_src{};

		math::bool2 fill_parent{};
		bool maintain_focus_until_mouse_drop{};

		clamped_fsize size{};
		align::spacing boarder{DefaultBoarder};

		float scale_context{1.};
		float scale_local{1.};

		elem_graphic_data graphic_data{};


		void set_empty_drawer(){
			boarder = {};
			graphic_data.drawer = &EmptyStyleDrawer;
		}

		constexpr void set_abs_src(const math::vec2 parentOriginalPoint) noexcept{
			absolute_src = parentOriginalPoint + relative_src;
		}

		[[nodiscard]] constexpr math::vec2 content_src_offset() const noexcept{
			return boarder.top_lft();
		}

		[[nodiscard]] constexpr math::vec2 content_abs_src() const noexcept{
			return absolute_src + content_src_offset();
		}

		[[nodiscard]] constexpr rect bound_relative() const noexcept{
			return rect{tags::from_extent, relative_src, size.get_size()};
		}

		[[nodiscard]] constexpr rect bound_absolute() const noexcept{
			return rect{tags::from_extent, absolute_src, size.get_size()};
		}

		[[nodiscard]] constexpr rect content_bound_relative() const noexcept{
			return rect{tags::from_extent, relative_src + content_src_offset(), content_size()};
		}

		[[nodiscard]] constexpr rect content_bound_absolute() const noexcept{
			return rect{tags::from_extent, absolute_src + content_src_offset(), content_size()};
		}

		[[nodiscard]] constexpr float get_min_boarder_size() const noexcept{
			return std::ranges::max({boarder.left, boarder.right, boarder.bottom, boarder.top});
		}

		[[nodiscard]] constexpr bool
		content_bound_contains(const rect& clipRegion, const math::vec2 pos) const noexcept{
			return clipRegion.contains_loose(pos) && content_bound_absolute().contains_loose(pos);
		}

		[[nodiscard]] constexpr bool bound_contains(const rect& clipRegion, const math::vec2 pos) const noexcept{
			return clipRegion.contains_loose(pos) && bound_absolute().contains_loose(pos);
		}

		[[nodiscard]] constexpr math::vec2 evaluate_valid_size(math::vec2 rawSize) const noexcept{
			return
				rawSize.clamp_xy(
					size.get_minimum_size(),
					size.get_maximum_size()).sub(boarder.extent()
				).max({});
		}

		[[nodiscard]] constexpr float content_width() const noexcept{
			return size.get_width() - boarder.width();
		}

		[[nodiscard]] constexpr float content_height() const noexcept{
			return size.get_height() - boarder.height();
		}

		[[nodiscard]] constexpr math::vec2 content_size() const noexcept{
			return (size.get_size() - boarder.extent()).max({});
		}

		[[nodiscard]] constexpr math::vec2 get_size() const noexcept{
			return size.get_size();
		}
	};

	export struct elem_fields{
		friend scene;

		std::string name{};

		elem_prop property{};
		cursor_states cursor_state{};


		//TODO using variant<type, float> instead?
		optional_mastering_extent context_size_restriction{};
		/**
		 * @brief size restriction in a layout context, usually is gain from a cell
		 */
		// math::vec2 context_bound{math::vectors::constant2<float>::inf_positive_vec2};
		// bool expandable{false};
	protected:
		group* parent{};
		scene* scene_{};

		std::queue<std::unique_ptr<action::action<elem>>> actions{};

	public:
		//state
		//TODO using a bit flags?
		bool activated{}; //TODO as graphic property? / rename it
		bool disabled{};
		bool visible{true};
		bool sleep{};

		//TODO move this to property?
		bool skip_inbound_capture{};

	private:
		bool has_scene_direct_access{false};
	public:
		//Layout Spec
		layout_state layout_state{};
		interactivity interactivity{interactivity::enabled};

		ElemDynamicChecker<elem> checkers{};

	protected:
		[[nodiscard]] elem_fields() = default;
	};

	export struct elem :

	elem_fields,
	stated_tooltip_owner<elem>,
	spreadable_event_handler<elem,
	                         input_event::collapser_state_changed,
	                         input_event::check_box_state_changed
	>,

		// StatedToolTipOwner<elem>,
		math::quad_tree_adaptor<elem>
	{
		friend scene;

		[[nodiscard]] elem(
			scene* scene,
			group* group){
			set_scene(scene);
			set_parent(group);
		}

		virtual ~elem(){
			clear_external_references();
		}

		elem(const elem& other) = delete;

		elem& operator=(const elem& other) = delete;

		elem(elem&& other) noexcept: elem_fields{std::move(other)}{
		}

		elem& operator=(elem&& other) noexcept{
			if(this == &other) return *this;
			elem_fields::operator =(std::move(other));
			return *this;
		}

		// [[nodiscard]] Graphic::Batch_Direct& getBatch() const noexcept;
		//
		[[nodiscard]] graphic::renderer_ui_ref get_renderer() const noexcept;

		[[nodiscard]] const cursor_states& get_cursor_state() const noexcept{
			return cursor_state;
		}

		void set_style(const style_drawer<elem>& drawer);

		void set_style(){
			property.set_empty_drawer();
		}

		bool resize_masked(const math::vec2 size, spread_direction mask = spread_direction::none){
			auto last = std::exchange(layout_state.acceptMask_inherent, mask);
			auto rst = resize(size);
			layout_state.acceptMask_inherent = last;
			return rst;
		}

		[[nodiscard]] constexpr math::vec2 get_size() const noexcept{
			return property.size.get_size();
		}

		[[nodiscard]] constexpr math::vec2 content_size() const noexcept{
			return property.content_size();
		}

		[[nodiscard]] /*constexpr*/ math::vec2 content_potential_size() const noexcept{
			return content_potential_size(context_size_restriction);
		}

		[[nodiscard]] /*constexpr*/ math::vec2 content_potential_size(optional_mastering_extent extent) const noexcept{
			//TODO directly return extent.size?
			auto sz = property.content_size();

			auto [depX, depY] = extent.get_dependent();

			if(depX){
				sz.x = std::numeric_limits<float>::infinity();
			}
			if(depY){
				sz.y = std::numeric_limits<float>::infinity();
			}
			return sz;
		}

		[[nodiscard]] constexpr bool maintain_focus_by_mouse() const noexcept{
			return property.maintain_focus_until_mouse_drop;
		}

		[[nodiscard]] constexpr bool is_activated() const noexcept{ return activated; }

		[[nodiscard]] constexpr bool is_visible() const noexcept{ return visible; }

		[[nodiscard]] constexpr bool is_sleep() const noexcept{ return sleep; }

		[[nodiscard]] constexpr bool is_disabled() const noexcept{ return disabled; }

		template <std::derived_from<group> T = group, bool checked = false>
		[[nodiscard]] T* get_parent() const noexcept{
			if constexpr (checked && !std::same_as<T, group>){
				return dynamic_cast<T*>(parent);
			}else{
				return static_cast<T*>(parent);
			}

		}

		[[nodiscard]] group* get_root_parent() const noexcept;

		void update_opacity(float val) noexcept;

		void notify_isolated_layout_changed();

		void set_parent(group* p) noexcept{
			parent = p;
		}

		[[nodiscard]] bool is_root_element() const noexcept{
			return parent == nullptr;
		}

		//TODO should manager be null?
		[[nodiscard]] scene* get_scene() const noexcept{
			return scene_;
		}

		[[nodiscard]] elem_prop& prop() noexcept{
			return property;
		}

		[[nodiscard]] const elem_prop& prop() const noexcept{
			return property;
		}

		[[nodiscard]] const auto& gprop() const noexcept{
			return property.graphic_data;
		}


		template <typename T>
		bool fire_event(const T& event, spread_direction spread_direction = spread_direction::lower, bool force_spread = false){
			if(spread_direction & spread_direction::local){
				if(spreadable_event_handler::spreadable_event_handler_fire(event) && !force_spread){
					return true;
				}
			}

			if(spread_direction & spread_direction::super && !is_root_element()){
				auto p = get_parent_to_elem();
				auto rst = p->fire_event(event, (spread_direction | spread_direction::local) - spread_direction::child, force_spread);
				if(!force_spread && rst){
					return true;
				}
			}

			if(spread_direction & spread_direction::child){
				for (auto && child : get_children()){
					auto rst = child->fire_event(event, (spread_direction | spread_direction::local) - spread_direction::super, force_spread);
					if(!force_spread && rst){
						return true;
					}
				}
			}

			return false;
		}


	public:
		[[nodiscard]] constexpr math::vec2 abs_pos() const noexcept{
			return property.absolute_src;
		}

		[[nodiscard]] virtual math::vec2 transform_pos(math::vec2 input) const noexcept{
			return input;
		}

		[[nodiscard]] math::vec2 get_local_from_global(math::vec2 global_input) const noexcept;


		[[nodiscard]] constexpr math::vec2 content_src_pos() const noexcept{
			return property.absolute_src + property.content_src_offset();
		}

		//TODO should it return bool?
		void remove_self_from_parent() noexcept;
		// void remove_self_from_parent_instantly();
		// void notify_remove();

		void clear_external_references() noexcept;
		void clear_external_references_recursively() noexcept{
			clear_external_references();

			for (auto && child : get_children()){
				child->clear_external_references_recursively();
			}
		}

		[[nodiscard]] bool is_focused_scroll() const noexcept;
		[[nodiscard]] bool is_focused_key() const noexcept;

		[[nodiscard]] bool is_focused() const noexcept;
		[[nodiscard]] bool is_inbounded() const noexcept;

	protected:
		void set_focused_scroll(bool focus) noexcept;
		void set_focused_key(bool focus) noexcept;

	public:

		[[nodiscard]] constexpr bool ignore_inbound() const noexcept{
			return skip_inbound_capture;
		}

		[[nodiscard]] constexpr bool interactable() const noexcept{
			if(!is_visible()) return false;
			if(disabled) return false;
			return interactivity == interactivity::enabled || interactivity == interactivity::intercepted;
		}

		[[nodiscard]] constexpr bool touch_blocked() const noexcept{
			return interactivity == interactivity::disabled || interactivity == interactivity::intercepted;
		}

		[[nodiscard]] bool contains(math::vec2 absPos) const noexcept;

		[[nodiscard]] virtual bool contains_self(math::vec2 absPos, float margin) const noexcept;

		[[nodiscard]] virtual bool contains_parent(math::vec2 cursorPos) const noexcept;

		/*virtual*/ void notify_layout_changed(spread_direction toDirection) noexcept;

		virtual bool resize(const math::vec2 size/*, spread_direction Mask*/){
			if(property.size.set_size(size)){
				notify_layout_changed(spread_direction::all);
				return true;
			}
			return false;
		}

	protected:
		void set_scene(scene* s) noexcept{
			this->scene_ = s;
			for(const auto& element : get_children()){
				element->set_scene(s);
			}
		}

	public:

		[[nodiscard]] virtual std::span<const elem_ptr> get_children() const noexcept{
			return {};
		}

		[[nodiscard]] bool has_children() const noexcept{
			return !get_children().empty();
		}

		virtual void update(float delta_in_ticks);

	public:

		virtual bool try_layout(){
			if(layout_state.is_changed()){
				layout();
				return true;
			}
			return false;
		}

		virtual void layout(){
			layout_state.clear();
		}

		void exhausted_layout(){
			std::size_t count{};
			while(layout_state.is_changed() || layout_state.is_children_changed()){
				layout();
				count++;
				if(count > 16){
					throw std::runtime_error("bad layout");
				}
			}
		}

		[[nodiscard]] constexpr stated_extent clip_boarder_from(stated_extent extent) const noexcept{
			if(extent.width.mastering()){extent.width.value = std::fdim(extent.width.value, property.boarder.width());}
			if(extent.height.mastering()){extent.height.value = std::fdim(extent.height.value, property.boarder.height());}

			return extent;
		}

		[[nodiscard]] optional_mastering_extent clip_boarder_from(optional_mastering_extent extent) const noexcept{
			auto [dx, dy] = extent.get_mastering();
			if(dx)extent.set_width(std::fdim(extent.potential_width(), property.boarder.width()));
			if(dy)extent.set_height(std::fdim(extent.potential_height(), property.boarder.height()));

			return extent;
		}

	protected:

		/**
		 * @brief pre get the size of this elem if none/one side of extent is known
		 * @param extent : any tag of the length should be within {mastering, external}
		 * @return expected size, or nullopt
		 */
		virtual std::optional<math::vec2> pre_acquire_size_impl(optional_mastering_extent extent){
			return std::nullopt;
		}

	public:

		std::optional<math::vec2> pre_acquire_size_no_boarder_clip(optional_mastering_extent extent){
			return pre_acquire_size_impl(extent).transform([&, this](const math::vec2 v){
				return property.size.clamp(v + prop().boarder.extent()).min(extent.potential_extent());
			});
		}

		std::optional<math::vec2> pre_acquire_size(optional_mastering_extent extent){
			return pre_acquire_size_impl(clip_boarder_from(extent)).transform([&, this](const math::vec2 v){
				return property.size.clamp(v + prop().boarder.extent()).min(extent.potential_extent());
			});
		}

	protected:
		virtual void on_cursor_moved(const math::vec2 delta){
			cursor_state.time_stagnate = 0;
			if(get_tooltip_prop().use_stagnate_time && !delta.equals({})){
				cursor_state.time_tooltip = 0.;
				tooltip_notify_drop();
			}
		}

		virtual void on_key_input(const core::ctrl::key_code_t key, const core::ctrl::key_code_t action, const core::ctrl::key_code_t mode){

		}

		virtual void on_unicode_input(const char32_t val){

		}

		virtual input_event::click_result on_click(const input_event::click click_event){
			if(!interactable()){
				return input_event::click_result::spread;
			}

			switch(click_event.code.action()){
			case core::ctrl::act::press : cursor_state.pressed = true;
				break;
			default : cursor_state.pressed = false;
			}

			return input_event::click_result::intercepted;
		}

		virtual void on_drag(const input_event::drag event){

		}

		virtual void on_inbound_changed(bool is_inbounded, bool changed){
			cursor_state.inbound = is_inbounded;
		}

		virtual void on_focus_changed(bool is_focused){
			cursor_state.focused = is_focused;
			if(!is_focused)cursor_state.pressed = false;
		}

		virtual void on_scroll(const input_event::scroll event){

		}

	public:
		// [[nodiscard]] virtual math::vec2 requestSpace(const StatedSize sz, math::vec2 minimumSize,
		// 											  math::vec2 currentAllocatedSize){
		// 	auto cur = get_size();
		// 	if(sz.x.slaving()){
		// 		cur.x = currentAllocatedSize.x;
		// 	}
		//
		// 	if(sz.y.slaving()){
		// 		cur.y = currentAllocatedSize.y;
		// 	}
		//
		// 	return cur;
		// }


		/*virtual*/ void try_draw(const rect clipSpace) const{
			if(!is_visible()) return;

			if(!clipSpace.overlap_inclusive(get_bound()))return;
			// if(IgnoreClipWhenDraw || inboundOf(clipSpace)){
			draw(clipSpace);
			// }
		}

		void draw(const rect clipSpace) const{
			draw_content(clipSpace);
		}

		void dialog_notify_drop() const;

	protected:
		void draw_background() const;

		virtual void draw_content(const rect clipSpace) const{
			draw_background();
		}

		virtual void draw_independent() const {

		}

		bool insert_independent_draw(independent_draw_state state) const;
		bool erase_independent_draw() const noexcept;

		void tooltip_on_drop() override{
			stated_tooltip_owner::tooltip_on_drop();
			cursor_state.time_tooltip = -10.f;
		}

	public:
		virtual bool update_abs_src(math::vec2 parent_content_abs_src);

		virtual esc_flag on_esc(){
			if(has_tooltip()){
				tooltip_notify_drop();
				return esc_flag::intercept;
			}
			return esc_flag::fall_through;
		}


		template <std::derived_from<action::action<elem>> ActionType, typename... T>
			requires (std::constructible_from<ActionType, T&&...>)
		void push_action(T&&... args){
			actions.push(std::make_unique<ActionType>(std::forward<T>(args)...));
		}

		template <typename... ActionType>
			requires (std::derived_from<std::remove_cvref_t<ActionType>, action::action<elem>> && ...)
		void push_action(ActionType&&... args){
			(actions.push(std::make_unique<std::remove_cvref_t<ActionType>>(std::forward<ActionType>(args))), ...);
		}

		std::vector<elem*> dfs_find_deepest_element(math::vec2 cursorPos);

		[[nodiscard]] rect get_bound() const noexcept{
			return prop().bound_absolute();
		}

		[[nodiscard]] rect get_content_bound() const noexcept{
			return prop().content_bound_absolute();
		}


	protected:
		[[nodiscard]] tooltip_align tooltip_align_policy() const override{
			switch(tooltip_prop_.layout_info.follow){
			case tooltip_follow::owner :{
				return {
						.follow = tooltip_follow::owner,
						.pos = align::get_vert(tooltip_prop_.layout_info.align_owner, get_bound()),
						.align = tooltip_prop_.layout_info.align_tooltip,
						// .extent =
					};
			}

			default : return {tooltip_prop_.layout_info.follow, std::nullopt, tooltip_prop_.layout_info.align_tooltip};
			}
		}

		[[nodiscard]] bool tooltip_should_maintain(math::vec2 cursorPos) const override;

		[[nodiscard]] bool tooltip_owner_contains(math::vec2 cursorPos) const override{
			return contains(cursorPos);
		}

		[[nodiscard]] bool tooltip_should_build(math::vec2 cursorPos) const override{
			return true
				&& has_tooltip_builder()
				&& tooltip_prop_.auto_build()
				&& cursor_state.time_tooltip > tooltip_prop_.min_hover_time;
		}

	public:
		void build_tooltip(bool belowScene = false, bool fade_in = true);


		[[nodiscard]] quad_tree_rect_type quad_tree_get_bound() const noexcept{
			return get_bound();
		}

		[[nodiscard]] bool quad_tree_contains(typename quad_tree_vector_type::const_pass_t point) const{
			return contains(point);
		}
	private:
		[[nodiscard]] elem* get_parent_to_elem() const noexcept;
	};

	void iterateAll_DFSImpl(math::vec2 cursorPos, std::vector<elem*>& selected, elem* current);

	export
	template <std::derived_from<elem> D, std::derived_from<elem> B>
	D& elem_cast(B& b){
#if DEBUG_CHECK
		return dynamic_cast<D&>(b);
#else
		return static_cast<D&>(b);
#endif
	}

	export
	template <std::derived_from<elem> D, std::derived_from<elem> B>
	const D& elem_cast(const B& b){
#if DEBUG_CHECK
		return dynamic_cast<const D&>(b);
#else
		return static_cast<const D&>(b);
#endif
	}

	export
	namespace util{
		bool is_valid_release_click(const elem& elem, const input_event::click& click) noexcept{
			return elem.contains(click.pos) && click.code.action() == core::ctrl::act::release;
		}

		esc_flag thoroughly_esc(elem* where) noexcept;
		esc_flag thoroughly_esc(elem& where) noexcept{
			return thoroughly_esc(std::addressof(where));
		}
	}

	export
	template <typename T>
	concept ui_component = std::is_base_of_v<elem, T>;

	// template <typename T>
	// concept ui_constucable_cpm
	//
	// export void elementBuildTooltip(elem& element) noexcept{
	// 	if(!element.hasTooltip()){
	// 		element.buildTooltip();
	// 	} else{
	// 		(void)element.dropTooltip();
	// 	}
	// }
}