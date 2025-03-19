module;

#include <cassert>

module mo_yanxi.ui.elem;

import mo_yanxi.ui.pre_decl;
import mo_yanxi.ui.scene;
import mo_yanxi.ui.group;
import mo_yanxi.ui.util;
import mo_yanxi.ui.graphic;
import mo_yanxi.graphic.renderer.ui;
import mo_yanxi.graphic.draw;
// import Graphic.Renderer.UI;

//
// float mo_yanxi::ui::EmptyElementDrawer::contentOpacity(const struct elem& element) const{
// 	if(element.disabled){
// 		return .55f;
// 	}
//
// 	return 1.f;
// }

namespace mo_yanxi{
	void ui::debug_elem_drawer::draw(const elem& element, rect region, float opacityScl) const{
		using namespace graphic;
		draw_acquirer acquirer{element.get_renderer().batch, graphic::draw::white_region};
		draw::line::rect_ortho(acquirer, region);

		if(element.cursor_state.focused){
			draw::fill::rect_ortho(acquirer.get(), region, colors::white.copy().mulA(.35f));
		}
	}

	void ui::cursor_states::register_default_cursor_events(elem_event_manager& event_manager){
		event_manager.on<events::focus_begin>([](auto, elem& self){
			self.cursor_state.focused = true;
		});

		event_manager.on<events::focus_end>([](auto, elem& self){
			self.cursor_state.focused = false;
			// stagnateTime = focusedTime = 0.f;
		});

		event_manager.on<events::moved>([](auto, elem& self){
			self.cursor_state.time_stagnate = 0;
			if(self.get_tooltip_prop().use_stagnate_time){
				self.cursor_state.time_tooltip = 0.;
				self.tooltip_notify_drop();
			}
		});
	}


	graphic::renderer_ui& ui::elem::get_renderer() const noexcept{
		return *get_scene()->renderer; //fuck fake positive...
	}

	ui::group* ui::elem::get_root_parent() const noexcept{
		group* cur = get_parent();
		if(!cur) return nullptr;

		while(true){
			if(const auto next = cur->get_parent()){
				cur = next;
			} else{
				break;
			}
		}
		return cur;
	}

	void ui::elem::update_opacity(const float val){
		if(util::tryModify(property.graphicData.context_opacity, val)){
			for(const auto& element : get_children()){
				element->update_opacity(graphicProp().get_opacity());
			}
		}
	}

	void ui::elem::mark_independent_layout_changed(){
		layoutState.notify_self_changed();
		get_scene()->independentLayout.insert(this);
	}

	math::vec2 ui::elem::get_local_from_global(math::vec2 global_input) const noexcept{
		std::vector parents{this};
		while(parents.back()){
			parents.push_back(parents.back()->get_parent());
		}

		for(const auto parent : parents | std::views::reverse){
			global_input = parent->transform_pos(global_input);
		}

		return global_input;
	}

	void ui::elem::remove_self_from_parent(){
		assert(!is_root_element());

		parent->postRemove(this);
	}

	void ui::elem::remove_self_from_parent_instantly(){
		assert(!is_root_element());

		parent->instantRemove(this);
	}

	void ui::elem::registerAsyncTask(){
	}

	void ui::elem::notify_remove(){
		assert(parent != nullptr);
		clear_external_references();
		parent->postRemove(this);
	}

	void ui::elem::clear_external_references() noexcept{
		if(scene_){
			scene_->dropAllFocus(this);
			scene_ = nullptr;
		}
	}

	bool ui::elem::is_focused_scroll() const noexcept{
		return scene_ && scene_->currentScrollFocus == this;
	}

	bool ui::elem::is_focused_key() const noexcept{
		return scene_ && scene_->currentKeyFocus == this;
	}

	bool ui::elem::is_focused() const noexcept{
		return scene_ && scene_->currentCursorFocus == this;
	}

	bool ui::elem::is_inbounded() const noexcept{
		return scene_ && std::ranges::contains(scene_->lastInbounds, this);
	}

	void ui::elem::set_focused_scroll(const bool focus) noexcept{
		if(!focus && !is_focused_scroll()) return;
		this->scene_->currentScrollFocus = focus ? this : nullptr;
	}

	void ui::elem::set_focused_key(const bool focus) noexcept{
		if(!focus && !is_focused_key()) return;
		this->scene_->currentKeyFocus = focus ? this : nullptr;
	}

	bool ui::elem::contains(const math::vec2 absPos) const noexcept{
		return (!parent || parent->contains_parent(absPos)) && contains_self(absPos, 0.);
	}

	bool ui::elem::contains_self(const math::vec2 absPos, const float margin) const noexcept{
		return property.bound_absolute().expand(margin, margin).contains_loose(absPos);
	}

	// void ui::elem::buildTooltip(const bool belowScene) noexcept{
	// 	()->tooltipManager.appendToolTip(*this, belowScene);
	// }

	bool ui::elem::contains_parent(const math::vec2 cursorPos) const{
		return (!parent || parent->contains_parent(cursorPos));
	}

	void ui::elem::notify_layout_changed(spread_direction toDirection){
		if(toDirection & spread_direction::local) layoutState.notify_self_changed();

		if(parent){
			if(toDirection & spread_direction::super || toDirection & spread_direction::from_content){
				if(parent->layoutState.notify_children_changed(toDirection & spread_direction::from_content)){
					parent->notify_layout_changed(toDirection - spread_direction::child);
				}
			}
		}

		if(toDirection & spread_direction::child){
			for(auto&& element : get_children()){
				if(element->layoutState.notify_parent_changed()){
					element->notify_layout_changed(toDirection - spread_direction::super);
				}
			}
		}
	}

	void ui::elem::update(const float delta_in_ticks){
		if(checkers.disableProv)[[unlikely]] {
			disabled = checkers.disableProv(*this);
		}

		if(checkers.visibilityChecker)[[unlikely]] {
			visible = checkers.visibilityChecker(*this);
		}

		if(checkers.activatedChecker)[[unlikely]] {
			activated = checkers.activatedChecker(*this);
		}

		cursor_state.update(delta_in_ticks);

		if(cursor_state.focused){
			//TODO dependent above?
			get_scene()->tooltip_manager.try_append_tooltip(*this, false);
		}

		for(float actionDelta = delta_in_ticks; !actions.empty();){
			const auto& current = actions.front();

			actionDelta = current->update(actionDelta, *this);

			if(actionDelta >= 0) [[unlikely]] {
				actions.pop();
			} else{
				break;
			}
		}
	}

	// void ui::elem::drawMain(const Rect clipSpace) const{
	// 	property.graphicData.drawer->draw(*this, getBound(), 1.);
	// }
	//
	// std::optional<math::vec2> ui::elem::getFitnessSize() const noexcept{
	// 	if(parent){
	// 		return parent->getElementFitnessSize(*this);
	// 	}
	//
	// 	return std::nullopt;
	// }

	// ui::TooltipAlignPos ui::elem::getAlignPolicy() const{
	// 	switch(tooltipProp.followTarget){
	// 	case TooltipFollow::depends :{
	// 		return {
	// 				Align::getVert(tooltipProp.followTargetAlign, getBound()), std::nullopt, tooltipProp.tooltipSrcAlign
	// 			};
	// 	}
	// 	case TooltipFollow::sceneDialog :{
	// 		auto bound = getScene()->getBound();
	// 		return {Align::getVert(tooltipProp.followTargetAlign, bound), bound.getSize(), tooltipProp.tooltipSrcAlign};
	// 	}
	//
	// 	default : return {tooltipProp.followTarget, std::nullopt, tooltipProp.tooltipSrcAlign};
	// 	}
	// }
	//

	//
	// void ui::elem::dropToolTipIfMoved() const{
	// 	if(tooltipProp.useStagnateTime) getScene()->tooltipManager.requestDrop(*this);
	// }

	void ui::elem::draw_pre(const rect clipSpace, rect redirect) const{
		if(property.graphicData.drawer)property.graphicData.drawer->draw(*this, get_bound(), property.graphicData.get_opacity());
	}

	bool ui::elem::update_abs_src(const math::vec2 parent_content_abs_src){
		if(util::tryModify(property.absolute_src, parent_content_abs_src + property.relative_src)){
			for(const auto& element : get_children()){
				element->update_abs_src(content_src_pos());
			}
			return true;
		}
		return false;
	}

	std::vector<ui::elem*> ui::elem::dfs_find_deepest_element(math::vec2 cursorPos){
		std::vector<elem*> rst{};

		iterateAll_DFSImpl(cursorPos, rst, this);

		return rst;
	}

	bool ui::elem::tooltip_should_maintain(math::vec2 cursorPos) const{
		assert(tooltip_handle.handle);
		return !tooltip_prop_.auto_release || tooltip_handle.handle->is_focused_key();
	}

	void ui::iterateAll_DFSImpl(math::vec2 cursorPos, std::vector<elem*>& selected, elem* current){
		if(!current->ignore_inbound() && current->contains(cursorPos)){
			selected.push_back(current);
		}

		if(current->touch_disabled() || !current->has_children()) return;

		auto transformed = current->transform_pos(cursorPos);

		for(const auto& child : current->get_children() | std::views::reverse){
			iterateAll_DFSImpl(transformed, selected, child.get());
		}
	}
}
