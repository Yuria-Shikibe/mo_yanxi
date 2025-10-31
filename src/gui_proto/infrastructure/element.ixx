//
// Created by Matrix on 2025/10/29.
//

export module mo_yanxi.gui.infrastructure:element;

export import mo_yanxi.math.vector2;
export import mo_yanxi.math.rect_ortho;
export import mo_yanxi.ui.clamped_size;

export import mo_yanxi.gui.layout.policies;
export import mo_yanxi.gui.flags;
export import mo_yanxi.gui.util;

export import align;

import :events;
import :scene;
export import :elem_ptr;



namespace mo_yanxi::gui{
export using boarder = align::spacing;
export using ui::clamped_fsize;

export constexpr inline boarder default_boarder{8, 8, 8, 8};

export
[[nodiscard]] stated_extent clip_boarder_from(stated_extent extent, const boarder boarder) noexcept{
	if(extent.width.mastering()){extent.width.value = std::fdim(extent.width.value, boarder.width());}
	if(extent.height.mastering()){extent.height.value = std::fdim(extent.height.value, boarder.height());}

	return extent;
}

export
[[nodiscard]] optional_mastering_extent clip_boarder_from(optional_mastering_extent extent, const boarder boarder) noexcept{
	auto [dx, dy] = extent.get_mastering();
	if(dx)extent.set_width(std::fdim(extent.potential_width(), boarder.width()));
	if(dy)extent.set_height(std::fdim(extent.potential_height(), boarder.height()));

	return extent;
}

export struct elem_ptr;


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

export struct elem{
	friend struct elem_ptr;
	friend struct scene;

private:
	void(*deleter_)(elem*) noexcept = nullptr;
	scene* scene_{};
	elem* parent_{};

	clamped_fsize size_{};
	math::vec2 relative_pos_{};
	math::vec2 absolute_pos_{};
	boarder boarder_{default_boarder};

public:
	optional_mastering_extent restriction_extent{};

private:
	cursor_states cursor_states_{};

	math::bool2 fill_parent_{};
	bool extend_focus_until_mouse_drop_{};

public:
	//TODO using bit flags?
	bool toggled{};
	bool disabled{};
	bool invisible{};
	bool sleep{};

	bool is_transparent_in_inbound_filter{};

private:
	//TODO direct access
	bool has_scene_direct_access{};

public:
	layout_state layout_state{};
	interactivity_flag interactivity{};


	virtual ~elem(){
		clear_scene_references();
	}

	[[nodiscard]] elem(scene& scene, elem* parent) noexcept : scene_(std::addressof(scene)), parent_(parent){

	}

	elem(const elem& other) = delete;
	elem(elem&& other) noexcept = delete;
	elem& operator=(const elem& other) = delete;
	elem& operator=(elem&& other) noexcept = delete;

#pragma region Event

	virtual void on_inbound_changed(bool is_inbounded, bool changed){
		cursor_states_.inbound = is_inbounded;
	}

	virtual void on_focus_changed(bool is_focused){
		cursor_states_.focused = is_focused;
		if(!is_focused)cursor_states_.pressed = false;
	}

	virtual events::op_afterwards on_key_input(const input_handle::key_set key){
		return events::op_afterwards::fall_through;
	}

	virtual events::op_afterwards on_unicode_input(const char32_t val){
		return events::op_afterwards::fall_through;
	}

	virtual events::op_afterwards on_click(const events::click event, std::span<elem*> aboves){
		if(!is_interactable()){
			return events::op_afterwards::fall_through;
		}

		switch(event.key.action){
		case input_handle::act::press : cursor_states_.pressed = true;
			break;
		default : cursor_states_.pressed = false;
		}

		return events::op_afterwards::intercepted;
	}

	virtual events::op_afterwards on_drag(const events::drag event){
		if(!is_interactable()){
			return events::op_afterwards::fall_through;
		}
		return events::op_afterwards::intercepted;
	}


	virtual events::op_afterwards on_cursor_moved(const events::cursor_move event){
		cursor_states_.time_stagnate = 0;
		return events::op_afterwards::fall_through;
		// if(get_tooltip_prop().use_stagnate_time && !delta.equals({})){
		// 	cursor_state.time_tooltip = 0.;
		// 	tooltip_notify_drop();
		// }
	}

	virtual events::op_afterwards on_scroll(const events::scroll event){
		return events::op_afterwards::fall_through;
	}

	virtual events::op_afterwards on_esc(){
		return events::op_afterwards::fall_through;
	}

#pragma endregion

#pragma region Draw
public:
	void try_draw(const rect clipSpace) const{
		if(invisible) return;
		if(!clipSpace.overlap_inclusive(abs_bound())) return;
		draw(clipSpace);
	}

	void draw(const rect clipSpace) const{
		draw_content(clipSpace);
	}

protected:
	void draw_background() const;

	virtual void draw_content(const rect clipSpace) const{
		draw_background();
	}
public:

#pragma endregion

#pragma region Behavior
public:

	virtual void update(float delta_in_ticks);

	void clear_scene_references() noexcept;

#pragma endregion

#pragma region Layout
protected:

	/**
	 * @brief pre get the size of this elem if none/one side of extent is known
	 *
	 * *recommended* to be const
	 *
	 * @param extent : any tag of the length should be within {mastering, external}
	 * @return expected size, or nullopt
	 */
	virtual std::optional<math::vec2> pre_acquire_size_impl(optional_mastering_extent extent){
		return std::nullopt;
	}

public:
	std::optional<math::vec2> pre_acquire_size_no_boarder_clip(const optional_mastering_extent extent);

	std::optional<math::vec2> pre_acquire_size(const optional_mastering_extent extent);

	void notify_layout_changed(propagate_mask propagation) noexcept;

	//TODO
	void notify_isolated_layout_changed();

protected:
	virtual bool resize_impl(const math::vec2 size){
		if(size_.set_size(size)){
			notify_layout_changed(propagate_mask::all);
			return true;
		}
		return false;
	}

public:
	bool resize(const math::vec2 size, propagate_mask temp_mask){
		const auto last = std::exchange(layout_state.inherent_broadcast_mask, temp_mask);
		const auto rst = resize_impl(size);
		layout_state.inherent_broadcast_mask = last;
		return rst;
	}

	bool resize(const math::vec2 size){
		return resize_impl(size);
	}

	virtual void layout(){
		layout_state.clear();
	}

	bool try_layout(){
		if(layout_state.any_lower_changed()){
			layout();
			return true;
		}
		return false;
	}


#pragma endregion

#pragma region Group
public:

	[[nodiscard]] virtual std::span<const elem_ptr> children() const noexcept{
		return {};
	}

	[[nodiscard]] bool has_children() const noexcept{
		return !children().empty();
	}

#pragma endregion

#pragma region Bound


[[nodiscard]] bool contains(math::vec2 absPos) const noexcept;

[[nodiscard]] bool contains_self(math::vec2 absPos, float margin) const noexcept;

[[nodiscard]] virtual bool parent_contain_constrain(math::vec2 cursorPos) const noexcept;

	virtual bool update_abs_src(math::vec2 parent_content_src) noexcept{
		return util::try_modify(absolute_pos_, parent_content_src + relative_pos_);
	}

#pragma endregion

#pragma region State

	[[nodiscard]] bool is_focused_scroll() const noexcept;
	[[nodiscard]] bool is_focused_key() const noexcept;

	[[nodiscard]] bool is_focused() const noexcept;
	[[nodiscard]] bool is_inbounded() const noexcept;

	void set_focused_scroll(bool focus) noexcept;
	void set_focused_key(bool focus) noexcept;

	virtual void on_focus_key_changed(bool isFocused){

	}
#pragma endregion

#pragma region Trivial_Getter_Setters
public:
	[[nodiscard]] math::bool2 get_fill_parent() const noexcept{
		return fill_parent_;
	}

	[[nodiscard]] constexpr bool is_toggled() const noexcept{ return toggled; }

	[[nodiscard]] constexpr bool is_visible() const noexcept{ return !invisible; }

	[[nodiscard]] constexpr bool is_sleep() const noexcept{ return sleep; }

	[[nodiscard]] constexpr bool is_disabled() const noexcept{ return disabled; }


	[[nodiscard]] bool is_focus_extended_by_mouse() const noexcept{
		return extend_focus_until_mouse_drop_;
	}

	void set_focus_extended_by_mouse(bool b) noexcept{
		extend_focus_until_mouse_drop_ = b;
	}

	[[nodiscard]] bool is_root_element() const noexcept{
		return parent_ == nullptr;
	}

	elem* set_parent(elem* const parent) noexcept{
		return std::exchange(parent_, parent);
	}

	[[nodiscard]] scene& get_scene() const noexcept{
		return *scene_;
	}

	[[nodiscard]] elem* parent() const noexcept{
		return parent_;
	}

	template <std::derived_from<elem> T, bool unchecked = false>
	[[nodiscard]] T* parent() const noexcept{
		if constexpr (!unchecked && !std::same_as<T, elem>){
			return dynamic_cast<T*>(parent_);
		}else{
			return static_cast<T*>(parent_);
		}
	}

	[[nodiscard]] vec2 content_extent() const noexcept{
		const auto [w, h] = size_.get_size();
		const auto [bw, bh] = boarder_.extent();
		return {std::fdim(w, bw), std::fdim(h, bh)};
	}

	[[nodiscard]] vec2 extent() const noexcept{
		return size_.get_size();
	}

	[[nodiscard]] rect abs_bound() const noexcept{
		return rect{tags::unchecked, tags::from_extent, abs_pos(), extent()};
	}

	[[nodiscard]] rect rel_bound() const noexcept{
		return rect{tags::unchecked, tags::from_extent, rel_pos(), extent()};
	}

	[[nodiscard]] math::vec2 abs_pos() const noexcept{
		return absolute_pos_;
	}

	[[nodiscard]] math::vec2 rel_pos() const noexcept{
		return relative_pos_;
	}

	[[nodiscard]] constexpr math::vec2 content_src_offset() const noexcept{
		return boarder_.top_lft();
	}

	[[nodiscard]] rect abs_content_bound() const noexcept{
		return rect{tags::unchecked, tags::from_extent, abs_pos() + content_src_offset(), content_extent()};
	}

	[[nodiscard]] rect rel_content_bound() const noexcept{
		return rect{tags::unchecked, tags::from_extent, rel_pos() + content_src_offset(), content_extent()};
	}

	[[nodiscard]] constexpr math::vec2 abs_content_src_pos() const noexcept{
		return abs_pos() + content_src_offset();
	}

	[[nodiscard]] constexpr math::vec2 rel_content_src_pos() const noexcept{
		return rel_pos() + content_src_offset();
	}


	[[nodiscard]] elem* root_parent() const noexcept{
		elem* cur = parent();
		if(!cur) return nullptr;

		while(true){
			if(const auto next = cur->parent()){
				cur = next;
			} else{
				break;
			}
		}
		return cur;
	}

	[[nodiscard]] constexpr bool is_interactable() const noexcept{
		if(invisible) return false;
		if(disabled) return false;
		if(touch_blocked())return false;
		return true;
	}

	[[nodiscard]] constexpr bool ignore_inbound() const noexcept{
		return is_transparent_in_inbound_filter;
	}

	[[nodiscard]] constexpr bool touch_blocked() const noexcept{
		return interactivity == interactivity_flag::disabled || interactivity == interactivity_flag::intercept;
	}
	// void update_opacity(const float val) noexcept{
	// 	if(util::try_modify(property.graphic_data.context_opacity, val)){
	// 		for(const auto& element : get_children()){
	// 			element->update_opacity(gprop().get_opacity());
	// 		}
	// 	}
	// }

#pragma endregion

protected:
	template <typename T>
	std::pmr::polymorphic_allocator<T> get_allocator(){
		return scene_->get_allocator<T>();
	}

private:
	void reset_scene(struct scene* scene) noexcept;
};

namespace util{

template <typename Alloc>
void iterateAll_DFSImpl(
	math::vec2 cursorPos,
	std::vector<elem*, Alloc>& selected,
	elem* current){
	if(current->is_disabled() || current->interactivity == interactivity_flag::disabled)return;

	if(!current->ignore_inbound() && current->contains(cursorPos)){
		selected.push_back(current);
	}

	if(current->touch_blocked() || !current->has_children()) return;

	//TODO transform?
	auto transformed = cursorPos;

	for(const auto& child : current->children()/* | std::views::reverse*/){
		if(!child->is_visible())continue;
		util::iterateAll_DFSImpl<Alloc>(transformed, selected, child.get());

		//TODO better inbound shadow, maybe dialog system instead of add to root
		if(child->interactivity == interactivity_flag::intercept && child->get_fill_parent().x && child->get_fill_parent().y){
			break;
		}
	}
}

template <typename Alloc = std::allocator<elem*>>
std::vector<elem*, std::remove_cvref_t<Alloc>> dfs_find_deepest_element(elem* root, math::vec2 cursorPos, Alloc&& alloc = {}){
	std::vector<elem*, std::remove_cvref_t<Alloc>> rst{std::forward<Alloc>(alloc)};

	util::iterateAll_DFSImpl(cursorPos, rst, root);

	return rst;
}

}

export
template <std::derived_from<elem> D, bool unchecked = false, std::derived_from<elem> B>
D& elem_cast(B& b) noexcept(unchecked || std::same_as<D, elem>){
	if constexpr (unchecked || std::same_as<D, elem>){
		return static_cast<D&>(b);
	}else{
		return dynamic_cast<D&>(b);
	}
}

export
template <std::derived_from<elem> D, bool unchecked = false, std::derived_from<elem> B>
const D& elem_cast(const B& b) noexcept(unchecked || std::same_as<D, elem>){
	if constexpr (unchecked || std::same_as<D, elem>){
		return static_cast<const D&>(b);
	}else{
		return dynamic_cast<const D&>(b);
	}
}

std::pmr::polymorphic_allocator<> elem_ptr::alloc_of(const scene& s) noexcept{
	return s.get_allocator();
}

std::pmr::polymorphic_allocator<> elem_ptr::alloc_of(const elem* ptr) noexcept{
	return ptr->get_scene().get_allocator();
}

void elem_ptr::set_deleter(elem* element, void(* p)(elem*) noexcept) noexcept{
	element->deleter_ = p;
}

void elem_ptr::delete_elem(elem* ptr) noexcept{
	ptr->deleter_(ptr);
}


}