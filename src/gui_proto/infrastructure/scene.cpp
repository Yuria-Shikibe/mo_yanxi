module;

module mo_yanxi.gui.infrastructure;

namespace mo_yanxi::gui{
scene::scene(scene&& other) noexcept: scene_base{std::move(other)}{
	root().reset_scene(this);
	inputs_.main_binds.set_context(std::ref(*this));
}

scene& scene::operator=(scene&& other) noexcept{
	if(this == &other) return *this;
	scene_base::operator =(std::move(other));
	root().reset_scene(this);
	inputs_.main_binds.set_context(std::ref(*this));
	return *this;
}

void scene::update(float delta_in_tick){
	root().update(delta_in_tick);
}

void scene::draw(rect clip) const{
	gui::viewport_guard _{renderer(), region_};
	root_->draw(clip);
}

void scene::on_key_action(const input_handle::key_set key){

	if(key.action == input_handle::act::press && key.key_code == std::to_underlying(input_handle::key::esc)){
		//TODO onEsc

		// if(on_esc() == esc_flag::fall_through){
		// 	on_cursor_pos_update(get_cursor_pos(), true);
		// }
	}else{
		elem* focus = focus_key_;

		//TODO fallback
		if(!focus) focus = focus_cursor_;
		if(!focus) return;
		focus->on_key_input(key);
	}
}

void scene::on_unicode_input(char32_t val) const{
	if(focus_key_){
		focus_key_->on_unicode_input(val);
	}
}

void scene::on_scroll(const math::vec2 scroll) const{
	if(focus_scroll_){
		focus_scroll_->on_scroll(events::scroll{scroll, inputs_.main_binds.get_mode()});
	}
}

void scene::on_cursor_pos_update(bool force_drop){
	std::pmr::vector<elem*> inbounds{get_allocator()};

	//TODO tooltip & dialog window
	// for (auto && activeTooltip : tooltip_manager.get_active_tooltips() | std::views::reverse){
	// 	if(tooltip_manager.is_below_scene(activeTooltip.element.get()))continue;
	// 	inbounds = activeTooltip.element->dfs_find_deepest_element(cursor_pos);
	// 	if(!inbounds.empty())goto upt;
	// }

	// if(auto dialog = dialog_manager.top()){
	// 	if(inbounds.empty()){
	// 		inbounds = dialog->dfs_find_deepest_element(cursor_pos);
	// 	}
	// }else{
	// 	if(inbounds.empty()){
	// 		inbounds = root->dfs_find_deepest_element(cursor_pos);
	// 	}
	//
	// 	if(inbounds.empty()){
	// 		for (auto && activeTooltip : tooltip_manager.get_active_tooltips() | std::views::reverse){
	// 			if(!tooltip_manager.is_below_scene(activeTooltip.element.get()))continue;
	// 			inbounds = activeTooltip.element->dfs_find_deepest_element(cursor_pos);
	// 			if(!inbounds.empty())goto upt;
	// 		}
	// 	}
	// }
	if(inbounds.empty()){
		inbounds = util::dfs_find_deepest_element(&root(), get_cursor_pos(), get_allocator<elem*>());
	}

	upt:

	update_inbounds(std::move(inbounds), force_drop);

	if(!focus_cursor_) return;

	events::drag dragEvent{};
	const auto mode = inputs_.main_binds.get_mode();

	 for(const auto& [i, state] : mouse_states_ | std::views::enumerate){
	 	if(!state.pressed) continue;

	 	dragEvent = {state.src, get_cursor_pos(), events::key_set{static_cast<std::uint16_t>(i), input_handle::act::ignore, mode}};
	 	focus_cursor_->on_drag(dragEvent);
	 }

	 focus_cursor_->on_cursor_moved(events::cursor_move{.src = inputs_.last_cursor_pos(), .dst = inputs_.cursor_pos()});
}

void scene::resize(const math::frect region){
	if(util::try_modify(region_, region)){
		root_->resize(region.extent());
	}
}

void scene::layout(){
	std::size_t count{};
	while(root_->layout_state.is_children_changed() || !independent_layouts_.empty()){
		for(const auto layout : independent_layouts_){
			layout->try_layout();
		}
		independent_layouts_.clear();

		root_->try_layout();

		count++;
		if(count > 8){
			// break;
			throw std::runtime_error("Bad Layout: Iteration Too Many Times");
		}
	}
}

void scene::update_inbounds(std::pmr::vector<elem*>&& next, bool force_drop){
	auto [i1, i2] = std::ranges::mismatch(last_inbounds_, next);

	for(const auto& element : std::ranges::subrange{i1, last_inbounds_.end()} | std::views::reverse){
		element->on_inbound_changed(false, true);
	}

	auto itr = next.begin();
	for(; itr != i2; ++itr){
		(*itr)->on_inbound_changed(true, false);
	}

	for(; itr != next.end(); ++itr){
		(*itr)->on_inbound_changed(true, true);
	}

	last_inbounds_ = std::move(next);

	try_swap_focus(last_inbounds_.empty() ? nullptr : last_inbounds_.back(), force_drop);
}

void scene::update_mouse_state(const input_handle::key_set k){
	using namespace input_handle;
	auto [c, a, m] = k;

	if(a == act::press && focus_key_ && !focus_key_->contains(get_cursor_pos())){
		focus_key_->on_focus_key_changed(false);
		focus_key_ = nullptr;
	}

	if(focus_cursor_){
		const events::click e{get_cursor_pos(), k};

		auto cur = last_inbounds_.rbegin();
		while(cur != last_inbounds_.rend()){
			std::span aboves{std::ranges::next(cur.base()), last_inbounds_.end()};
			if((*cur)->on_click(e, aboves) != events::op_afterwards::intercepted){
				++cur;
			}else{
				break;
			}
		}
	}

	if(a == act::press){
		mouse_states_[c].reset(get_cursor_pos());
	}

	if(a == act::release){
		mouse_states_[c].clear(get_cursor_pos());
	}

	if(focus_cursor_ && focus_cursor_->is_focus_extended_by_mouse()){
		on_cursor_pos_update(false);
	}
}


void scene::try_swap_focus(elem* newFocus, bool force_drop){
	if(newFocus == focus_cursor_) return;

	if(focus_cursor_){
		if(!force_drop && focus_cursor_->is_focus_extended_by_mouse()){
			if(!is_mouse_pressed()){
				swap_focus(newFocus);
			} else return;
		} else{
			swap_focus(newFocus);
		}
	} else{
		swap_focus(newFocus);
	}
}

void scene::swap_focus(elem* newFocus){
	if(focus_cursor_){
		for(auto& state : mouse_states_){
			state.clear(get_cursor_pos());
		}
		focus_cursor_->on_focus_changed(false);
		focus_cursor_->cursor_states_.quit_focus();
	}

	focus_cursor_ = newFocus;

	if(focus_cursor_){
		if(focus_cursor_->is_interactable()){
			focus_cursor_->on_focus_changed(true);
		}
	}
}

void scene::drop_all_focus(const elem* target) noexcept{
	drop_event_focus(target);
	std::erase(last_inbounds_, target);
	// asyncTaskOwners.erase(const_cast<elem*>(target));
	independent_layouts_.erase(const_cast<elem*>(target));
	// erase_independent_draw(target);
	// erase_direct_access({}, target);
	// tooltipManager.requestDrop(*target);
}
}
