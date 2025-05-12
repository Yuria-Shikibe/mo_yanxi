module mo_yanxi.ui.elem.text_input_area;

import mo_yanxi.ui.graphic;

void mo_yanxi::ui::text_input_area::draw_content(const rect clipSpace) const{
	using namespace graphic;
	if(caret_){
		auto off2 = get_glyph_abs_src();

		draw_acquirer param{get_renderer().get_batch(), draw::white_region};
		if(caret_->shouldBlink()){
			const color caret_Color =
				(on_failed() ? colors::red_dusted : colors::white).copy().mul_a(gprop().get_opacity());

			const auto& row = glyph_layout[caret_->range.dst.pos.y];
			const auto& glyph = row[caret_->range.dst.pos.x];

			const float height = row.bound.height();

			rect rect{tags::from_extent, (off2 + row.src).add(glyph.region.get_src_x(), -row.bound.ascender), {caret::Stroke, height}};

			draw::fill::rect_ortho(param.get(), rect, caret_Color);
		}

		if(caret_->range.src == caret_->range.dst){
			goto drawBase;
		}

		auto [beg, end] = caret_->range.ordered();
		const auto& beginRow = glyph_layout[beg.pos.y];
		const auto& endRow = glyph_layout[end.pos.y];

		const auto& beginGlyph = beginRow[beg.pos.x];
		const auto& endGlyph = endRow[end.pos.x];

		const math::vec2 beginPos = (beginRow.src + off2).add(beginGlyph.region.get_src_x(), -beginRow.bound.ascender);

		const math::vec2 endPos = (endRow.src + off2).add(endGlyph.region.get_src_x(), endRow.bound.descender);

		const color lineSelectionColor =
			(on_failed() ? colors::red_dusted : colors::gray).copy().mul_a(.35f);

		if(beg.pos.y == end.pos.y){
			draw::fill::rect_ortho(param.get(), rect{beginPos, endPos}, lineSelectionColor);
		}else{
			auto totalSize = glyph_layout.extent();
			const math::vec2 beginLineEnd = beginRow.src.copy().add_y(beginRow.bound.descender).set_x(totalSize.x) + off2;
			 math::vec2 endLineBegin = endRow.src.copy().add_y(-endRow.bound.ascender).set_x(0) + off2;

			rect srcRect{beginPos, beginLineEnd};
			rect endRect{endLineBegin, endPos};
			draw::fill::rect_ortho(param.get(), srcRect, lineSelectionColor);

			if(end.pos.y - beg.pos.y > 1){
				draw::fill::rect_ortho(param.get(), rect{beginLineEnd/* - math::vec2{0., srcRect.height()}*/, endLineBegin/* + math::vec2{0., endRect.height()}*/}, lineSelectionColor);
			}else{
				endLineBegin.y = beginPos.y + beginRow.bound.height();
				endRect = rect{endLineBegin, endPos};
			}

			draw::fill::rect_ortho(param.get(), endRect, lineSelectionColor);
		}
	}

	drawBase:
	
	label::draw_content(clipSpace);
	
}
