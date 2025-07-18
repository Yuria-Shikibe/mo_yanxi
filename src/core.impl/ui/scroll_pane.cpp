module mo_yanxi.ui.elem.scroll_pane;

import mo_yanxi.ui.graphic;
import mo_yanxi.ui.assets;

void mo_yanxi::ui::scroll_pane::draw_content(const rect clipSpace) const{
	draw_background();

	using namespace graphic;
	auto& r = renderer_from_erased(get_renderer());

	const bool enableHori = enable_hori_scroll();
	const bool enableVert = enable_vert_scroll();

	if(enableHori || enableVert)r.batch.push_scissor({get_viewport().shrink(8, 8), 16});

	if(item)item->draw(clipSpace);

	if(enableHori || enableVert)r.batch.pop_scissor();

	draw_acquirer param{r.batch, draw::white_region};
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
}

void mo_yanxi::ui::scroll_pane::update_item_layout(){
	if(!item)return;

	//TODO merge these two method
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
					bound.set_width(math::clamp_positive(bound.potential_width() - scroll_bar_stroke_));
					need_elem_relayout = true;
				}

				if(context_size_restriction.width_dependent() && sz->x > property.content_width()){
					need_self_relayout = true;
				}

				break;
			}
			case layout_policy::vert_major :{
				if(sz->x > property.content_width()){
					bound.set_height(math::clamp_positive(bound.potential_height() - scroll_bar_stroke_));
					need_elem_relayout = true;
				}

				if(context_size_restriction.height_dependent() && sz->y > property.content_height()){
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

			elemSz += property.boarder.extent();
			resize(elemSz);
		}
	}

	item->layout();
}
