module mo_yanxi.ui.elem.slider;

import mo_yanxi.ui.graphic;

void mo_yanxi::ui::slider::draw_content(const rect clipSpace) const{
	elem::draw_content(clipSpace);

	using namespace graphic;

	draw_acquirer param{get_renderer().get_batch(), draw::white_region};

	rect rect{get_bar_size()};

	rect.src = content_src_pos() + get_bar_cur_pos();
	draw::fill::rect_ortho(param.get(), rect, colors::gray.copy().mulA(gprop().get_opacity()));

	rect.src = content_src_pos() + get_bar_last_pos();
	draw::fill::rect_ortho(param.get(), rect, colors::light_gray.copy().mulA(gprop().get_opacity()));
}
