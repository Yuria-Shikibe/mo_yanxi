module;

#include <cassert>

module mo_yanxi.gui.infrastructure;

namespace mo_yanxi::gui::tooltip{
void tooltip_instance::update_layout(const tooltip_manager& manager, math::vec2 cursor_pos){
	assert(owner != nullptr);

	const auto policy = owner->tooltip_get_align_config();

	//TODO bound element to the scene region?
	element->restriction_extent = policy.extent;
	if(policy.extent.fully_mastering()){
		element->set_prefer_extent(policy.extent.potential_extent());
	}
	//
	// math::vec2 sz{};
	// math::vec2 fsz{};

	//Legacy
	// if(policy.follow == tooltip_follow::dialog){
	// 	auto [fx, fy] = element->property.fill_parent;
	// 	if(fx){
	// 		fsz.x = manager.scene->get_extent().x;
	// 		element->context_size_restriction.set_width(fsz.x);
	// 	}
	//
	// 	if(fy){
	// 		fsz.y = manager.scene->get_extent().y;
	// 		element->context_size_restriction.set_height(fsz.y);
	// 	}
	// }

	if(element->layout_state.check_any_changed()){
		auto sz = element->pre_acquire_size(policy.extent).value_or(policy.extent.potential_extent().inf_to0().max({60, 60}));
		element->resize(sz);

		element->layout_elem();
	}

	const auto elemOffset = align::get_offset_of(policy.align, element->extent());
	math::vec2 followOffset{elemOffset};

	switch(policy.follow){
	case anchor_type::cursor :{
		followOffset += cursor_pos;
		break;
	}
	//
	// case anchor_type::dialog :{
	// 	followOffset += align::get_vert(policy.align, manager.scene->get_region());
	// 	break;
	// }

	case anchor_type::initial_pos :{
		if(!is_pos_set()){
			followOffset += cursor_pos;
		} else{
			followOffset = last_pos;
		}

		break;
	}

	case anchor_type::owner :{
		followOffset += policy.pos.value_or({});

		break;
	}
	default : break;
	}

	last_pos = followOffset;
	element->update_abs_src(followOffset);
}

tooltip_instance& tooltip_manager::append_tooltip(
	spawner& owner,
	bool belowScene,
	bool fade_in){
	auto rst = owner.tooltip_setup();
	assert(rst != nullptr);
	return append_tooltip(owner, std::move(rst), belowScene, fade_in);
}

tooltip_instance& tooltip_manager::append_tooltip(spawner& owner, elem_ptr&& elem,
	bool belowScene, bool fade_in){

	// if(owner.has_tooltip()){
	// 	owner.tooltip_notify_drop();
	// }

	auto& scene = owner.tooltip_get_scene();
	auto& val = actives.emplace_back(std::move(elem), &owner);
	val.update_layout(*this, scene.get_cursor_pos());
	scene.update_cursor(/*true*/);

	// TODO action
	// if(fade_in){
	// 	val.element->update_opacity(0.f);
	// 	val.element->push_action<action::alpha_action>(5, 1.);
	// }

	drawSequence.push_back({val.element.get(), belowScene});
	return val;
}

tooltip_instance* tooltip_manager::try_append_tooltip(spawner& owner, bool belowScene, bool fade_in){
	if(!owner.has_tooltip() && owner.tooltip_should_build(owner.tooltip_get_scene().get_cursor_pos())){
		return &append_tooltip(owner, belowScene, fade_in);
	}
	return nullptr;
}

void tooltip_manager::update(
	float delta_in_time,
	math::vec2 cursor_pos,
	bool is_mouse_pressed){
	updateDropped(delta_in_time);

	for (auto&& active : actives){
		active.element->update(delta_in_time);
		active.element->try_layout();
		active.update_layout(*this, cursor_pos);
	}

	if(!is_mouse_pressed){
		const auto lastNotInBound = std::ranges::find_if_not(actives, [&, this](const tooltip_instance& toolTip){
			if(toolTip.owner->tooltip_should_drop(cursor_pos))return false;

			const auto follow = toolTip.owner->tooltip_get_align_config().follow;

			const bool selfContains = follow != anchor_type::cursor && toolTip.element->contains_self(cursor_pos, MarginSize);
			const bool ownerContains = follow != anchor_type::initial_pos && toolTip.owner->tooltip_spawner_contains(cursor_pos);
			return selfContains || ownerContains || toolTip.owner->tooltip_should_maintain(cursor_pos);
		});

		drop_since(lastNotInBound);
	}
}

events::op_afterwards tooltip_manager::on_esc(){
	for (auto&& elem : actives | std::views::reverse){
		if(util::thoroughly_esc(elem.element.get()) != events::op_afterwards::fall_through){
			return events::op_afterwards::intercepted;
		}
	}

	if(!actives.empty()){
		drop_back();
		return events::op_afterwards::intercepted;
	}

	return events::op_afterwards::fall_through;
}

bool tooltip_manager::drop(ActivesItr be, ActivesItr se){
	if(be == se)return false;

	auto range = std::ranges::subrange{be, se};
	for (auto && validToolTip : range){
		validToolTip.owner->tooltip_notify_drop();
	}

	dropped.append_range(range | std::ranges::views::as_rvalue | std::views::transform([](tooltip_instance&& validToolTip){
		return tooltip_expired{std::move(validToolTip.element), RemoveFadeTime};
	}));

	actives.erase(be, se);
	return true;
}

void tooltip_manager::updateDropped(float delta_in_time){
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


void spawner::tooltip_drop(){
	//TODO
}

}