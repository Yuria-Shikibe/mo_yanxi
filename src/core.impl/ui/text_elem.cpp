module mo_yanxi.ui.elem.text_elem;

import mo_yanxi.graphic.renderer.ui;
import mo_yanxi.ui.graphic;
import mo_yanxi.font.typesetting.draw;


void mo_yanxi::ui::basic_text_elem::draw_content(const rect clipSpace, rect redirect) const{
	// elem::draw_content(clipSpace, redirect);

	draw_acquirer acquirer{get_renderer().batch, {}};
	graphic::draw::glyph_layout(acquirer, glyph_layout, content_src_pos(), false, property.graphicData.get_opacity());
}
