module;

#include <cassert>

module mo_yanxi.ui.tooltip_manager;

import mo_yanxi.ui.group;
import mo_yanxi.ui.scene;
import mo_yanxi.ui.elem;
import mo_yanxi.ui.action;
import mo_yanxi.ui.action.graphic;
import align;
import mo_yanxi.algo;

namespace mo_yanxi::ui{




	// bool ToolTipManager::onEsc(){
	// 	for (auto&& elem : actives | std::views::reverse){
	// 		if(!elem.element->onEsc()){
	// 			return false;
	// 		}
	// 	}
	//
	// 	if(!actives.empty()){
	// 		dropBack();
	// 		return false;
	// 	}
	//
	// 	return true;
	// }


}

// void mo_yanxi::ui::tooltip_instance::drop() const{
// 	if(element && owner){
// 		element->getScene()->tooltipManager.requestDrop(owner);
// 	}
// }
//
// void mo_yanxi::ui::tooltip_instance::drop() const{
// }

void mo_yanxi::ui::tooltip_instance::updatePosition(const tooltip_manager& manager){
	assert(owner != nullptr);

	// constexpr Align::Spacing spacing{0, 0, 0, 0};

	const auto policy = owner->tooltip_align_policy();

	//TODO bound element to the scene region?
	element->context_size_restriction = policy.extent;

	if(auto sz = element->pre_acquire_size(policy.extent)){
		element->resize(sz.value());
	}else{
		element->resize(math::vec2{policy.extent.width, policy.extent.height}.max({60, 60}));
	}

	element->try_layout();

	const auto elemOffset = align::get_offset_of(policy.align, element->get_size());
	math::vec2 followOffset{elemOffset};

	std::visit([&] <typename T> (const T& val){
		if constexpr (std::same_as<T, math::vec2>){
			followOffset += val;
		} else if constexpr(std::same_as<T, tooltip_follow>){
			switch(val){
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
					}else{
						followOffset = last_pos;
					}

					break;
				}

				default: std::unreachable();
			}
		}
	}, policy.pos);


	last_pos = followOffset;
	element->update_abs_src(followOffset);
}

mo_yanxi::math::vec2 mo_yanxi::ui::tooltip_manager::get_cursor_pos() const noexcept{
	return scene->cursorPos;
}

mo_yanxi::ui::tooltip_instance& mo_yanxi::ui::tooltip_manager::append_tooltip(
	tooltip_owner& owner,
	bool belowScene,
	bool fade_in){
	auto rst = owner.tooltip_setup(*scene);
	auto& val = actives.emplace_back(std::move(rst), &owner);
	val.updatePosition(*this);
	scene->on_cursor_pos_update();

	drawSequence.push_back({val.element.get(), belowScene});
	return val;
}

void mo_yanxi::ui::tooltip_manager::update(float delta_in_time){
	updateDropped(delta_in_time);

	for (auto&& active : actives){
		active.element->update(delta_in_time);
		active.element->try_layout();
		active.updatePosition(*this);
	}

	if(!scene->isMousePressed()){
		const auto lastNotInBound = std::ranges::find_if_not(actives, [this](const tooltip_instance& toolTip){
			if(toolTip.owner->tooltip_force_drop(get_cursor_pos()))return false;

			const auto target_is = [&](tooltip_follow follow){
				return std::visit([follow] <typename T>(const T& val){
					if constexpr(std::same_as<T, math::vec2>){
						return false;
					} else if constexpr(std::same_as<T, tooltip_follow>){
						return val == follow;
					}
				}, toolTip.owner->tooltip_align_policy().pos);
			};

			const bool selfContains = !target_is(tooltip_follow::cursor) && toolTip.element->contains_self(get_cursor_pos(), MarginSize);
			const bool ownerContains = !target_is(tooltip_follow::initial_pos) && toolTip.owner->tooltip_owner_contains(get_cursor_pos());
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
		if(elem.element->on_esc() != esc_flag::droppable){
			return esc_flag::sustain;
		}
	}

	if(!actives.empty()){
		dropBack();
		return esc_flag::sustain;
	}

	return esc_flag::droppable;
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
		tooltip.element->update(delta_in_time);
		tooltip.remainTime -= delta_in_time;
		if(tooltip.remainTime <= 0){
			std::erase(drawSequence, tooltip.element.get());
		}
	}

	std::erase_if(dropped, [&](decltype(dropped)::reference dropped){
		return dropped.remainTime <= 0;
	});
}
