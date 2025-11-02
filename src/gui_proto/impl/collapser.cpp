module mo_yanxi.gui.elem.collapser;

namespace mo_yanxi::gui{
void collapser::update_item_size(bool isContent) const{
	const auto size = item_size[isContent];
	const auto [major, minor] = layout::get_vec_ptr(layout_policy_);

	if(!layout::is_size_pending(size)){
		auto sz = content_extent();
		sz.*minor = size;
		items[isContent]->resize(sz, propagate_mask::lower);
		items[isContent]->restriction_extent = sz;
	} else{
		auto csize = content_extent();
		layout::optional_mastering_extent extent{csize};
		extent.set_minor_pending(layout_policy_);
		auto finalsize = items[isContent]->pre_acquire_size(extent).value();
		finalsize.*major = csize.*major;
		items[isContent]->restriction_extent = extent;
		items[isContent]->resize(finalsize, propagate_mask::lower);
	}
}

bool collapser::update_abs_src(math::vec2 parent_content_src) noexcept{
	if(elem::update_abs_src(parent_content_src)){
		auto [_, minor] = layout::get_vec_ptr(layout_policy_);
		auto src = content_src_pos_abs();
		items[0]->update_abs_src(src);
		auto sz = items[0]->extent();
		src.*minor += pad_ + sz.*minor;
		items[1]->update_abs_src(src);
		return true;
	}
	return false;
}

void collapser::update_collapse(float delta) noexcept{
	const bool enterable = expandable();
	switch(state_){
	case collapser_state::un_expand:{
		if (enterable){
			expand_reload_ += delta;
			if(expand_reload_ >= settings.expand_enter_spacing){
				expand_reload_ = 0.f;

				if(std::isinf(settings.expand_speed)){
					state_ = collapser_state::expanded;
				}else{
					state_ = collapser_state::expanding;
				}

			}
		}else{
			expand_reload_ = 0;
		}
		break;
	}
	case collapser_state::expanding:{
		expand_reload_ += settings.expand_speed * delta;
		notify_layout_changed(propagate_mask::local | propagate_mask::force_upper);
		require_scene_cursor_update();

		if(expand_reload_ >= 1){
			expand_reload_ = 0.f;
			state_ = collapser_state::expanded;
			if(update_opacity_during_expand_)content().update_opacity(get_draw_opacity());
		}else if(update_opacity_during_expand_){
			content().update_opacity(get_interped_progress() * get_draw_opacity());
		}
		break;
	}
	case collapser_state::expanded:{
		if (!enterable){
			expand_reload_ += delta;
			if(expand_reload_ >= settings.expand_exit_spacing){
				expand_reload_ = 1.f;

				if(std::isinf(settings.expand_speed)){
					state_ = collapser_state::un_expand;
				}else{
					state_ = collapser_state::exit_expanding;
				}
			}
		}else{
			expand_reload_ = 0;
		}
		break;
	}
	case collapser_state::exit_expanding:{
		expand_reload_ = std::fdim(expand_reload_, settings.expand_speed * delta);
		notify_layout_changed(propagate_mask::local | propagate_mask::force_upper);
		require_scene_cursor_update();

		if(expand_reload_ == 0.f){
			state_ = collapser_state::un_expand;
			if(update_opacity_during_expand_)content().update_opacity(0);
		}else if(update_opacity_during_expand_){
			content().update_opacity(get_interped_progress() * get_draw_opacity());
		}
		break;
	}
	default: std::unreachable();
	}
}

std::optional<math::vec2> collapser::pre_acquire_size_impl(layout::optional_mastering_extent extent){
	auto table_size = head().pre_acquire_size(extent).value_or(head().extent());

	if(layout_policy_ == layout::layout_policy::hori_major){
		extent.set_height_pending();
		extent.set_width(table_size.x);
	} else if(layout_policy_ == layout::layout_policy::vert_major){
		extent.set_width_pending();
		extent.set_height(table_size.y);
	}

	if(float prog = get_interped_progress(); prog >= std::numeric_limits<float>::epsilon()){
		auto [_, minor] = layout::get_vec_ptr(layout_policy_);
		auto content_ext = content().pre_acquire_size(extent).value_or(content().extent());
		content_ext.*minor += pad_;
		table_size.*minor += content_ext.*minor * prog;
	}

	return table_size;
}

events::op_afterwards collapser::on_click(const events::click event, std::span<elem* const> aboves){
	if(expand_cond_ == collapser_expand_cond::click){
		if((!aboves.empty() && aboves.front() == items[0].get())){
			if(event.key.action == input_handle::act::release){
				clicked_ = !clicked_;
			}
			return events::op_afterwards::intercepted;
		} else if(head().contains(event.pos)){
			cursor_states_.update_press(event.key);
			if(event.key.action == input_handle::act::release){
				clicked_ = !clicked_;
			}
			return events::op_afterwards::intercepted;
		}

		return events::op_afterwards::fall_through;
	}else{
		return elem::on_click(event, aboves);
	}

}

void collapser::draw_content(const rect clipSpace) const{
	draw_background();

	const auto space = content_bound_abs().intersection_with(clipSpace);
	head().draw(space);

	switch(state_){
	case collapser_state::un_expand : break;
	case collapser_state::expanding :[[fallthrough]];
	case collapser_state::exit_expanding :{
		auto& r = get_scene().renderer();
		r.push_scissor({get_collapsed_region()});
		r.notify_viewport_changed();
		content().draw(space);
		r.pop_scissor();
		r.notify_viewport_changed();
		break;
	}
	case collapser_state::expanded : content().draw(space);
		break;
	default : std::unreachable();
	}
}

float collapser::get_interped_progress() const noexcept{
	static constexpr auto smoother = [](float a) static noexcept{
		return a * a * a * (a * (a * 6.0f - 15.0f) + 10.0f);
	};
	switch(state_){
	case collapser_state::expanding: [[fallthrough]];
	case collapser_state::exit_expanding: return smoother(expand_reload_);
	case collapser_state::un_expand: return 0;
	case collapser_state::expanded: return 1;
	default: std::unreachable();
	}
}

rect collapser::get_collapsed_region() const noexcept{
	const auto [_, minor] = layout::get_vec_ptr(layout_policy_);
	const auto prog = get_interped_progress();
	const auto off = head().extent().*minor + pad_ * prog;
	auto content_src = content_src_pos_abs();
	content_src.*minor += off;

	auto content_ext = content_extent();
	content_ext.*minor = math::fdim(content_ext.*minor, off);

	return rect{tags::from_extent, content_src, content_ext};
}
}
