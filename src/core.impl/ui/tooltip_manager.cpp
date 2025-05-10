module;

#include <cassert>

module mo_yanxi.ui.basic;

import mo_yanxi.ui.action;
import mo_yanxi.ui.action.generic;
import align;
import mo_yanxi.algo;


void mo_yanxi::ui::tooltip_instance::update_layout(const tooltip_manager& manager){
	assert(owner != nullptr);

	const auto policy = owner->tooltip_align_policy();

	//TODO bound element to the scene region?
	element->context_size_restriction = policy.extent;

	math::vec2 sz{};
	math::vec2 fsz{};
	if(policy.follow == tooltip_follow::dialog){
		auto [fx, fy] = element->property.fill_parent;
		if(fx){
			fsz.x = manager.scene->region.width();
			element->context_size_restriction.width = {size_category::mastering, fsz.x};
		}

		if(fy){
			fsz.y = manager.scene->region.height();
			element->context_size_restriction.height = {size_category::mastering, fsz.y};
		}
	}

	if(element->layout_state.check_any_changed()){
		sz = element->pre_acquire_size(policy.extent).value_or(math::vec2{policy.extent.width, policy.extent.height}.max({60, 60}));
		if(fsz.x == 0)fsz.x = sz.x;
		if(fsz.y == 0)fsz.y = sz.y;
	}



	element->resize(fsz);

	// auto parent_size = policy.follow ==

	// if(.x)
	// if(){
	// 	element->resize(sz.value());
	// }else{
	// }

	element->layout();

	const auto elemOffset = align::get_offset_of(policy.align, element->get_size());
	math::vec2 followOffset{elemOffset};

	switch(policy.follow){
	case tooltip_follow::cursor :{
		followOffset += manager.get_cursor_pos();
		break;
	}

	case tooltip_follow::dialog :{
		followOffset += align::get_vert(policy.align, manager.scene->region);
		break;
	}

	case tooltip_follow::initial_pos :{
		if(!isPosSet()){
			followOffset += manager.get_cursor_pos();
		} else{
			followOffset = last_pos;
		}

		break;
	}
	case tooltip_follow::owner :{
		followOffset += policy.pos.value_or({});

		break;
	}
	default : break;
	}

	last_pos = followOffset;
	element->update_abs_src(followOffset);
}

mo_yanxi::math::vec2 mo_yanxi::ui::tooltip_manager::get_cursor_pos() const noexcept{
	return scene->cursor_pos;
}

mo_yanxi::ui::tooltip_instance& mo_yanxi::ui::tooltip_manager::append_tooltip(
	tooltip_owner& owner,
	bool belowScene,
	bool fade_in){
	auto rst = owner.tooltip_setup(*scene);
	return append_tooltip(owner, std::move(rst), belowScene, fade_in);
}

mo_yanxi::ui::tooltip_instance& mo_yanxi::ui::tooltip_manager::append_tooltip(tooltip_owner& owner, elem_ptr&& elem,
	bool belowScene, bool fade_in){

	if(owner.has_tooltip()){
		owner.tooltip_notify_drop();
	}

	auto& val = actives.emplace_back(std::move(elem), &owner);
	val.update_layout(*this);
	scene->on_cursor_pos_update();

	if(fade_in){
		val.element->update_opacity(0.f);
		val.element->push_action<action::alpha_action>(5, 1.);
	}

	drawSequence.push_back({val.element.get(), belowScene});
	return val;
}

void mo_yanxi::ui::tooltip_manager::update(float delta_in_time){
	assert(scene);
	updateDropped(delta_in_time);

	for (auto&& active : actives){
		active.element->update(delta_in_time);
		active.element->try_layout();
		active.update_layout(*this);
	}

	if(!scene->isMousePressed()){
		const auto lastNotInBound = std::ranges::find_if_not(actives, [this](const tooltip_instance& toolTip){
			if(toolTip.owner->tooltip_should_force_drop(get_cursor_pos()))return false;

			auto follow = toolTip.owner->tooltip_align_policy().follow;

			const bool selfContains = follow != tooltip_follow::cursor && toolTip.element->contains_self(get_cursor_pos(), MarginSize);
			const bool ownerContains = follow != tooltip_follow::initial_pos && toolTip.owner->tooltip_owner_contains(get_cursor_pos());
			return selfContains || ownerContains || toolTip.owner->tooltip_should_maintain(get_cursor_pos());
		});

		dropFrom(lastNotInBound);
	}
}

void mo_yanxi::ui::tooltip_manager::draw_above() const{
	for (auto&& elem : drawSequence){
		if(!elem.belowScene)elem.element->try_draw(scene->region);
	}
}

void mo_yanxi::ui::tooltip_manager::draw_below() const{
	for (auto&& elem : drawSequence){
		if(elem.belowScene)elem.element->try_draw(scene->region);
	}
}

mo_yanxi::ui::esc_flag mo_yanxi::ui::tooltip_manager::on_esc(){
	for (auto&& elem : actives | std::views::reverse){
		if(util::thoroughly_esc(elem.element.get()) != esc_flag::fall_through){
			return esc_flag::intercept;
		}
	}

	if(!actives.empty()){
		dropBack();
		return esc_flag::intercept;
	}

	return esc_flag::fall_through;
}

bool mo_yanxi::ui::tooltip_manager::drop(ActivesItr be, ActivesItr se){
	if(be == se)return false;

	auto range = std::ranges::subrange{be, se};
	for (auto && validToolTip : range){
		validToolTip.owner->tooltip_on_drop();
	}

	dropped.append_range(range | std::ranges::views::as_rvalue | std::views::transform([](tooltip_instance&& validToolTip){
		return DroppedToolTip{std::move(validToolTip.element), RemoveFadeTime};
	}));

	actives.erase(be, se);
	return true;
}

void mo_yanxi::ui::tooltip_manager::updateDropped(float delta_in_time){
	for (auto&& tooltip : dropped){
		tooltip.remainTime -= delta_in_time;
		tooltip.element->update(delta_in_time);
		tooltip.element->update_opacity(tooltip.remainTime / RemoveFadeTime);
		if(tooltip.remainTime <= 0){
			std::erase(drawSequence, tooltip.element.get());
		}
	}

	std::erase_if(dropped, [&](decltype(dropped)::reference dropped){
		return dropped.remainTime <= 0;
	});
}
