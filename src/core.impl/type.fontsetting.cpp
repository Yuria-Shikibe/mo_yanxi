module mo_yanxi.font.typesetting;

namespace mo_yanxi::font::typesetting{
	namespace func{
		float get_uppper_pad(const parse_context& context, const glyph_layout& layout,
		                     const layout_rect region) noexcept{
			const auto default_spacing = context.get_line_spacing();
			auto upper_pad = region.ascender;
			if(context.current_row > 0){
				const auto& last_line = layout[context.current_row - 1];
				upper_pad = std::max(region.ascender + last_line.bound.descender, default_spacing);
			}

			return upper_pad;
		}
	}


	bool parser::try_append(parse_context& context, glyph_layout& layout, layout_unit& layout_unit, const bool end_line,
	                        const bool terminate){
		auto& line = layout.get_row(context.current_row);
		auto line_bound = line.bound;
		line_bound.max_height(layout_unit.bound);
		line_bound.width = std::max(line_bound.width, context.row_pen_pos + layout_unit.bound.width);

		const auto upper_pad = func::get_uppper_pad(context, layout, line_bound);

		const math::vec2 line_region{line_bound.width, upper_pad + line_bound.descender};
		const math::vec2 captured = layout.captured_size.copy().max_x(line_region.x).add_y(line_region.y);

		if(!terminate && !end_line && captured.beyond(layout.get_clamp_size())){
			//buffer is too big in extent
			if(line.size() == 0 || (line.is_append_line() && line.size() == 1)){
				//the largest line can't place it, failed, pop the append line
				layout.pop_line();
				if(context.current_row > 0){
					context.current_row--;
					context.row_pen_pos = layout.get_row(context.current_row).bound.width;
				} else{
					context.row_pen_pos = 0;
				}

				return false;
			} else{
				const auto last_upper_pad = func::get_uppper_pad(context, layout, line.bound);
				const math::vec2 last_line_region{line.bound.width, last_upper_pad + line.bound.descender};

				if((layout.policy() & layout_policy::truncate) != layout_policy{}){
					layout.captured_size.max_x(last_line_region.x).add_y(last_line_region.y);
					context.current_row++;
					context.row_pen_pos = 0;

					return false;
				}else{
					//auto feed line
					struct layout_unit append_hint{};
					auto& last_glyph = layout_unit.buffer.back();

					//truncate line hint:
					append_hint.push_glyph(
						context,
						{line_feed_character, last_glyph.code.unit_index}, last_glyph.layout_pos.index + 1, U'\0');

					layout.captured_size.max_x(last_line_region.x).add_y(last_line_region.y);
					context.current_row++;
					context.row_pen_pos = 0;

					if(try_append(context, layout, append_hint, false)){
						//maybe next line can place it, truncate current line
						if(try_append(context, layout, layout_unit, end_line)){
							return true;
						}
					}

					return false;
				}
			}
		} else{
			//buffer is placeable, append it to current line

			const layout_index_t last_index = line.size();
			for(auto&& glyph : layout_unit.buffer){
				glyph.layout_pos.pos = {last_index, context.current_row};
				glyph.region.move_x(context.row_pen_pos);
				line.glyphs.push_back(std::move(glyph));
			}

			context.row_pen_pos += layout_unit.pen_advance;
			line.bound = line_bound;


			layout_unit.clear();

			const auto last_y_offset = [&]() -> float{
				if(context.current_row == 0){
					return 0;
				}
				auto& last_line = layout[context.current_row - 1];
				return last_line.src.y;
			}();
			auto y_offset = last_y_offset + upper_pad;
			line.src.y = y_offset;

			if(end_line || terminate){
				layout.captured_size = captured;
				context.current_row++;
				context.row_pen_pos = 0;
			}

			return true;
		}
	}

	void parser::end_parse(glyph_layout& layout, parse_context& context, const code_point code,
	                       const layout_index_t idx){
		struct layout_unit append_hint{};

		append_hint.push_glyph(
			context,
			{0, code.unit_index}, idx, U'\0');

		parser::try_append(context, layout, append_hint, code.code != U'\n', true);
	}

	void parser::operator()(glyph_layout& layout, parse_context context, const tokenized_text& formatted_text) const{
		tokenized_text::token_iterator lastTokenItr = formatted_text.tokens.begin();

		auto view = formatted_text.codes | std::views::enumerate;
		auto itr = view.begin();

		layout_unit unit{};

		for(; itr != view.end(); ++itr){
			auto [layout_index, code] = *itr;

			lastTokenItr = func::exec_tokens(layout, context, *this, lastTokenItr, formatted_text, layout_index);

			unit.push_glyph(context, code, layout_index);
			if(!try_append(context, layout, unit, code.code == U'\n' || code.code == U'\0')){
				if((layout.policy() & layout_policy::truncate) != layout_policy{}){
					do{
						++itr;
					}while(itr != view.end() && itr.base()->code != U'\n');
				}else{
					break;
				}
			}
		}

		if(itr != view.end()){
			end_parse(layout, context, *itr.base(), itr - view.begin());
		}

		layout.captured_size.min(layout.get_clamp_size());
	}

	void parser::operator()(glyph_layout& layout, const float scale) const{
		layout.elements.clear();
		parse_context context{};
		context.set_throughout_scale(scale);
		tokenized_text formatted_text{layout.get_text(), {.reserve = reserve_tokens}};

		this->operator()(layout, context, formatted_text);
		// auto idx = parse(layout, context, formatted_text);
		// end_parse(layout, context, formatted_text, idx + 1);
	}
}
