module mo_yanxi.ui.elem.nested_scene;

import mo_yanxi.ui.graphic;


void mo_yanxi::ui::nested_scene::draw_content(const rect clipSpace) const{
	draw_background();

	const auto proj = camera_.get_world_to_uniformed_flip_y();

	auto& batch = graphic::renderer_from_erased(this->get_renderer()).batch;

	batch.push_projection(proj);
	batch.push_viewport(scene_.get_region());

	float edge = 4 / camera_.get_scale();
	batch.push_scissor({camera_.get_viewport()/*.shrink(edge), 4*/});

	scene_.draw(camera_.get_viewport());
	//
	draw_acquirer acquirer = ui::get_draw_acquirer(get_renderer());
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

	batch.pop_scissor();
	batch.pop_viewport();
	batch.pop_projection();
}
