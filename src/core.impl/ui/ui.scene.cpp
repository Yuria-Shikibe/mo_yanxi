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
	const owner<group*> root,
	graphic::renderer_ui* renderer) :
	scene_base{name, root, renderer}{

	// keyMapping = &core::input.register_sub_input(name);
	tooltip_manager.scene = this;

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

void mo_yanxi::ui::scene::notifyLayoutUpdate(elem* element){
	independentLayout.insert(element);
}






void mo_yanxi::ui::scene::dropAllFocus(const elem* target){
	dropEventFocus(target);
	std::erase(lastInbounds, target);
	asyncTaskOwners.erase(const_cast<elem*>(target));
	independentLayout.erase(const_cast<elem*>(target));
	// tooltipManager.requestDrop(*target);
}

void mo_yanxi::ui::scene::trySwapFocus(elem* newFocus){
	if(newFocus == currentCursorFocus) return;

	if(currentCursorFocus){
		if(currentCursorFocus->maintain_focus_by_mouse()){
			if(!isMousePressed()){
				swapFocus(newFocus);
			} else return;
		} else{
			swapFocus(newFocus);
		}
	} else{
		swapFocus(newFocus);
	}
}

void mo_yanxi::ui::scene::swapFocus(elem* newFocus){
	if(currentCursorFocus){
		for(auto& state : mouseKeyStates){
			state.clear(cursor_pos);
		}
		currentCursorFocus->events().fire(events::focus_end{cursor_pos});
	}

	currentCursorFocus = newFocus;

	if(currentCursorFocus){
		if(currentCursorFocus->interactable())currentCursorFocus->events().fire(events::focus_begin{cursor_pos});
	}
}

mo_yanxi::ui::esc_flag mo_yanxi::ui::scene::on_esc(){
	// return true;
	if(tooltip_manager.on_esc() != esc_flag::droppable)return esc_flag::sustain;
	//
	elem* focus = currentKeyFocus;
	if(!focus) focus = currentCursorFocus;
	if(!focus)return esc_flag::droppable;
	else return focus->on_esc();
}

void mo_yanxi::ui::scene::on_mouse_action(const core::ctrl::key_code_t key, const core::ctrl::key_code_t action, const core::ctrl::key_code_t mode){
	if(action == core::ctrl::act::press && currentKeyFocus && !currentKeyFocus->contains(cursor_pos)){
		currentKeyFocus = nullptr;
	}

	if(currentCursorFocus)
		currentCursorFocus->events().fire(
			events::click{cursor_pos, core::ctrl::key_pack{key, action, mode}}
		);

	if(action == core::ctrl::act::press){
		mouseKeyStates[key].reset(cursor_pos);
	}

	if(action == core::ctrl::act::release){
		mouseKeyStates[key].clear(cursor_pos);
	}

	if(currentCursorFocus && currentCursorFocus->maintain_focus_by_mouse()){
		on_cursor_pos_update(cursor_pos);
	}
}

void mo_yanxi::ui::scene::on_key_action(const core::ctrl::key_code_t key, const core::ctrl::key_code_t action, const core::ctrl::key_code_t mode){
	elem* focus = currentKeyFocus;
	if(!focus) focus = currentCursorFocus;
	if(!focus) return;
	focus->input_key(key, action, mode);
	if(action == core::ctrl::act::press){
		if(key == core::ctrl::key::Esc){
			(void)on_esc();
		}
	}

}

void mo_yanxi::ui::scene::on_unicode_input(char32_t val) const{
	if(currentKeyFocus){
		currentKeyFocus->input_unicode(val);
	}
}

void mo_yanxi::ui::scene::on_scroll(const math::vec2 scroll, core::ctrl::key_code_t mode) const{
	if(currentScrollFocus){
		currentScrollFocus->events().fire(events::scroll{scroll, mode});
	}
}

void mo_yanxi::ui::scene::on_cursor_pos_update(const math::vec2 newPos){
	const auto delta = newPos - cursor_pos;
	cursor_pos = newPos;

	std::vector<elem*> inbounds{};

	for (auto && activeTooltip : tooltip_manager.get_active_tooltips() | std::views::reverse){
		if(tooltip_manager.is_below_scene(activeTooltip.element.get()))continue;
		inbounds = activeTooltip.element->dfs_find_deepest_element(cursor_pos);
		if(!inbounds.empty())goto upt;
	}

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

	upt:

	updateInbounds(std::move(inbounds));

	if(!currentCursorFocus) return;

	events::drag dragEvent{};
	const auto mode = get_input_mode();

	for(const auto& [i, state] : mouseKeyStates | std::views::enumerate){
		if(!state.pressed) continue;

		dragEvent = {state.src, newPos, core::ctrl::key_pack{static_cast<core::ctrl::key_code_t>(i), core::ctrl::act::ignore, mode}};
		currentCursorFocus->events().fire(dragEvent);
	}

	currentCursorFocus->notify_cursor_moved(delta);
}

void mo_yanxi::ui::scene::resize(const math::frect region){
	if(util::tryModify(this->region, region)){
		// root->update_abs_src({});
		root->resize(region.size());
	}
}

void mo_yanxi::ui::scene::update(const float delta_in_ticks){
	tooltip_manager.update(delta_in_ticks);
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

	for (auto&& elem : tooltip_manager.get_draw_sequence()){
		if(elem.belowScene){
			elem.element->try_draw(clipSpace);
		}
	}

	root->draw(clipSpace);

	for (auto&& elem : tooltip_manager.get_draw_sequence()){
		if(!elem.belowScene){
			elem.element->try_draw(clipSpace);
		}
	}
}

mo_yanxi::ui::scene::scene(scene&& other) noexcept:
	scene_base{std::move(other)}/*,
	tooltipManager{std::move(other.tooltipManager)}*/{

	tooltip_manager.scene = this;
	root->set_scene(this);
}

mo_yanxi::ui::scene& mo_yanxi::ui::scene::operator=(scene&& other) noexcept{
	if(this == &other) return *this;
	scene_base::operator =(std::move(other));
	// tooltipManager = std::move(other.tooltipManager);

	tooltip_manager.scene = this;
	if(root)root->set_scene(this);
	return *this;
}

void mo_yanxi::ui::scene::updateInbounds(std::vector<elem*>&& next){
	auto [i1, i2] = std::ranges::mismatch(lastInbounds, next);

	for(const auto& element : std::ranges::subrange{i1, lastInbounds.end()}){
		element->events().fire(events::exbound{cursor_pos});
	}

	// for(const auto& element : std::ranges::subrange{i2, next.end()}){
	for(const auto& element : next){
		element->events().fire(events::inbound{cursor_pos});
	}

	lastInbounds = std::move(next);

	trySwapFocus(lastInbounds.empty() ? nullptr : lastInbounds.back());
}

