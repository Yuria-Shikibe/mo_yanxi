module;

//TODO this should be the impl file of :scene
//Anyway the highlight breakdown when using module mo_yanxi.ui.basic:scene;
module mo_yanxi.ui.basic;


import mo_yanxi.concepts;
import mo_yanxi.algo;

import mo_yanxi.core.global;
import mo_yanxi.core.global.graphic;

import mo_yanxi.graphic.renderer.ui;

import mo_yanxi.core.platform;
import mo_yanxi.owner;

std::string_view mo_yanxi::ui::scene::get_clipboard() const noexcept{
	return core::plat::get_clipboard(core::global::graphic::context.window().get_handle());
}

void mo_yanxi::ui::scene::set_clipboard(const char* sv) const noexcept{
	core::plat::set_clipborad(core::global::graphic::context.window().get_handle(), sv);
}

mo_yanxi::core::ctrl::key_code_t mo_yanxi::ui::scene::get_input_mode() const noexcept{
	return core::global::input.main_binds.get_mode();
}

void mo_yanxi::ui::scene::drop_dialog(const elem* elem){
	dialog_manager.truncate(elem);
}

void mo_yanxi::ui::scene::root_draw() const{
	renderer->batch.push_projection(rect{tags::from_extent, math::vec2{}, region.size()});
	renderer->batch.push_viewport(region);

	draw(region);

	renderer->batch.pop_viewport();
	renderer->batch.pop_projection();
}

double mo_yanxi::ui::scene::get_global_time() const noexcept{
	return core::global::timer.global_time();
}





mo_yanxi::ui::scene::scene(
	const std::string_view name,
	const owner<basic_group*> root,
	graphic::renderer_ui* renderer) :
	scene_base{name, root, renderer}{

	// keyMapping = &core::input.register_sub_input(name);
	tooltip_manager.scene = this;
	dialog_manager.scene_ = this;

	if(!root)return;
	root->skip_inbound_capture = true;
	root->interactivity = interactivity::children_only;
	root->set_scene(this);
}

mo_yanxi::ui::scene::~scene(){
	// if(keyMapping)Global::input->eraseSubInput(name);
	if(root_ownership)delete root.handle;
	else root->set_scene(nullptr);
}

// void mo_yanxi::ui::scene::setIMEPos(Geom::Point2 pos) const{
// 	pos += this->pos.as<int>();
// 	Core::Spec::setInputMethodEditor(Core::Global::window->getNative(), pos.x, size.y - pos.y);
// }

void mo_yanxi::ui::scene::registerAsyncTaskElement(elem* element){
	asyncTaskOwners.insert(element);
}

void mo_yanxi::ui::scene::notify_layout_update(elem* element){
	independentLayout.insert(element);
}





void mo_yanxi::ui::scene::drop_all_focus(const elem* target){
	drop_event_focus(target);
	std::erase(lastInbounds, target);
	asyncTaskOwners.erase(const_cast<elem*>(target));
	independentLayout.erase(const_cast<elem*>(target));
	// tooltipManager.requestDrop(*target);
}

void mo_yanxi::ui::scene::try_swap_focus(elem* newFocus, bool force_drop){
	if(newFocus == currentCursorFocus) return;

	if(currentCursorFocus){
		if(!force_drop && currentCursorFocus->maintain_focus_by_mouse()){
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

void mo_yanxi::ui::scene::swap_focus(elem* newFocus){
	if(currentCursorFocus){
		for(auto& state : mouseKeyStates){
			state.clear(cursor_pos);
		}
		currentCursorFocus->events().fire(input_event::focus_end{cursor_pos});
		currentCursorFocus->on_focus_changed(false);
		currentCursorFocus->cursor_state.quit_focus();
	}

	currentCursorFocus = newFocus;

	if(currentCursorFocus){
		if(currentCursorFocus->interactable()){
			currentCursorFocus->events().fire(input_event::focus_begin{cursor_pos});
			currentCursorFocus->on_focus_changed(true);
		}
	}
}

mo_yanxi::ui::esc_flag mo_yanxi::ui::scene::on_esc(){
	// return true;
	if(tooltip_manager.on_esc() != esc_flag::fall_through)return esc_flag::intercept;
	if(dialog_manager.on_esc() != esc_flag::fall_through)return esc_flag::intercept;
	//
	elem* focus = currentKeyFocus;
	if(!focus) focus = currentCursorFocus;

	return util::thoroughly_esc(focus);
}

void mo_yanxi::ui::scene::on_mouse_action(const core::ctrl::key_code_t key, const core::ctrl::key_code_t action, const core::ctrl::key_code_t mode){
	if(action == core::ctrl::act::press && currentKeyFocus && !currentKeyFocus->contains(cursor_pos)){
		currentKeyFocus = nullptr;
	}

	if(currentCursorFocus){
		const input_event::click e{cursor_pos, core::ctrl::key_pack{key, action, mode}};

		if(currentCursorFocus->on_click(e) != input_event::click_result::intercepted){
			auto cur = lastInbounds.rbegin();
			while(cur != lastInbounds.rend()){
				if(*cur == currentCursorFocus){
					++cur;
					continue;
				}

				if((*cur)->on_click(e) != input_event::click_result::intercepted){
					++cur;
				}else{
					break;
				}
			}
		}
	}

	if(action == core::ctrl::act::press){
		mouseKeyStates[key].reset(cursor_pos);
	}

	if(action == core::ctrl::act::release){
		mouseKeyStates[key].clear(cursor_pos);
	}

	if(currentCursorFocus && currentCursorFocus->maintain_focus_by_mouse()){
		on_cursor_pos_update(cursor_pos, false);
	}
}

void mo_yanxi::ui::scene::on_key_action(const core::ctrl::key_code_t key, const core::ctrl::key_code_t action, const core::ctrl::key_code_t mode){

	if(action == core::ctrl::act::press && key == core::ctrl::key::Esc){
		if(on_esc() == esc_flag::fall_through){
			on_cursor_pos_update(cursor_pos, true);
		}
	}else{
		elem* focus = currentKeyFocus;
		if(!focus) focus = currentCursorFocus;
		if(!focus) return;
		focus->input_key(key, action, mode);
	}
}

void mo_yanxi::ui::scene::on_unicode_input(char32_t val) const{
	if(currentKeyFocus){
		currentKeyFocus->input_unicode(val);
	}
}

void mo_yanxi::ui::scene::on_scroll(const math::vec2 scroll, core::ctrl::key_code_t mode) const{
	if(currentScrollFocus){
		currentScrollFocus->events().fire(input_event::scroll{scroll, mode});
		currentScrollFocus->on_scroll(input_event::scroll{scroll, mode});
	}
}

void mo_yanxi::ui::scene::on_cursor_pos_update(const math::vec2 newPos, bool force_drop){
	const auto delta = newPos - cursor_pos;
	cursor_pos = newPos;

	std::vector<elem*> inbounds{};


	for (auto && activeTooltip : tooltip_manager.get_active_tooltips() | std::views::reverse){
		if(tooltip_manager.is_below_scene(activeTooltip.element.get()))continue;
		inbounds = activeTooltip.element->dfs_find_deepest_element(cursor_pos);
		if(!inbounds.empty())goto upt;
	}

	if(auto dialog = dialog_manager.top()){
		if(inbounds.empty()){
			inbounds = dialog->dfs_find_deepest_element(cursor_pos);
		}
	}else{
		if(inbounds.empty()){
			inbounds = root->dfs_find_deepest_element(cursor_pos);
		}

		if(inbounds.empty()){
			for (auto && activeTooltip : tooltip_manager.get_active_tooltips() | std::views::reverse){
				if(!tooltip_manager.is_below_scene(activeTooltip.element.get()))continue;
				inbounds = activeTooltip.element->dfs_find_deepest_element(cursor_pos);
				if(!inbounds.empty())goto upt;
			}
		}
	}


	upt:

	updateInbounds(std::move(inbounds), force_drop);

	if(!currentCursorFocus) return;

	input_event::drag dragEvent{};
	const auto mode = get_input_mode();

	for(const auto& [i, state] : mouseKeyStates | std::views::enumerate){
		if(!state.pressed) continue;

		dragEvent = {state.src, newPos, core::ctrl::key_pack{static_cast<core::ctrl::key_code_t>(i), core::ctrl::act::ignore, mode}};
		currentCursorFocus->events().fire(dragEvent);
		currentCursorFocus->on_drag(dragEvent);
	}

	currentCursorFocus->notify_cursor_moved(delta);
}

void mo_yanxi::ui::scene::resize(const math::frect region){
	if(util::try_modify(this->region, region)){
		// root->update_abs_src({});
		root->resize(region.size());
	}
}

void mo_yanxi::ui::scene::update(const float delta_in_ticks){
	tooltip_manager.update(delta_in_ticks);
	dialog_manager.update(delta_in_ticks);
	root->update(delta_in_ticks);
}


// void mo_yanxi::ui::scene::setCameraFocus(Graphic::Camera2D* camera) const{
// 	Core::Global::Focus::camera.switchFocus(camera);
// }

bool mo_yanxi::ui::scene::is_cursor_captured() const noexcept{
	return !lastInbounds.empty();
}

mo_yanxi::ui::elem* mo_yanxi::ui::scene::getCursorCaptureRoot() const noexcept{
	if(lastInbounds.empty())return nullptr;
	return lastInbounds.front();
}

void mo_yanxi::ui::scene::layout(){
	std::size_t count{};
	while(root->layout_state.is_children_changed() || !independentLayout.empty()){
		// root->tryLayout();

		for(const auto layout : independentLayout){
			layout->try_layout();
		}
		independentLayout.clear();

		root->try_layout();
		for (const auto & dialog : dialog_manager.dialogs){
			dialog.elem->try_layout();
		}

		count++;
		if(count > 8){
			// break;
			throw std::runtime_error("Bad Layout: Iteration Too Many Times");
		}
	}

	// if(count)std::println(std::cerr, "{}", count);
}

void mo_yanxi::ui::scene::draw(math::frect clipSpace) const{
	//TODO draw and blit?

	//TODO fix blit bound(apply viewport transform) when draw nested scene

	constexpr auto to_uint_bound = [](rect region) static  {
		return region.expand(8).round<std::int32_t>().max_src({}).as<std::uint32_t>();
	};

	if(dialog_manager.empty()){
		for (auto&& elem : tooltip_manager.get_draw_sequence()){
			if(elem.belowScene){
				elem.element->try_draw(clipSpace);
				renderer->batch->consume_all();
				renderer->batch.blit_viewport(elem.element->get_bound());
			}
		}

		for (auto& elem : root->get_children()){
			elem->try_draw(clipSpace);
			renderer->batch->consume_all();
			renderer->batch.blit_viewport(elem->get_bound());
		}
	}else{
		dialog_manager.draw_all(clipSpace);
	}


	for (auto&& elem : tooltip_manager.get_draw_sequence()){
		if(!elem.belowScene){
			elem.element->try_draw(clipSpace);
			renderer->batch->consume_all();
			renderer->batch.blit_viewport(elem.element->get_bound());
		}
	}

	renderer->batch->consume_all();
}

mo_yanxi::ui::scene::scene(scene&& other) noexcept:
	scene_base{std::move(other)}/*,
	tooltipManager{std::move(other.tooltipManager)}*/{

	tooltip_manager.scene = this;
	dialog_manager.scene_ = this;

	root->set_scene(this);
}

mo_yanxi::ui::scene& mo_yanxi::ui::scene::operator=(scene&& other) noexcept{
	if(this == &other) return *this;
	scene_base::operator =(std::move(other));
	// tooltipManager = std::move(other.tooltipManager);

	tooltip_manager.scene = this;
	dialog_manager.scene_ = this;

	if(root)root->set_scene(this);
	return *this;
}

void mo_yanxi::ui::scene::updateInbounds(std::vector<elem*>&& next, bool force_drop){
	auto [i1, i2] = std::ranges::mismatch(lastInbounds, next);

	for(const auto& element : std::ranges::subrange{i1, lastInbounds.end()}){
		element->events().fire(input_event::exbound{cursor_pos});
	}

	// for(const auto& element : std::ranges::subrange{i2, next.end()}){
	for(const auto& element : next){
		element->events().fire(input_event::inbound{cursor_pos});
	}

	lastInbounds = std::move(next);

	try_swap_focus(lastInbounds.empty() ? nullptr : lastInbounds.back(), force_drop);
}

