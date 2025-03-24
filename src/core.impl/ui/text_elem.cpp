module mo_yanxi.ui.elem.text_elem;

import mo_yanxi.graphic.renderer.ui;
import mo_yanxi.ui.graphic;
import mo_yanxi.font.typesetting.draw;


std::optional<mo_yanxi::font::typesetting::layout_pos_t> mo_yanxi::ui::basic_text_elem::get_layout_pos(
	math::vec2 globalPos) const{
	if(glyph_layout.empty() || !contains(globalPos)){
		return std::nullopt;
	}

	using namespace font::typesetting;
	auto textLocalPos = globalPos - get_glyph_abs_src();

	auto row =
	   std::ranges::lower_bound(
		   glyph_layout.rows(), textLocalPos.y,
		   {}, &glyph_layout::row::bottom);


	if(row == glyph_layout.rows().end()){
		if(glyph_layout.rows().empty()){
			return std::nullopt;
		}else{
			row = std::ranges::prev(glyph_layout.rows().end());
		}
	}

	auto elem = row->line_nearest(textLocalPos.x);

	if(elem == row->glyphs.end() && !row->glyphs.empty()){
		elem = std::prev(row->glyphs.end());
	}

	return layout_pos_t{
		static_cast<layout_pos_t::value_type>(std::ranges::distance(row->glyphs.begin(), elem)),
		static_cast<layout_pos_t::value_type>(std::ranges::distance(glyph_layout.rows().begin(), row))
	};
}

void mo_yanxi::ui::basic_text_elem::draw_content(const rect clipSpace) const{
	// elem::draw_content(clipSpace, redirect);

	draw_acquirer acquirer{get_renderer().batch, {}};
	graphic::draw::glyph_layout(acquirer, glyph_layout, get_glyph_abs_src(), false, property.graphic_data.get_opacity());
}
