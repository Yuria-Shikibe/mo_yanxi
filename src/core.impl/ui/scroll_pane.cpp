module mo_yanxi.ui.elem.scroll_pane;

import mo_yanxi.ui.graphic;
import mo_yanxi.ui.assets;

void mo_yanxi::ui::scroll_pane::draw_pre(const rect clipSpace) const{
	elem::draw_pre(clipSpace);

	const bool enableHori = enable_hori_scroll();
	const bool enableVert = enable_vert_scroll();

	if(enableHori || enableVert)get_renderer().batch.push_scissor({get_viewport().shrink(8, 8), 16});
}

void mo_yanxi::ui::scroll_pane::draw_post(const rect clipSpace) const{


	const bool enableHori = enable_hori_scroll();
	const bool enableVert = enable_vert_scroll();

	using namespace graphic;

	if(enableHori || enableVert)get_renderer().batch.pop_scissor();

	draw_acquirer param{get_renderer().get_batch(), draw::white_region};
	param.proj.mode_flag = vk::vertices::mode_flag_bits::sdf;

	if(enableHori){
		float shrink = scroll_bar_stroke_ * .25f;
		auto rect = get_hori_bar_rect().shrink(2).move_y(prop().boarder.bottom * .0 + shrink);
		rect.add_height(-shrink);

		draw::nine_patch(param, theme::shapes::base, rect, colors::gray);
	}

	if(enableVert){
		float shrink = scroll_bar_stroke_ * .25f;
		auto rect = get_vert_bar_rect().shrink(2).move_x(prop().boarder.right * .0 + shrink);
		rect.add_width(-shrink);
		draw::nine_patch(param, theme::shapes::base, rect, colors::gray);
	}

	elem::draw_post(clipSpace);
}

void mo_yanxi::ui::scroll_pane::update_item_layout(){
	if(!item)return;

	modifyChildren(*item);
	setChildrenFillParentSize_Quiet_legacy(*item, content_size());

	auto bound = item->context_size_restriction;

	if(auto sz = item->pre_acquire_size(bound)){
		bool need_self_relayout = false;

		if(bar_caps_size){
			bool need_elem_relayout = false;
			switch(layout_policy_){
			case layout_policy::hori_major :{
				if(sz->y > property.content_height()){
					bound.width.value -= scroll_bar_stroke_;
					bound.width.value = math::clamp_positive(bound.width.value);
					need_elem_relayout = true;
				}

				if(context_size_restriction.width.dependent() && sz->x > property.content_width()){
					need_self_relayout = true;
				}

				break;
			}
			case layout_policy::vert_major :{
				if(sz->x > property.content_width()){
					bound.height.value -= scroll_bar_stroke_;
					bound.height.value = math::clamp_positive(bound.height.value);
					need_elem_relayout = true;
				}

				if(context_size_restriction.height.dependent() && sz->y > property.content_height()){
					need_self_relayout = true;
				}
				break;
			}
			default: break;
			}

			if(need_elem_relayout){
				auto s = item->pre_acquire_size(bound);
				if(s) sz = s;
			}
		}

		item->resize_masked(*sz, spread_direction::local | spread_direction::child);

		if(need_self_relayout){
			auto elemSz = item->get_size();
			switch(layout_policy_){
			case layout_policy::hori_major :{
				if(elemSz.x > property.content_width()){
					//width resize
					elemSz.y = content_size().y;
					elemSz.x += static_cast<float>(bar_caps_size) * scroll_bar_stroke_;
				}
				break;
			}
			case layout_policy::vert_major :{
				if(elemSz.y > property.content_height()){
					//height resize
					elemSz.x = content_size().x;
					elemSz.y += static_cast<float>(bar_caps_size) * scroll_bar_stroke_;
				}
				break;
			}
			default: break;
			}

			elemSz += property.boarder.get_size();
			resize(elemSz);
		}
	}

	item->layout();
}
