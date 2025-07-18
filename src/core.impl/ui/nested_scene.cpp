module mo_yanxi.ui.elem.nested_scene;

import mo_yanxi.ui.graphic;

void mo_yanxi::ui::nested_scene::draw_pre(const rect clipSpace) const{
	elem::draw_pre(clipSpace);

	const auto proj = camera_.get_world_to_uniformed_flip_y();

	get_renderer().batch.push_projection(proj);
	get_renderer().batch.push_viewport(scene_.get_region());

	float edge = 4 / camera_.get_scale();
	get_renderer().batch.push_scissor({camera_.get_viewport()/*.shrink(edge), 4*/});

}

void mo_yanxi::ui::nested_scene::draw_content(const rect clipSpace) const{
	scene_.draw(camera_.get_viewport());
	//
	draw_acquirer acquirer{get_renderer().batch, graphic::draw::white_region};
	auto cpos = getTransferredPos(get_scene()->get_cursor_pos());
	graphic::draw::fill::rect_ortho(acquirer.get(), rect{cpos, 12});

	if(auto elem = get_selection(cpos)){
		graphic::draw::line::rect_ortho(acquirer, elem->get_bound());
	}

	if(drag_state_){
		if(auto drawer = drag_state_->element->gprop().drawer){
			drawer->draw(*drag_state_->element, drag_state_->element->get_bound().set_src(cpos + drag_state_->offset), 0.25f);
		}
	}
	// graphic::draw::fill::rect_ortho(acquirer.get(), rect{scene_.cursorPos, 12}, graphic::colors::YELLOW);
}

void mo_yanxi::ui::nested_scene::draw_post(const rect clipSpace) const{
	get_renderer().batch.pop_scissor();
	get_renderer().batch.pop_viewport();
	get_renderer().batch.pop_projection();

	elem::draw_post(clipSpace);
}
