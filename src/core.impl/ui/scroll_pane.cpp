module mo_yanxi.ui.elem.scroll_pane;

import mo_yanxi.ui.graphic;
import mo_yanxi.ui.assets;

void mo_yanxi::ui::scroll_pane::draw_pre(const rect clipSpace) const{
	elem::draw_pre(clipSpace);

	const bool enableHori = enable_hori_scroll();
	const bool enableVert = enable_vert_scroll();

	if(enableHori || enableVert)get_renderer().batch.push_scissor({get_viewport().shrink(8, 8), 16});
}

void mo_yanxi::ui::scroll_pane::draw_post(const rect clipSpace) const{


	const bool enableHori = enable_hori_scroll();
	const bool enableVert = enable_vert_scroll();

	using namespace graphic;

	if(enableHori || enableVert)get_renderer().batch.pop_scissor();

	draw_acquirer param{get_renderer().get_batch(), draw::white_region};
	param.proj.mode_flag = vk::vertices::mode_flag_bits::sdf;

	if(enableHori){
		float shrink = scroll_bar_stroke_ * .25f;
		auto rect = get_hori_bar_rect().shrink(2).move_y(prop().boarder.bottom * -.25f - shrink);
		rect.add_height(-shrink);

		draw::nine_patch(param, assets::shapes::base, rect, colors::gray);
	}

	if(enableVert){
		float shrink = scroll_bar_stroke_ * .25f;
		auto rect = get_vert_bar_rect().shrink(2).move_x(prop().boarder.right * .25f + shrink);
		rect.add_width(-shrink);
		draw::nine_patch(param, assets::shapes::base, rect, colors::gray);
	}

	elem::draw_post(clipSpace);
}
