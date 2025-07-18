module mo_yanxi.ui.image_drawable;

import mo_yanxi.ui.primitives;
import mo_yanxi.ui.graphic;

void mo_yanxi::ui::image_drawable::draw(
	const elem& elem,
	math::frect region,
	graphic::color color_scl) const{

	draw_acquirer acquirer{graphic::renderer_from_erased(elem.get_renderer()).get_batch(), image};
	acquirer.proj.mode_flag = draw_flags;
	graphic::draw::fill::rect_ortho(acquirer.get(), region, color_scl);
}

void mo_yanxi::ui::icon_drawable::draw(const elem& elem, math::frect region, graphic::color color_scl) const{
	draw_acquirer acquirer{graphic::renderer_from_erased(elem.get_renderer()).get_batch(), image};
	acquirer.proj.mode_flag = draw_flags;
	graphic::draw::fill::rect_ortho(acquirer.get(), region, color_scl);
}

void mo_yanxi::ui::image_caped_region_drawable::draw(const elem& elem, math::frect region, graphic::color color_scl) const{
	draw_acquirer acquirer{graphic::renderer_from_erased(elem.get_renderer()).get_batch()};
	acquirer.proj.mode_flag = draw_flags;

	const auto h = region.height() / 2.f;
	graphic::draw::caped_region(acquirer, this->region, region.vert_00().add_y(h), region.vert_10().add_y(h), color_scl, scale);
}

void mo_yanxi::ui::image_nine_region_drawable::draw(const elem& elem, math::frect region, graphic::color color_scl) const{

	draw_acquirer acquirer{graphic::renderer_from_erased(elem.get_renderer()).get_batch()};
	acquirer.proj.mode_flag = draw_flags;
	graphic::draw::nine_patch(acquirer, this->region, region, color_scl);

}
