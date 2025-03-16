module mo_yanxi.ui.scroll_pane;

import mo_yanxi.ui.graphic;

void mo_yanxi::ui::scroll_pane::draw_pre(const rect clipSpace, const rect redirect) const{
	elem::draw_pre(clipSpace, redirect);

	const bool enableHori = enable_hori_scroll();
	const bool enableVert = enable_vert_scroll();

	if(enableHori || enableVert)get_renderer().batch.push_scissor({get_viewport().shrink(8, 8), 16});
}

void mo_yanxi::ui::scroll_pane::draw_post(const rect clipSpace, const rect redirect) const{


	const bool enableHori = enable_hori_scroll();
	const bool enableVert = enable_vert_scroll();

	using namespace graphic;

	if(enableHori || enableVert)get_renderer().batch.pop_scissor();

	draw_acquirer param{get_renderer().get_batch(), draw::white_region};

	if(enableHori){
		draw::fill::rect_ortho(param.get(), get_hori_bar_rect());
	}

	if(enableVert){
		draw::fill::rect_ortho(param.get(), get_vert_bar_rect());
	}

	elem::draw_post(clipSpace, redirect);
}
