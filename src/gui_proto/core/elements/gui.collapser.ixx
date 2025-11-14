//
// Created by Matrix on 2025/11/2.
//

export module mo_yanxi.gui.elem.collapser;


export import mo_yanxi.gui.infrastructure;
export import mo_yanxi.gui.layout.policies;

import std;

namespace mo_yanxi::gui{
export
//TODO as bit flag?
enum struct collapser_expand_cond : std::uint8_t{
	click,
	inbound,
	focus,
	pressed,
};

enum struct collapser_state : std::uint8_t{
	un_expand,
	expanding,
	expanded,
	exit_expanding,
};

struct collapser_settings{
	//TODO move cond to this struct?

	float expand_enter_spacing{30.f};
	float expand_exit_spacing{30.f};
	float expand_speed{3.15f / 60.f};
};

export
struct collapser : elem{
private:
	std::array<elem_ptr, 2> items{};
	std::array<float, 2> item_size{layout::pending_size, layout::pending_size};
	layout::layout_policy layout_policy_{
			search_parent_layout_policy(false).value_or(layout::layout_policy::hori_major)
		};

	float pad_ = 8;
	float expand_reload_{};

	collapser_expand_cond expand_cond_{};
	collapser_state state_{};
	bool clicked_{};
	bool update_opacity_during_expand_{};

	void update_item_size(bool isContent) const;

	void set_item_size(bool isContent, float size){
		if(util::try_modify(item_size[isContent], size)){
			update_item_size(isContent);
		}
	}

	void update_item_size() const{
		update_item_size(false);
		update_item_size(true);
	}

	bool update_abs_src(math::vec2 parent_content_src) noexcept override;

	void update_collapse(float delta) noexcept;

public:
	collapser_settings settings{};

	[[nodiscard]] collapser(scene& scene, elem* parent)
		: elem(scene, parent){
		interactivity = gui::interactivity_flag::children_only;
		layout_state.intercept_lower_to_isolated = true;
	}

	[[nodiscard]] elem& head() const noexcept{
		return *items[0];
	}

	[[nodiscard]] elem& content() const noexcept{
		return *items[1];
	}

	[[nodiscard]] collapser_expand_cond get_expand_cond() const noexcept{
		return expand_cond_;
	}

	void set_expand_cond(const collapser_expand_cond expand_cond){
		expand_cond_ = expand_cond;
	}

	[[nodiscard]] bool expandable() const noexcept{
		//TODO maintain if children has tooltip?
		switch(expand_cond_){
		case collapser_expand_cond::inbound:
			return cursor_states_.inbound;
		case collapser_expand_cond::focus:
			return cursor_states_.focused;
		case collapser_expand_cond::pressed:
			return cursor_states_.pressed;
		default:
			return clicked_;
		}
	}

	[[nodiscard]] layout::layout_policy get_layout_policy() const{
		return layout_policy_;
	}

	void set_layout_policy(const layout::layout_policy layout_policy){
		if(util::try_modify(layout_policy_, layout_policy)){
			notify_isolated_layout_changed();
		}
	}

	void set_head_size(float size){
		set_item_size(false, size);
	}

	void set_content_size(float size){
		set_item_size(true, size);
	}


	template <std::derived_from<elem> E, typename... Args>
		requires (std::constructible_from<E, scene&, elem*, Args...>)
	E& emplace(bool as_content, Args&&... args){
		items[as_content] = elem_ptr{get_scene(), this, std::in_place_type<E>, std::forward<Args>(args)...};
		notify_isolated_layout_changed();
		return static_cast<E&>(*items[as_content]);
	}

	template <invocable_elem_init_func Fn, typename... Args>
	auto& create(
		bool as_content,
		Fn&& init,
		Args&&... args
	){
		items[as_content] = elem_ptr{get_scene(), this, std::forward<Fn>(init), std::forward<Args>(args)...};
		notify_isolated_layout_changed();
		return static_cast<elem_init_func_create_t<Fn>&>(*items[as_content]);
	}

	template <std::derived_from<elem> E, typename... Args>
		requires (std::constructible_from<E, scene&, elem*, Args...>)
	E& emplace_head(Args&&... args){
		return this->emplace<E>(false, std::forward<Args>(args)...);
	}

	template <std::derived_from<elem> E, typename... Args>
		requires (std::constructible_from<E, scene&, elem*, Args...>)
	E& emplace_content(Args&&... args){
		return this->emplace<E>(true, std::forward<Args>(args)...);
	}

	template <invocable_elem_init_func Fn, typename... Args>
	auto& create_head(Fn&& init, Args&&... args){
		return this->create(false, std::forward<Fn>(init), std::forward<Args>(args)...);
	}

	template <invocable_elem_init_func Fn, typename... Args>
	auto& create_content(Fn&& init, Args&&... args){
		return this->create(true, std::forward<Fn>(init), std::forward<Args>(args)...);
	}

	[[nodiscard]] std::span<const elem_ptr> children() const noexcept override{
		return items;
	}

	[[nodiscard]] bool is_update_opacity_during_expand() const{
		return update_opacity_during_expand_;
	}

	void set_update_opacity_during_expand(const bool update_opacity_during_expand) noexcept{
		if(util::try_modify(update_opacity_during_expand_, update_opacity_during_expand) && items[1]){
			if(update_opacity_during_expand){
				content().update_opacity(get_interped_progress() * get_draw_opacity());
			} else{
				content().update_opacity(get_draw_opacity());
			}
		}
	}

protected:
	std::optional<math::vec2> pre_acquire_size_impl(layout::optional_mastering_extent extent) override;

	events::op_afterwards on_click(const events::click event, std::span<elem* const> aboves) override;

	void draw_content(const rect clipSpace) const override;

	void layout_elem() override{
		elem::layout_elem();
		update_item_size();

		for(auto& item : items){
			item->try_layout();
		}
	}

	void update(float delta_in_ticks) override{
		elem::update(delta_in_ticks);

		for(auto& item : items){
			item->update(delta_in_ticks);
		}

		update_collapse(delta_in_ticks);
		content().invisible = state_ == collapser_state::un_expand;
	}

	[[nodiscard]] float get_interped_progress() const noexcept;

	[[nodiscard]] bool is_expanding() const noexcept{
		return state_ == collapser_state::expanding || state_ == collapser_state::exit_expanding;
	}

	[[nodiscard]] rect get_collapsed_region() const noexcept;

	[[nodiscard]] std::optional<layout::layout_policy> search_layout_policy_getter_impl() const noexcept override{
		return get_layout_policy();
	}
};

}
