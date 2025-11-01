module;

#include <cassert>

module mo_yanxi.gui.infrastructure;

import mo_yanxi.graphic.draw.instruction;

namespace mo_yanxi::gui{
void elem::draw_background() const{
	const auto region = abs_bound();
	get_scene().renderer().draw(graphic::draw::instruction::rectangle_ortho_outline{
		.v00 = region.vert_00(),
		.v11 = region.vert_11(),
		.stroke = 2,
		.vert_color = graphic::colors::light_gray
	});
}

void elem::update(float delta_in_ticks){
	cursor_states_.update(delta_in_ticks);


}

void elem::clear_scene_references() noexcept{
	assert(scene_ != nullptr);
	scene_->drop_all_focus(this);
}

std::optional<math::vec2> elem::pre_acquire_size_no_boarder_clip(const layout::optional_mastering_extent extent){
	return pre_acquire_size_impl(extent).transform([&, this](const math::vec2 v){
		return size_.clamp(v + boarder_.extent()).min(extent.potential_extent());
	});
}

std::optional<math::vec2> elem::pre_acquire_size(const layout::optional_mastering_extent extent){
	return pre_acquire_size_impl(clip_boarder_from(extent, boarder_)).transform([&, this](const math::vec2 v){
		return size_.clamp(v + boarder_.extent()).min(extent.potential_extent());
	});
}

void elem::notify_layout_changed(propagate_mask propagation) noexcept{
	if(check_propagate_satisfy(propagation, propagate_mask::local)) layout_state.notify_self_changed();

	if(parent_){
		const bool force_upper = check_propagate_satisfy(propagation, propagate_mask::force_upper);
		if(force_upper || (check_propagate_satisfy(propagation, propagate_mask::super) && layout_state.is_broadcastable(propagate_mask::super))){
			if(parent_->layout_state.notify_children_changed(force_upper)){
				if(parent_->layout_state.intercept_lower_to_isolated){
					parent_->notify_isolated_layout_changed();
				}else{
					parent_->notify_layout_changed(propagation - propagate_mask::child);
				}
			}
		}
	}

	if(check_propagate_satisfy(propagation, propagate_mask::child) && layout_state.is_broadcastable(propagate_mask::child)){
		for(auto&& element : children()){
			if(element->layout_state.notify_parent_changed()){
				element->notify_layout_changed(propagation - propagate_mask::super);
			}
		}
	}
}

void elem::notify_isolated_layout_changed(){
	get_scene().notify_isolated_layout_update(this);
}

bool elem::contains(const math::vec2 absPos) const noexcept{
	return abs_bound().contains_loose(absPos) && (!parent() || parent()->parent_contain_constrain(absPos));
}

bool elem::contains_self(const math::vec2 absPos, const float margin) const noexcept{
	return abs_bound().expand(margin, margin).contains_loose(absPos);
}

bool elem::parent_contain_constrain(const math::vec2 cursorPos) const noexcept{
	return (!parent() || parent()->parent_contain_constrain(cursorPos));
}

bool elem::is_focused_scroll() const noexcept{
	assert(scene_ != nullptr);
	return scene_->focus_cursor_ == this;

}

bool elem::is_focused_key() const noexcept{
	assert(scene_ != nullptr);
	return scene_->focus_key_ == this;
}

bool elem::is_focused() const noexcept{
	assert(scene_ != nullptr);
	return scene_->focus_cursor_ == this;
}

bool elem::is_inbounded() const noexcept{
	assert(scene_ != nullptr);
	return std::ranges::contains(scene_->last_inbounds_, this);
}

void elem::set_focused_scroll(const bool focus) noexcept{
	if(!focus && !is_focused_scroll()) return;
	this->scene_->focus_cursor_ = focus ? this : nullptr;
}

void elem::set_focused_key(const bool focus) noexcept{
	if(!focus && !is_focused_key()) return;
	this->scene_->focus_key_ = focus ? this : nullptr;
}

void elem::reset_scene(struct gui::scene* scene) noexcept{
	scene_ = scene;
	for (auto && child : children()){
		child->reset_scene(scene);
	}
}
}
