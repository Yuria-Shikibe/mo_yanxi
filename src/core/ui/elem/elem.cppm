module;

#include <cassert>

export module mo_yanxi.ui.elem;

export import mo_yanxi.math;
export import mo_yanxi.math.vector2;
export import mo_yanxi.math.rect_ortho;
export import mo_yanxi.event;

export import Align;

export import mo_yanxi.graphic.color;

export import mo_yanxi.ui.flags;
export import mo_yanxi.ui.pre_decl;
export import mo_yanxi.ui.comp.drawer;
export import mo_yanxi.ui.event_types;
export import mo_yanxi.ui.clamped_size;
export import mo_yanxi.ui.util;
export import mo_yanxi.ui.elem_ptr;
export import mo_yanxi.ui.layout.policies;
// export import Core.UI.Drawer;
// export import Core.UI.Flags;
// export import Core.UI.Util;
// export import Core.UI.CellBase;
// export import Core.UI.Action;
// export import Core.UI.Util;

// export import Core.Ctrl.Constants;
//
// import Core.UI.StatedLength;
//
// import Graphic.Color;
//
// import Core.UI.ToolTipInterface;
// import math.QuadTree.Interface;
//

import mo_yanxi.meta_programming;
import mo_yanxi.handle_wrapper;
import mo_yanxi.owner;
import mo_yanxi.func_initialzer;

import std;

namespace mo_yanxi{
	namespace graphic{
		export struct renderer_ui;
		// struct renderer_ui;
	}
}


namespace mo_yanxi::ui{
	// /*export*/ struct elem;
	// export struct group;
	// export struct scene;


	export constexpr inline bool IgnoreClipWhenDraw = false;

	export using rect = math::frect;

	//TODO boarder requirement ?
	export struct debug_elem_drawer final : public style_drawer<struct elem>{
		void draw(const struct elem& element, rect region, float opacityScl) const override;
	};

	export struct empty_drawer final : public style_drawer<struct elem>{
		void draw(const struct elem& element, rect region, float opacityScl) const override{
		}

		// float content_opacity(const elem& element) const override{}
	};

	export constexpr inline align::spacing DefaultBoarder{BoarderStroke, BoarderStroke, BoarderStroke, BoarderStroke};

	export constexpr inline debug_elem_drawer DefaultStyleDrawer;
	export constexpr inline empty_drawer EmptyStyleDrawer;

	export inline const style_drawer<elem>* global_style_drawer;

	namespace events{
		// export
		// using ElementConnection = ConnectionEvent<elem>;
		// export
		// using GeneralCodeEvent = CodeEvent<>;
	}

	export using elem_event_manager =
	mo_yanxi::events::event_manager<
		std::move_only_function<void(struct elem&) const>,
		// events::GeneralCodeEvent,
		// events::ElementConnection,
		events::click,
		events::drag,
		events::exbound,
		events::inbound,
		events::focus_begin,
		events::focus_end,
		events::scroll,
		events::moved>;

	const style_drawer<struct elem>* getDefaultStyleDrawer(){
		return global_style_drawer ? global_style_drawer : &DefaultStyleDrawer;
	}

	export struct elem_graphic_data{
		const style_drawer<struct elem>* drawer{getDefaultStyleDrawer()};

		graphic::color style_color_scl{graphic::colors::white};
		graphic::color style_color_ovr{};
		float inherent_opacity{1.f};

		float context_opacity{1.f};

		[[nodiscard]] constexpr float get_opacity() const noexcept{
			return inherent_opacity * context_opacity;
		}

		// [[nodiscard]] constexpr Graphic::Color getScaledColor() const noexcept{
		// 	return style_color.copy().mulA(getOpacity());
		// }

		// mutable Graphic::Color tmpColor{};
		//TODO drawer
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


		bool inbound{};
		bool focused{};
		bool pressed{};

		void update(const float delta_in_ticks){
			if(inbound){
				math::approach_inplace(time_inbound, maximum_duration, delta_in_ticks);
			} else{
				math::approach_inplace(time_inbound, 0, delta_in_ticks);
			}

			if(focused){
				math::approach_inplace(time_focus, maximum_duration, delta_in_ticks);
				math::approach_inplace(time_stagnate, maximum_duration, delta_in_ticks);
			} else{
				math::approach_inplace(time_focus, 0, delta_in_ticks);
				time_stagnate = 0.;
			}
		}

		void registerFocusEvent(elem_event_manager& event_manager);

		void registerDefEvent(elem_event_manager& event_manager){
			// event_manager.on<events::EndFocus>([this](auto){
			// 	pressed = false;
			// });
			//
			// event_manager.on<events::Inbound>([this](auto){
			// 	inbound = true;
			// });
			//
			// event_manager.on<events::Exbound>([this](auto){
			// 	inbound = false;
			// });
			//
			// event_manager.on<events::Click>([this](const events::Click& click){
			// 	switch(click.code.action()){
			// 		case Ctrl::Act::Press: pressed = true; break;
			// 		default: pressed = false;
			// 	}
			// });
		}
	};



	export struct elem_prop{
	protected:
		//TODO uses string view?
		std::string elementTypeName{};

	public:
		[[nodiscard]] elem_prop() = default;

		[[nodiscard]] elem_prop(const std::string_view element_type_name)
			: elementTypeName(element_type_name){
		}

		std::string name{};


		//position fields
		math::vec2 relative_src{};
		math::vec2 absolute_src{};

		math::vector2<bool> fill_parent{};
		bool maintain_focus_until_mouse_drop{};

		clamped_fsize size{};
		align::spacing boarder{DefaultBoarder};

		float scale_context{1.};
		float scale_local{1.};

		elem_graphic_data graphicData{};


		void set_empty_drawer(){
			boarder = {};
			graphicData.drawer = &EmptyStyleDrawer;
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
					size.get_maximum_size()).sub(boarder.get_size()
				).max({});
		}

		[[nodiscard]] constexpr float content_width() const noexcept{
			return size.get_width() - boarder.width();
		}

		[[nodiscard]] constexpr float content_height() const noexcept{
			return size.get_height() - boarder.height();
		}

		[[nodiscard]] constexpr math::vec2 content_size() const noexcept{
			return (size.get_size() - boarder.get_size()).max({});
		}

		[[nodiscard]] constexpr math::vec2 get_size() const noexcept{
			return size.get_size();
		}
	};

	export struct elem_datas{

		elem_prop property{};
		cursor_states cursor_state{};

		stated_extent context_size_restriction{};
		/**
		 * @brief size restriction in a layout context, usually is gain from a cell
		 */
		// math::vec2 context_bound{math::vectors::constant2<float>::inf_positive_vec2};
		// bool expandable{false};
	protected:
		group* parent{};
		scene* scene_{};

		// std::queue<std::unique_ptr<Action<elem>>> actions{};

	public:
		elem_event_manager event_slots{};
		ElemDynamicChecker<elem> checkers{};
		//state
		//TODO using a bit flags?
		bool activated{}; //TODO as graphic property? / rename it
		bool disabled{};
		bool visible{true};
		bool sleep{};

		//TODO move this to property?
		bool skip_inbound_capture{};

		//Layout Spec
		layout_state layoutState{};
		interactivity interactivity{interactivity::enabled};

	protected:
		[[nodiscard]] explicit elem_datas(const std::string_view tyName)
			: property(tyName){
		}

		[[nodiscard]] elem_datas() = default;
	};

	export struct elem : elem_datas
		// :

		// StatedToolTipOwner<elem>,
		// math::QuadTreeAdaptable<elem>
	{

		[[nodiscard]] elem(){
			cursor_state.registerFocusEvent(events());
			event_slots.set_context(*this);
		}

		[[nodiscard]] elem(
			scene* scene,
			group* group = nullptr,
			const std::string_view tyName = "")
			: elem_datas{tyName}{
			elem::set_scene(scene);
			elem::set_parent(group);

			cursor_state.registerFocusEvent(events());
			event_slots.set_context(*this);
		}

		virtual ~elem(){
			clear_external_references();
		}

		elem(const elem& other) = delete;

		elem& operator=(const elem& other) = delete;

		elem(elem&& other) noexcept: elem_datas{std::move(other)}{
			event_slots.set_context(*this);
		}

		elem& operator=(elem&& other) noexcept{
			if(this == &other) return *this;
			elem_datas::operator =(std::move(other));
			event_slots.set_context(*this);
			return *this;
		}

		// [[nodiscard]] Graphic::Batch_Direct& getBatch() const noexcept;
		//
		[[nodiscard]] graphic::renderer_ui& get_renderer() const noexcept;

		[[nodiscard]] const cursor_states& get_cursor_state() const noexcept{
			return cursor_state;
		}

		void set_style(const style_drawer<elem>& drawer){
			property.graphicData.drawer = &drawer;
			if(drawer.apply_to(*this)){
				notify_layout_changed(spread_direction::all_visible);
			}
		}

		void set_style(){
			property.set_empty_drawer();
		}

		bool resize_quiet(const math::vec2 size, spread_direction mask = spread_direction::none){
			auto last = std::exchange(layoutState.acceptMask_inherent, mask);
			auto rst = resize(size);
			layoutState.acceptMask_inherent = last;
			return rst;
		}

		[[nodiscard]] constexpr math::vec2 get_size() const noexcept{
			return property.size.get_size();
		}

		[[nodiscard]] constexpr math::vec2 content_size() const noexcept{
			return property.content_size();
		}

		[[nodiscard]] constexpr math::vec2 content_potential_size() const noexcept{
			return content_potential_size(context_size_restriction);
		}
		[[nodiscard]] constexpr math::vec2 content_potential_size(stated_extent extent) const noexcept{
			auto sz = property.content_size();
			if(extent.width.dependent()){
				sz.x = std::numeric_limits<float>::infinity();
			}
			if(extent.height.dependent()){
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

		[[nodiscard]] group* get_parent() const noexcept{
			return parent;
		}

		[[nodiscard]] group* get_root_parent() const noexcept;

		virtual void update_opacity(float val);

		void mark_independent_layout_changed();

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

		[[nodiscard]] const auto& graphicProp() const noexcept{
			return property.graphicData;
		}

		elem_event_manager& events() noexcept{
			return event_slots;
		}

		/**
		 * @brief just used to notify tooltip drop
		 * @param delta cursor transform
		 */
		void notify_cursor_moved(const math::vec2 delta) const{
			event_slots.fire(events::moved{delta});
			// dropToolTipIfMoved();
		}

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

		void remove_self_from_parent();
		void remove_self_from_parent_instantly();

		void registerAsyncTask();

		void notify_remove();

		void clear_external_references() noexcept;

		[[nodiscard]] bool is_focused_scroll() const noexcept;
		[[nodiscard]] bool is_focused_key() const noexcept;

		[[nodiscard]] bool is_focused() const noexcept;
		[[nodiscard]] bool is_inbounded() const noexcept;

		void set_focused_scroll(bool focus) noexcept;
		void set_focused_key(bool focus) noexcept;
		//
		// [[nodiscard]] constexpr bool isQuietInteractable() const noexcept{
		// 	return (interactivity != Interactivity::enabled && hasTooltip()) && isVisible();
		// }

		[[nodiscard]] constexpr bool ignore_inbound() const noexcept{
			return skip_inbound_capture;
		}

		[[nodiscard]] constexpr bool interactable() const noexcept{
			if(!is_visible()) return false;
			if(disabled) return false;
			return interactivity == interactivity::enabled;
		}

		[[nodiscard]] constexpr bool touch_disabled() const noexcept{
			return interactivity == interactivity::disabled;
		}

		[[nodiscard]] bool contains(math::vec2 absPos) const noexcept;

		[[nodiscard]] virtual bool contains_self(math::vec2 absPos, float margin = 0.f) const noexcept;

		[[nodiscard]] virtual bool contains_parent(math::vec2 cursorPos) const;

		virtual void notify_layout_changed(spread_direction toDirection);

		virtual bool resize(const math::vec2 size/*, spread_direction Mask*/){
			if(property.size.set_size(size)){
				notify_layout_changed(spread_direction::all);
				return true;
			}
			return false;
		}


		virtual void set_scene(scene* s){
			this->scene_ = s;
		}


		[[nodiscard]] virtual std::span<const elem_ptr> get_children() const noexcept{
			return {};
		}

		[[nodiscard]] bool has_children() const noexcept{
			return !get_children().empty();
		}


		virtual void update(float delta_in_ticks);

		virtual bool try_layout(){
			if(layoutState.is_changed()){
				layout();
				return true;
			}
			return false;
		}

		virtual void layout(){
			layoutState.clear();
		}

		void exhaustedLayout(){
			std::size_t count{};
			while(layoutState.is_changed() || layoutState.is_children_changed()){
				layout();
				count++;
				if(count > 16){
					throw std::runtime_error("bad layout");
				}
			}
		}

		virtual std::optional<math::vec2> pre_acquire_size(stated_extent extent){
			return std::nullopt;
		}

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


		virtual void try_draw(const rect clipSpace, rect redirect) const{
			if(!is_visible()) return;
			// if(IgnoreClipWhenDraw || inboundOf(clipSpace)){
			draw(clipSpace, redirect);
			// }
		}

		void draw(const rect clipSpace) const{
			draw(clipSpace, get_bound());
		}

		void draw(const rect clipSpace, rect redirect) const{
			draw_pre(clipSpace, redirect);
			draw_content(clipSpace, redirect);
			draw_post(clipSpace, redirect);
		}

		virtual void draw_pre(const rect clipSpace, rect redirect) const;

		virtual void draw_content(const rect clipSpace, rect redirect) const{

		}
		virtual void draw_post(const rect clipSpace, rect redirect) const{

		}

		virtual bool update_abs_src(math::vec2 parent_content_abs_src);
		//
		// virtual void drawMain(Rect clipSpace) const;
		//
		// virtual void drawPost() const{
		// }

		/*
		template <ElemInitFunc InitFunc>
		void setTooltipState(const ToolTipProperty& toolTipProperty, InitFunc&& initFunc) noexcept{
			StatedToolTipOwner::setTooltipState(toolTipProperty, std::forward<InitFunc>(initFunc));
		}

		template <ElemInitFunc InitFunc>
		void setTooltipState_asDialog(InitFunc&& initFunc) noexcept{
			this->setTooltipState(ToolTipProperty{
				                      .followTarget = mo_yanxi::ui::TooltipFollow::sceneDialog,
				                      .followTargetAlign = Align::Pos::center,
				                      .tooltipSrcAlign = Align::Pos::center,
				                      .autoRelease = false,
				                      .minHoverTime = mo_yanxi::ui::ToolTipProperty::DisableAutoTooltip,
			                      }, std::forward<InitFunc>(initFunc));
		}

		void buildTooltip(bool belowScene = false) noexcept;

		template <std::derived_from<Action<elem>> ActionType, typename... T>
		void pushAction(T&&... args){
			actions.push(std::make_unique<ActionType>(std::forward<T>(args)...));
		}

		template <typename... ActionType>
			requires (std::derived_from<std::decay_t<ActionType>, Action<elem>> && ...)
		void pushAction(ActionType&&... args){
			actions.push(std::make_unique<std::decay_t<ActionType>>(std::forward<ActionType>(args))...);
		}

		// ------------------------------------------------------------------------------------------------------------------
		// interface region
		// ------------------------------------------------------------------------------------------------------------------




		/**
		 * @brief
		 * @return false if should not fallback
		 #1#
		virtual bool onEsc(){
			return true;
		}

		[[nodiscard]] std::optional<math::vec2> getFitnessSize() const noexcept;

		// [[nodiscard]]  bool keyFocusShouldDrop(const math::vec2 pos) const noexcept{
		// 	return !containsPos(pos);
		// }


		virtual void inputUnicode(const char32_t val){
		}




		virtual void getFuture(){
		}

		[[nodiscard]] TooltipAlignPos getAlignPolicy() const override;

		[[nodiscard]] bool tooltipShouldDrop(const math::vec2 cursorPos) const override{
			return tooltipProp.autoRelease && !containsPos(cursorPos) && !isFocusedKey();
		}

		[[nodiscard]] bool tooltipShouldBuild(const math::vec2 cursorPos) const override{
			return true
				&& hasTooltipBuilder()
				&& tooltipProp.autoBuild()
				&& (tooltipProp.useStagnateTime ? cursorState.stagnateTime : cursorState.focusedTime)
				> tooltipProp.minHoverTime;
		}

	protected:
		void tooltipNotifyDrop() override{
			cursorState.focusedTime = cursorState.stagnateTime = -15.f;
		}

	public:
		[[nodiscard]] bool hasTooltip() const noexcept;
		bool dropTooltip() const noexcept;

	protected:
		[[nodiscard]] bool inboundOf(const Rect& clipSpace_abs) const noexcept{
			return clipSpace_abs.overlap_Exclusive(property.bound_absolute());
		}

		void dropToolTipIfMoved() const;
	*/
	public:
		std::vector<elem*> dfsFindDeepestElement(math::vec2 cursorPos);

		[[nodiscard]] rect get_bound() const noexcept{
			return prop().bound_absolute();
		}

		// [[nodiscard]] bool roughIntersectWith(const elem& other) const noexcept{
		// 	return get_bound().overlap_Exclusive(other.get_bound());
		// }

		// [[nodiscard]] bool containsPoint(typename math::vec2 point) const{
		// 	return contains(point);
		// }
	};

	void iterateAll_DFSImpl(math::vec2 cursorPos, std::vector<struct elem*>& selected, struct elem* current);
	//
	// export void elementBuildTooltip(elem& element) noexcept{
	// 	if(!element.hasTooltip()){
	// 		element.buildTooltip();
	// 	} else{
	// 		(void)element.dropTooltip();
	// 	}
	// }
}

module : private;
