module mo_yanxi.ui.image_drawable;

import mo_yanxi.ui.elem;
import mo_yanxi.ui.graphic;

void mo_yanxi::ui::image_drawable::draw(
	const elem& elem,
	math::frect region,
	graphic::color color_scl,
	graphic::color color_ovr){

	draw_acquirer acquirer{elem.get_renderer().get_batch(), image};
	acquirer.proj.mode_flag = draw_flags;
	graphic::draw::fill::rect_ortho(acquirer.get(), region, color_scl, color_ovr);
}
