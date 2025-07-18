module mo_yanxi.ui.elem.collapser;

import mo_yanxi.ui.graphic;

void mo_yanxi::ui::collapser::draw_pre(const rect clipSpace) const{
	basic_group::draw_pre(clipSpace);
}

void mo_yanxi::ui::collapser::draw_content(const rect clipSpace) const{
	const auto space = property.content_bound_absolute().intersection_with(clipSpace);
	head_->draw(space);

	auto& batch = graphic::renderer_from_erased(this->get_renderer()).batch;

	if(is_expanding())batch.push_scissor({get_collapsed_region()});

	if(expand_progress > 0){
		content_->draw(space);
	}

	if(is_expanding())batch.pop_scissor();
}
