module mo_yanxi.font.typesetting;

namespace mo_yanxi::font::typesetting{
	namespace func{
		float get_upper_pad(const parse_context& context, const glyph_layout& layout,
		                     const layout_rect region) noexcept{
			const auto default_spacing = context.get_line_spacing();
			auto upper_pad = region.ascender;
			if(context.get_current_row() > 0){
				const auto& last_line = layout[context.get_current_row() - 1];
				upper_pad = std::max(region.ascender + last_line.bound.descender, default_spacing);
			}

			return upper_pad;
		}
	}

	void layout_unit::push_glyph(
		const parse_context& context,
		const code_point code,
		const unsigned layout_global_index,
		const std::optional<char_code> real_code,
		bool termination
		){
		const layout_index_t column_index = buffer.size();
		auto& current = buffer.emplace_back(
			code_point{real_code.value_or(code.code), code.unit_index},
			layout_abs_pos{{column_index, 0}, layout_global_index},
			context.get_glyph(code.code)
		);

		bool emptyChar = code.code == real_code && (code.code == U'\0' || code.code == U'\n');

		const auto font_region_scale = (!termination && emptyChar) ? math::vec2{} : context.get_current_correction_scale();
		float advance = current.glyph.metrics().advance.x * font_region_scale.x;


		const auto placementPos = context.get_current_offset().add_x(pen_advance);

		current.region = current.glyph.metrics().place_to(placementPos, font_region_scale);

		if(emptyChar){
			advance = 0;
			current.region.set_width(current.region.width() / 4);
		}

		current.correct_scale = font_region_scale;
		if(current.region.get_src_x() < 0){
			//Fetch for italic line head
			advance -= current.region.get_src_x();
			current.region.src.x = 0;
		}
		current.color = context.get_color();


		bound.max_height({
				.width = 0,
				.ascender = placementPos.y - current.region.get_src_y() + 6,
				.descender = current.region.get_end_y() - placementPos.y + 6
			});
		bound.width = std::max(bound.width, current.region.get_end_x());
		pen_advance += advance;
	}

	bool parser::flush(parse_context& context, glyph_layout& layout, layout_unit& layout_unit, const bool end_line,
	                   const bool terminate){
		auto& line = layout.get_row(context.current_row);
		auto line_bound = line.bound;
		line_bound.max_height(layout_unit.bound);
		line_bound.width = std::max(line_bound.width, context.row_pen_pos + layout_unit.bound.width);

		const auto upper_pad = func::get_upper_pad(context, layout, line_bound);

		const math::vec2 line_region{line_bound.width, upper_pad + line_bound.descender};
		const math::vec2 captured = layout.captured_size.copy().max_x(line_region.x).add_y(line_region.y);

		bool size_excessive = captured.beyond(layout.get_clamp_size());
		if(!terminate && !end_line && size_excessive){
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
				const auto last_upper_pad = func::get_upper_pad(context, layout, line.bound);
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

					if(flush(context, layout, append_hint, false)){
						//maybe next line can place it, truncate current line
						if(flush(context, layout, layout_unit, end_line)){
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
			{0, code.unit_index}, idx, U'\0', true);

		parser::flush(context, layout, append_hint, code.code != U'\n', true);
	}

	void parser::operator()(glyph_layout& layout, parse_context context, const tokenized_text& formatted_text) const{
		tokenized_text::token_iterator lastTokenItr = formatted_text.tokens.begin();

		auto view = formatted_text.codes | std::views::enumerate;
		auto itr = view.begin();

		layout_unit unit{};

		auto stl = std::prev(view.end());

		for(; itr != stl; ++itr){
			auto [layout_index, code] = *itr;

			lastTokenItr = func::exec_tokens(layout, context, *this, lastTokenItr, formatted_text, layout_index);

			unit.push_glyph(context, code, layout_index);

			//TODO decide when to flush layout unit
			if(!flush(context, layout, unit, code.code == U'\n' || code.code == U'\0')){
				if((layout.policy() & layout_policy::truncate) != layout_policy{}){
					do{
						++itr;
					}while(itr != stl && itr.base()->code != U'\n');
				}else{
					break;
				}
			}
		}

		if(itr != stl){
			layout.clip = true;
		}else{
			layout.clip = false;
		}

		end_parse(layout, context, *itr.base(), itr - view.begin());

		layout.captured_size.min(layout.get_clamp_size());
	}

	void parser::operator()(glyph_layout& layout, const float scale) const{
		layout.elements.clear();
		parse_context context{scale};
		tokenized_text formatted_text{layout.get_text(), {.reserve = reserve_tokens}};

		this->operator()(layout, context, formatted_text);
	}

	tokenized_text::tokenized_text(const std::string_view string, const token_sentinel sentinel){
		static constexpr auto InvalidPos = std::string_view::npos;
		const auto size = string.size();

		codes.reserve(size + 1);

		enum struct TokenState{
			normal,
			endWaiting,
			signaled
		};

		bool escapingNext{};
		TokenState recordingToken{};
		decltype(string)::size_type tokenRegionBegin{InvalidPos};
		posed_token_argument* currentToken{};

		decltype(string)::size_type off = 0;

	scan:
		for(; off < size; ++off){
			const char codeByte = string[off];
			char_code charCode{std::bit_cast<std::uint8_t>(codeByte)};

			if(charCode == '\r') continue;

			if(charCode == '\\' && recordingToken != TokenState::signaled){
				if(!escapingNext){
					escapingNext = true;
					continue;
				} else{
					escapingNext = false;
				}
			}

			if(!escapingNext){
				if(codeByte == sentinel.signal && tokenRegionBegin == InvalidPos){
					if(recordingToken != TokenState::signaled){
						recordingToken = TokenState::signaled;
					} else{
						recordingToken = TokenState::normal;
					}

					if(recordingToken == TokenState::signaled){
						currentToken = &tokens.emplace_back(
							posed_token_argument{static_cast<std::uint32_t>(codes.size())});

						if(!sentinel.reserve) continue;
					}
				}

				if(recordingToken == TokenState::endWaiting){
					if(codeByte == sentinel.begin){
						recordingToken = TokenState::signaled;
						currentToken = &tokens.emplace_back(
							posed_token_argument{static_cast<std::uint32_t>(codes.size())});
					} else{
						recordingToken = TokenState::normal;
					}
				}

				if(recordingToken == TokenState::signaled){
					if(codeByte == sentinel.begin && tokenRegionBegin == InvalidPos){
						tokenRegionBegin = off + 1;
					}

					if(codeByte == sentinel.end){
						if(!currentToken || tokenRegionBegin == InvalidPos){
							tokenRegionBegin = InvalidPos;
							currentToken = nullptr;
							recordingToken = TokenState::normal;
							if(sentinel.reserve)goto record;
							else continue;
						}

						currentToken->data = string.substr(tokenRegionBegin, off - tokenRegionBegin);
						tokenRegionBegin = InvalidPos;
						currentToken = nullptr;
						recordingToken = TokenState::endWaiting;
					}

					if(!sentinel.reserve) continue;
				}
			}

		record:

			const auto codeSize = encode::getUnicodeLength<>(reinterpret_cast<const char&>(charCode));

			if(!escapingNext){
				if(codeSize > 1 && off + codeSize <= size){
					charCode = encode::utf_8_to_32(string.data() + off, codeSize);
				}

				codes.push_back({charCode, static_cast<code_point_index>(off)});
			} else{
				escapingNext = false;
			}

			// if(charCode == '\n') rows++;

			off += codeSize - 1;
		}

		if(!sentinel.reserve && recordingToken == TokenState::signaled){
			tokens.pop_back();

			recordingToken = TokenState::normal;
			off = tokenRegionBegin;
			goto scan;
		}

		if(codes.empty() || codes.back().code != U'\0'){
			codes.push_back({U'\0', static_cast<code_point_index>(string.size())});
			// rows++;
		}


		codes.shrink_to_fit();
	}

}
