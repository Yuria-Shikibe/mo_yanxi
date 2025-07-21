module mo_yanxi.ui.elem.slider;

import mo_yanxi.graphic.draw.multi_region;
import mo_yanxi.ui.graphic;
import mo_yanxi.ui.assets;


void mo_yanxi::ui::raw_slider::draw_bar(
	const slider2d_slot& slot,
	graphic::color base,
	graphic::color temp
	) const noexcept{
	draw_acquirer param = get_draw_acquirer(get_renderer());
	using namespace graphic;

	param.proj.mode_flag = draw::mode_flags::sdf;
	rect rect{get_bar_size()};

	const auto barext = getBarMovableSize();

	if(slot.is_sliding()){
		rect.src = content_src_pos() + slot.get_progress() * barext;
		draw::nine_patch(param, theme::shapes::base, rect, base.mul_a(gprop().get_opacity()));
	}

	rect.src = content_src_pos() + slot.get_temp_progress() * barext;
	draw::nine_patch(param, theme::shapes::base, rect, temp.mul_a(gprop().get_opacity()));
}

void mo_yanxi::ui::raw_slider::draw_content(rect clipSpace) const{
	elem::draw_content(clipSpace);

	draw_bar(bar);
}
