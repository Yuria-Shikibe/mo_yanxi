module;

#include <cassert>

export module mo_yanxi.gui.infrastructure:scene;

import :elem_ptr;
import std;
import prototype.renderer.ui;
import mo_yanxi.handle_wrapper;
import mo_yanxi.math.rect_ortho;

import mo_yanxi.gui.util;
export import mo_yanxi.input_handle;


namespace mo_yanxi::gui{
export using vec2 = math::vec2;
export using rect = math::frect;

using allocator_type = std::pmr::polymorphic_allocator<>;
constexpr bool b = std::allocator_traits<allocator_type>::propagate_on_container_swap::value;
struct mouse_state{
	math::vec2 src{};
	bool pressed{};

	void reset(const math::vec2 pos) noexcept{
		src = pos;
		pressed = true;
	}

	void clear(const math::vec2 pos) noexcept{
		src = pos;
		pressed = false;
	}
};

export struct ui_manager;

export struct elem;

export
struct scene;

struct scene_base{
	friend elem;
	friend ui_manager;

private:
	std::unique_ptr<std::pmr::unsynchronized_pool_resource> resource_{};
	exclusive_handle_member<renderer*> renderer_{};

protected:
	elem_ptr root_{};
	rect region_;


	std::array<mouse_state, std::to_underlying(input_handle::mouse::Count)> mouse_states_{};
	input_handle::input_manager<scene&> inputs_{};

	elem* focus_scroll_{nullptr};
	elem* focus_cursor_{nullptr};
	elem* focus_key_{nullptr};

	std::pmr::vector<elem*> last_inbounds_{resource_.get()};
	std::pmr::unordered_set<elem*> independent_layouts_{resource_.get()};


	[[nodiscard]] scene_base() = default;

	template <std::derived_from<elem> T, typename ...Args>
	[[nodiscard]] explicit(false) scene_base(
		renderer& renderer,
		std::in_place_type_t<T>,
		Args&& ...args
		);

public:
	[[nodiscard]] renderer& renderer() const noexcept{
		assert(renderer_);
		return *renderer_;
	}

	template <typename T = std::byte>
	[[nodiscard]] std::pmr::polymorphic_allocator<T> get_allocator() const noexcept {
		return std::pmr::polymorphic_allocator<T>{resource_.get()};
	}

	template <std::derived_from<elem> T = elem, bool unchecked = false>
	T& root(){
		assert(root_ != nullptr);
		if constexpr (std::same_as<T, elem> || unchecked){
			return *static_cast<T*>(root_.get());
		}
		return dynamic_cast<T&>(*root_);
	}


	[[nodiscard]] rect get_region() const noexcept{
		return region_;
	}

	[[nodiscard]] vec2 get_extent() const noexcept{
		return region_.extent();
	}

	[[nodiscard]] vec2 get_cursor_pos() const noexcept{
		return inputs_.cursor_pos();
	}


	[[nodiscard]] bool is_mouse_pressed() const noexcept{
		return std::ranges::any_of(mouse_states_, std::identity{}, &mouse_state::pressed);
	}

	[[nodiscard]] bool is_mouse_pressed(input_handle::mouse mouse_button_code) const noexcept{
		return mouse_states_[std::to_underlying(mouse_button_code)].pressed;
	}


	void drop_event_focus(const elem* target) noexcept{
		if(focus_scroll_ == target)focus_scroll_ = nullptr;
		if(focus_cursor_ == target)focus_cursor_ = nullptr;
		if(focus_key_ == target)focus_key_ = nullptr;
	}

};


export
struct scene : scene_base{
	friend elem;
	friend ui_manager;

	using scene_base::scene_base;

	scene(const scene& other) = delete;
	scene(scene&& other) noexcept;
	scene& operator=(const scene& other) = delete;
	scene& operator=(scene&& other) noexcept;


private:
	void update(float delta_in_tick);

	void draw(rect clip) const;

	void draw() const{
		draw(region_);
	}

#pragma region Events
	void inform_input(const input_handle::key_set k){
		if(k.key_code < std::to_underlying(input_handle::mouse::Count)){
			update_mouse_state(k);
		}else{
			on_key_action(k);
		}
		inputs_.inform(k);
	}

	void update_mouse_state(input_handle::key_set k);

	void inform_cursor_move(math::vec2 pos){
		inputs_.cursor_move_inform(pos);
		on_cursor_pos_update(false);
	}

	void on_key_action(const input_handle::key_set key);

	void on_unicode_input(char32_t val) const;

	void on_scroll(const math::vec2 scroll) const;

	void on_cursor_pos_update(bool force_drop);
#pragma endregion


	void resize(const math::frect region);

	void layout();


	void update_inbounds(std::pmr::vector<elem*>&& next, bool force_drop);

	void try_swap_focus(elem* newFocus, bool force_drop);

	void swap_focus(elem* newFocus);

	void drop_all_focus(const elem* target) noexcept;

	void notify_isolated_layout_update(elem* element){
		independent_layouts_.insert(element);
	}

};

template <std::derived_from<elem> T, typename ... Args>
scene_base::scene_base(gui::renderer& renderer, std::in_place_type_t<T>, Args&&... args):
	resource_(std::make_unique<std::pmr::unsynchronized_pool_resource>()),
	renderer_(std::addressof(renderer)), root_(static_cast<scene&>(*this), nullptr, std::in_place_type<T>, std::forward<Args>(args)...){
	inputs_.main_binds.set_context(std::ref(static_cast<scene&>(*this)));
}
}