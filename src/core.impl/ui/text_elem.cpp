module mo_yanxi.ui.elem.label;

import mo_yanxi.graphic.renderer.ui;
import mo_yanxi.ui.graphic;
import mo_yanxi.font.typesetting.draw;


std::optional<mo_yanxi::font::typesetting::layout_pos_t> mo_yanxi::ui::label::get_layout_pos(
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

void mo_yanxi::ui::label::draw_content(const rect clipSpace) const{
	// elem::draw_content(clipSpace, redirect);

	draw_acquirer acquirer{graphic::renderer_from_erased(get_renderer()).batch};
	graphic::draw::glyph_layout(acquirer, glyph_layout, get_glyph_abs_src(), property.graphic_data.get_opacity() * (disabled ? 0.3f : 1.f));
}
