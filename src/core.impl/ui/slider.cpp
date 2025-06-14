module mo_yanxi.ui.elem.slider;

import mo_yanxi.graphic.draw.multi_region;
import mo_yanxi.ui.graphic;
import mo_yanxi.ui.assets;

void mo_yanxi::ui::slider::draw_content(const rect clipSpace) const{
	elem::draw_content(clipSpace);

	using namespace graphic;

	draw_acquirer param{get_renderer().get_batch(), draw::white_region};

	param.proj.mode_flag = draw::mode_flags::sdf;
	rect rect{get_bar_size()};

	if(is_sliding()){
		rect.src = content_src_pos() + get_bar_cur_pos();
		draw::nine_patch(param, theme::shapes::base, rect, colors::gray.copy().mul_a(gprop().get_opacity()));
	}

	rect.src = content_src_pos() + get_bar_last_pos();
	draw::nine_patch(param, theme::shapes::base, rect, colors::light_gray.copy().mul_a(gprop().get_opacity()));

}
