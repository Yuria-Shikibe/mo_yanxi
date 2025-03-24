module;

#include <cassert>

module mo_yanxi.ui.elem.progress_bar;

import mo_yanxi.ui.graphic;


void mo_yanxi::ui::progress_bar_drawer::draw(const progress_bar& elem) const{
	using namespace graphic;
	draw_acquirer param{elem.get_renderer().get_batch(), draw::white_region};

	float progress = elem.get_progress().current;
	auto region = get_region(elem);
	auto [from, to] = elem.getColor();

	to.lerp(from, 1 - progress);
	from.mulA(elem.gprop().get_opacity());
	to.mulA(elem.gprop().get_opacity());

	draw::fill::fill(
		param.get(),
		region.vert_00(),
		region.vert_10(),
		region.vert_11(),
		region.vert_01(),
		from, to, to, from
	);
}

mo_yanxi::ui::rect mo_yanxi::ui::progress_bar_drawer::get_region(const progress_bar& elem) noexcept{
	return rect{tags::from_extent, elem.content_src_pos(), elem.getBarSize()};
}

void mo_yanxi::ui::progress_bar::draw_content(const rect clipSpace) const{
	elem::draw_content(clipSpace);

	assert(drawer != nullptr);
	drawer->draw(*this);



}
