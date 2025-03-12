module;

#include "../src/ext/enum_operator_gen.hpp"

export module mo_yanxi.font.typesetting;

export import mo_yanxi.font.manager;
export import mo_yanxi.encode;
export import mo_yanxi.graphic.color;
export import mo_yanxi.math;
export import mo_yanxi.heterogeneous.open_addr_hash;
export import Align;
import std;

namespace mo_yanxi{

	template <typename T, typename Cont = std::vector<T>>
	struct optional_stack{
		std::stack<T, Cont> stack{};

		[[nodiscard]] optional_stack() = default;

		[[nodiscard]] explicit optional_stack(const std::stack<T, Cont>& stack)
			: stack{stack}{
		}

		void push(const T& val){
			stack.push(val);
		}

		void push(T&& val){
			stack.push(std::move(val));
		}

		std::optional<T> pop_and_get(){
			if(stack.empty()){
				return std::nullopt;
			}

			const std::optional rst{std::move(stack.top())};
			stack.pop();
			return rst;
		}

		void pop(){
			if(!stack.empty()) stack.pop();
		}

		[[nodiscard]] T top(const T defaultVal) const noexcept{
			if(stack.empty()){
				return defaultVal;
			} else{
				return stack.top();
			}
		}

		[[nodiscard]] std::optional<T> top() const noexcept{
			if(stack.empty()){
				return std::nullopt;
			} else{
				return stack.top();
			}
		}
	};
}

namespace mo_yanxi::font::typesetting{
	constexpr char_code line_feed_character = U'\u2925';

	export using code_point_index = unsigned;

	export using layout_pos_t = math::upoint2;
	export using layout_index_t = unsigned;

	export struct layout_abs_pos{
		layout_pos_t pos;
		layout_index_t index;

		constexpr friend bool operator==(const layout_abs_pos& lhs, const layout_abs_pos& rhs) noexcept{
			return lhs.index == rhs.index;
		}
	};


	struct code_point{
		char_code code;
		code_point_index unit_index;
	};


	export struct glyph_elem{
		code_point code{};
		/**
		 * @brief specify the index of the code-unit in the layout text
		 */
		layout_abs_pos layout_pos{};

		glyph glyph{};
		graphic::color color{};
		math::frect region{};

		[[nodiscard]] glyph_elem() = default;

		[[nodiscard]] glyph_elem(
			code_point code,
			const layout_abs_pos layout_pos,
			font::glyph&& glyph)
			: code(code),
			  layout_pos(layout_pos),
			  glyph(std::move(glyph)){
		}

		// Math::Range heightAlign{};

		// [[nodiscard]] constexpr math::vec2 v00() const noexcept{
		// 	return src;
		// }
		//
		// [[nodiscard]] constexpr math::vec2 v01() const noexcept{
		// 	return {src.x, end.y};
		// }
		//
		// [[nodiscard]] constexpr math::vec2 v10() const noexcept{
		// 	return {end.x, src.y};
		// }
		//
		// [[nodiscard]] constexpr math::vec2 v11() const noexcept{
		// 	return end;
		// }

		[[nodiscard]] constexpr float midX() const noexcept{
			return region.get_center_x();
		}

		[[nodiscard]] constexpr float blank() const noexcept{
			return region.size().area() == 0;
		}

		[[nodiscard]] constexpr auto index() const noexcept{
			return layout_pos.index;
		}
	};


	export struct glyph_line{
		float width;
		float ascender;
		float descender;

		[[nodiscard]] constexpr float height() const noexcept{
			return ascender + descender;
		}

		[[nodiscard]] constexpr math::vec2 size() const noexcept{
			return {width, height()};
		}

		[[nodiscard]] constexpr math::frect to_region(math::vec2 src) const noexcept{
			return {tags::from_extent, src.add_y(descender), width, -height()};
		}
	};

	export inline font_manager* default_font_manager{};
	export inline font_face* default_font{};

	export
	struct token_argument{
		static constexpr char token_split_char = '|';

		std::string_view data{};

		[[nodiscard]] auto get_all_args() const noexcept{
			return data
				| std::views::split(token_split_char)
				| std::views::transform([](auto a){ return std::string_view{a}; });
		}

		[[nodiscard]] auto get_args() const noexcept{
			return get_all_args() | std::views::drop(1);
		}

		[[nodiscard]] std::string_view get_first_arg() const noexcept{
			auto v = get_args();
			if(v.empty()){
				return {};
			}else{
				return v.front();
			}
		}

		[[nodiscard]] std::string_view name() const{
			return get_all_args().front();
		}
	};


	/**
	 * @warning If any tokens are used, the lifetime of @link tokenized_text @endlink should not longer than the source string
	 */
	export
	struct tokenized_text{
		static constexpr char token_split_char = '|';

		struct token_sentinel{
			char signal{'#'};
			char begin{'<'};
			char end{'>'};
			bool reserve{false};
		};

		struct posed_token_argument : token_argument{
			std::uint32_t pos{};

			[[nodiscard]] posed_token_argument() = default;

			[[nodiscard]] explicit posed_token_argument(std::uint32_t pos)
				: pos(pos){
			}
		};

		// private:
		std::uint32_t rows{};

		/**
		 * @brief String Normalize to NFC Format
		 */
		std::vector<code_point> codes{};

		std::vector<posed_token_argument> tokens{};
		// public:

		using pos_t = decltype(codes)::size_type;
		using token_iterator = decltype(tokens)::iterator;

		token_iterator get_token(const pos_t pos, const token_iterator& last){
			return std::ranges::lower_bound(last, tokens.end(), pos, {}, &posed_token_argument::pos);
		}

		token_iterator get_token(const pos_t pos){
			return get_token(pos, tokens.begin());
		}

		[[nodiscard]] auto get_token_group(const pos_t pos, const token_iterator& last) const{
			return std::ranges::equal_range(last, tokens.end(), pos, {}, &posed_token_argument::pos);
		}

		[[nodiscard]] tokenized_text() = default;

		[[nodiscard]] explicit(false) tokenized_text(const std::string_view string, const token_sentinel sentinel = {}){
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
								throw std::invalid_argument("Parse String Tokens Failed");
							}

							currentToken->data = string.substr(tokenRegionBegin, off - tokenRegionBegin);
							tokenRegionBegin = InvalidPos;
							currentToken = nullptr;
							recordingToken = TokenState::endWaiting;
						}

						if(!sentinel.reserve) continue;
					}
				}

				const auto codeSize = encode::getUnicodeLength<>(reinterpret_cast<const char&>(charCode));

				if(!escapingNext){
					if(codeSize > 1 && off + codeSize <= size){
						charCode = encode::utf_8_to_32(string.data() + off, codeSize);
					}

					codes.push_back({charCode, static_cast<code_point_index>(off)});
				} else{
					escapingNext = false;
				}

				if(charCode == '\n') rows++;

				off += codeSize - 1;
			}

			if(!sentinel.reserve && recordingToken == TokenState::signaled){
				tokens.pop_back();

				recordingToken = TokenState::normal;
				off = tokenRegionBegin;
				goto scan;
			}

			if(codes.empty() || !codes.back().code == U'\0'){
				codes.push_back({U'\0', static_cast<code_point_index>(string.size())});
				rows++;
			}


			codes.shrink_to_fit();
		}
	};

	export struct parse_context{
	private:

	public:
		glyph_size_type default_size{64, 0};
		tokenized_text text{};

		optional_stack<graphic::color> colorHistory{};
		optional_stack<math::vec2> offsetHistory{};
		optional_stack<glyph_size_type> size_history{};
		optional_stack<font_face*> font_history{};

		math::vec2 captured_size{};

		//TODO replace this
		math::vec2 pen_pos{};

		glyph_line line_rect{};

	private:
		float throughout_scale{1.f};
		math::vec2 currentOffset{};
		float minimumLineHeight{50.f};
		float minimum_line_spacing{12.f};

	public:
		void reset_context(){
			colorHistory.stack = {};
			offsetHistory.stack = {};
			size_history.stack = {};
			font_history.stack = {};
		}

		[[nodiscard]] math::vec2 get_current_correction_scale() const noexcept{
			math::isize2 sz = get_current_size().as<math::isize2::value_type>();
			if(sz.y == 0)sz.y = sz.x;
			else if(sz.x == 0)sz.x = sz.y;

			const math::isize2 snapped{get_snapped_size(sz.x), get_snapped_size(sz.y)};

			return sz.as<float>() / snapped.as<float>();
		}

		[[nodiscard]] math::vec2 get_current_offset() const noexcept{
			return currentOffset * throughout_scale;
		}

		void set_throughout_scale(const float scl) noexcept{
			throughout_scale = scl;
		}

		[[nodiscard]] float get_throughout_scale() const noexcept{
			return throughout_scale;
		}

		[[nodiscard]] float get_min_line_height() const noexcept{
			return minimumLineHeight * throughout_scale;
		}

		[[nodiscard]] float get_line_spacing() const noexcept{
			const auto sz = get_current_size();
			const auto spacing = get_face().get_line_spacing(sz);
			return math::max(spacing, minimum_line_spacing) * throughout_scale;
		}

		[[nodiscard]] glyph_size_type get_current_size() const noexcept{
			return size_history.top(default_size).scl(throughout_scale);
		}

		[[nodiscard]] glyph_size_type get_current_snapped_size() const noexcept{
			const auto sz = get_current_size();
			return {get_snapped_size(sz.x), get_snapped_size(sz.y)};
		}

		[[nodiscard]] graphic::color get_color() const noexcept{
			return colorHistory.top(graphic::colors::white);
		}

		void push_offset(const math::vec2 off){
			offsetHistory.push(off);
			currentOffset += off;
		}

		void push_scaled_current_size(const float scl){
			size_history.push(size_history.top(default_size).scl(scl));
		}

		void push_scaled_offset(const math::vec2 offScale){
			auto off = size_history.top(default_size);
			if(off.x == 0) off.x = off.y;
			if(off.y == 0) off.y = off.x;

			const auto offset = off.as<float>() * offScale;
			offsetHistory.push(offset);
			currentOffset += offset;
		}

		math::vec2 pop_offset(){
			const math::vec2 offset = offsetHistory.pop_and_get().value_or({});
			currentOffset -= offset;
			return offset;
		}

		[[nodiscard]] font_face& get_face() const{
			const auto t = font_history.top();
			if(!t){
				if(default_font) return *default_font;
				throw std::runtime_error("No Valid Font Face");
			}
			return **t;
		}

		[[nodiscard]] glyph get_glyph(const char_code code) const{
			return default_font_manager->get_glyph_exact(get_face(), glyph_identity{code, get_current_snapped_size()});
		}

	};



	export enum struct layout_policy{
		/**
		 * @brief try layout all character within given direction clamp length with auto feed-line
		 * if not set, character beyond clamp size are either reserved or clipped
		 */
		auto_feed_line = 1 << 0,

		/**
		 * @brief reserve character beyond clip-space
		 */
		reserve = 1 << 1,

		//Not implemented
		horizontal = 0 << 2,
		vertical = 1 << 2,

		//Not implemented
		/**
		 * @brief reverse layout direction
		 */
		reversed = 1 << 3,
	};

	BITMASK_OPS(export, layout_policy)

	export struct parser;

	export struct glyph_layout{
		friend parser;

		struct row{
			using row_type = std::vector<glyph_elem>;
			// float baselineHeight{};
			math::vec2 src{};
			glyph_line bound{};
			row_type glyphs{};

			[[nodiscard]] math::frect getRectBound() const noexcept{
				return {src.x, src.y - bound.descender, bound.width, bound.height()};
			}

			[[nodiscard]] float top() const noexcept{
				return src.y + bound.ascender;
			}

			[[nodiscard]] float bottom() const noexcept{
				return src.y - bound.descender;
			}

			[[nodiscard]] std::size_t size() const noexcept{
				return glyphs.size();
			}

			const glyph_elem& operator[](const layout_index_t row) const noexcept{
				return glyphs[row];
			}

			glyph_elem& operator[](const layout_index_t row) noexcept{
				return glyphs[row];
			}

			[[nodiscard]] bool isFakeLine() const noexcept{
				return glyphs.empty() || glyphs.back().code.code != U'\n';
			}

			[[nodiscard]] bool isAppendLine() const noexcept{
				return !glyphs.empty() && glyphs.front().code.code == U'\0';
			}

			[[nodiscard]] bool has_valid_before(const std::size_t xPos) const noexcept{
				if(xPos == 0) return false;
				for(std::size_t x = 0; x < std::min(xPos - 1, glyphs.size()); ++x){
					if(glyphs[x].code.code != U'\0'){
						return true;
					}
				}
				return false;
			}

			[[nodiscard]] row_type::const_iterator line_nearest(const float x_local) const noexcept{
				return std::ranges::lower_bound(glyphs, x_local - src.x, {}, &glyph_elem::midX);
			}

			[[nodiscard]] row_type::iterator line_nearest(const float x_local) noexcept{
				return std::ranges::lower_bound(glyphs, x_local - src.x, {}, &glyph_elem::midX);
			}
		};

	private:
		std::string text{};
		std::vector<row> elements{};

		math::vec2 captured_size{};
		math::vec2 clamp_size{};
		Align::Pos align{Align::Pos::top_left};

		float drawScale{1.f};
		bool clip{};
		layout_policy policy_{};

	public:
		// math::vec2 maximumSize{};
		// math::vec2 minimumSize{};

		[[nodiscard]] glyph_layout() = default;

		[[nodiscard]] std::string_view get_text() const{
			return text;
		}

		[[nodiscard]] math::vec2 extent() const{
			return captured_size;
		}

		[[nodiscard]] constexpr math::vec2 draw_extent() const noexcept{
			return extent().copy().scl(drawScale);
		}

		[[nodiscard]] math::vec2 get_clamp_size() const noexcept{
			return clamp_size;
		}

		void set_align(const Align::Pos align){
			if(this->align == align) return;
			this->align = align;

			update_align();
		}

		void update_align(){
			if(align & Align::Pos::center_x){
				for(auto& element : elements){
					element.src.x = (captured_size.x - element.bound.width) / 2.f;
				}
			} else if(align & Align::Pos::left){
				for(auto& element : elements){
					element.src.x = 0;
				}
			} else if(align & Align::Pos::right){
				for(auto& element : elements){
					element.src.x = (captured_size.x - element.bound.width);
				}
			}
		}

		[[nodiscard]] layout_policy policy() const noexcept{
			return policy_;
		}

		[[nodiscard]] bool is_clipped() const noexcept{
			return clip;
		}

		[[nodiscard]] bool is_size_compatible(const math::vec2 clip_size) const noexcept{
			if(captured_size.within(clamp_size)){
				return true;
			}else if((policy_ & layout_policy::auto_feed_line) == layout_policy::auto_feed_line){
				if(clip){
					if((policy_ & layout_policy::reserve) == layout_policy::reserve){
						if((policy_ & layout_policy::vertical) == layout_policy::vertical){
							if(clamp_size.y == clip_size.y){
								return true;
							}
						}else{
							if(clamp_size.x == clip_size.x){
								return true;
							}
						}
					}
				}else{
					return true;
				}
			}else if((policy_ & layout_policy::reserve) == layout_policy::reserve){
				return true;
			}
			return false;
		}

		[[nodiscard]] bool is_layout_compatible(const std::string_view text, const math::vec2 clip_size) const{
			const bool size_cpt{is_size_compatible(clip_size)};

			return size_cpt && this->text == text;
		}

		// bool resetSize(const math::vec2 size){
		// 	if(is_size_compatible(size)){
		//
		// 	}else{
		// 		elements.clear();
		// 	}
		// 	if(maximumSize.equals(size)) return false;
		//
		// 	maximumSize = size;
		// 	clip = false;
		// 	return true;
		// }

		// void reset(std::string&& newText, const math::vec2 size){
		// 	resetSize(size);
		// 	text = std::move(newText);
		// }

		[[nodiscard]] std::span<const row> rows() const noexcept{
			return elements;
		}

		[[nodiscard]] std::span<row> rows() noexcept{
			return elements;
		}

		[[nodiscard]] bool empty() const noexcept{
			return elements.empty();
		}

		[[nodiscard]] auto row_size() const noexcept{
			return elements.size();
		}

		[[nodiscard]] auto all_glyphs() const noexcept{
			return elements | std::views::transform(&row::glyphs) | std::views::join;
		}

		[[nodiscard]] auto get_begin_char_unit() const noexcept{
			return text.front();
		}

		[[nodiscard]] const glyph_elem& at(const layout_pos_t where) const noexcept{
			return elements[where.y].glyphs[where.x];
		}

		[[nodiscard]] bool contains(const layout_pos_t where) const noexcept{
			if(where.y >= row_size()) return false;
			if(where.x >= elements[where.y].size()) return false;
			return true;
		}

		[[nodiscard]] bool is_end(const layout_pos_t where) const noexcept{
			if(where.y + 1 == row_size()){
				if(where.x + 1 == elements[where.y].glyphs.size()) return true;
			}
			return false;
		}

		const row& operator [](const layout_index_t row) const noexcept{
			return elements[row];
		}

		row& operator [](const layout_index_t row) noexcept{
			return elements[row];
		}

		template <typename T>
		auto& operator[](this T& self, layout_index_t row, layout_index_t column) noexcept{
			return self[row][column];
		}

		[[nodiscard]] const glyph_elem* find_valid_elem(const layout_index_t index) const noexcept{
			auto lowerRow = std::ranges::upper_bound(elements, index, {}, [](const row& row){
				return row.glyphs.empty() ? std::numeric_limits<layout_index_t>::max() : row.glyphs.front().index();
			});

			if(lowerRow == elements.begin()) return nullptr;
			--lowerRow;

			const auto upperRow = std::ranges::upper_bound(lowerRow, elements.end(), index, {}, [](const row& row){
				return row.glyphs.empty() ? std::numeric_limits<layout_index_t>::max() : row.glyphs.front().index();
			});

			const auto dst = std::distance(lowerRow, upperRow);

			if(dst == 0) return nullptr;

			if(dst == 1)[[likely]] {
				const auto elem = std::ranges::upper_bound(lowerRow->glyphs | std::views::reverse, index,
				                                           std::greater_equal{}, &glyph_elem::index);
				if(elem != lowerRow->glyphs.rend()){
					return std::to_address(elem);
				}
			} else{
				auto view = std::ranges::subrange{lowerRow, upperRow} | std::views::transform(&row::glyphs) |
					std::views::join | std::views::reverse;
				const auto elem = std::ranges::upper_bound(view, index, std::greater_equal{}, &glyph_elem::index);
				if(elem != view.end()){
					return std::to_address(elem);
				}
			}

			std::unreachable();
		}

		row& get_row(const std::size_t row){
			if(row >= row_size()){
				elements.resize(row + 1);
				return elements[row];
			}

			return elements[row];
		}

		void append_line(){
			elements.resize(row_size() + 1);
			// if(elements.size() > 1){
			// 	elements.back().src.y = elements[elements.size() - 2].src.y + line_spacing;
			// }
		}

		void pop_line(){
			elements.pop_back();
		}
	};

	export
	struct token_modifier{
		using function_type = void(glyph_layout&, parse_context&, token_argument);
		std::add_pointer_t<function_type> modifier{};

		void operator()(glyph_layout& layout, parse_context& ctx, const token_argument token) const{
			modifier(layout, ctx, token);
		}

		[[nodiscard]] token_modifier() = default;

		[[nodiscard]] explicit(false) token_modifier(function_type modifier)
			: modifier{modifier}{}
	};

	namespace func{

		export template <typename T>
			requires std::is_arithmetic_v<T>
		T string_cast(std::string_view str, T def = 0){
			T t{def};
			std::from_chars(str.data(), str.data() + str.size(), t);
			return t;
		}

		export template <typename T = int>
			requires std::is_arithmetic_v<T>
		std::vector<T> string_cast_seq(const std::string_view str, T def = 0, std::size_t expected = 2){
			const char* begin = str.data();
			const char* end = begin + str.size();

			std::vector<T> result{};
			if(expected) result.reserve(expected);

			while(!expected || result.size() != expected){
				if(begin == end) break;
				T t{def};
				auto [ptr, ec] = std::from_chars(begin, end, t);
				begin = ptr;

				if(ec == std::errc::invalid_argument){
					begin++;
				} else{
					result.push_back(t);
				}
			}

			return result;
		}

		template <typename T, std::size_t sz>
		struct cast_result{
			std::array<T, sz> data;
			typename std::array<T, sz>::size_type size;
		};

		export template <typename T = int, std::size_t expected_count>
			requires std::is_arithmetic_v<T>
		cast_result<T, expected_count> string_cast_seq(const std::string_view str, T def = 0){
			const char* begin = str.data();
			const char* end = begin + str.size();

			std::array<T, expected_count> result{};
			std::size_t count{};

			while(count != expected_count && begin != end){
				T t{def};
				auto [ptr, ec] = std::from_chars(begin, end, t);
				begin = ptr;

				if(ec == std::errc::invalid_argument){
					begin++;
				} else{
					result[count++] = t;
				}
			}

			return {result, count};
		}


		void push_color(parse_context& context, std::string_view arg){

			if(arg.empty()){
				context.colorHistory.pop();
			} else{
				const auto color = graphic::color::from_string(arg);
				context.colorHistory.push(color);
			}
		}


		export void begin_subscript(glyph_layout& layout, parse_context& context){
			context.push_scaled_offset({-0.025f, -0.105f});
			context.push_scaled_current_size(0.55f);
		}

		export void begin_superscript(glyph_layout& layout, parse_context& context){
			context.push_scaled_offset({-0.025f, 0.525f});
			context.push_scaled_current_size(0.55f);
		}

		export void end_script(glyph_layout& layout, parse_context& context){
			context.pop_offset();
			context.size_history.pop();
		}

	}

	struct parser_base{
		string_open_addr_hash_map<token_modifier> modifiers{};

		void exec_token(
			glyph_layout& layout,
			parse_context& context,
			token_argument token) const{
			if(token.data.empty())return;
			std::string_view name{};

			for (const auto basic_string_view : token.get_all_args() | std::views::take(1)){
				name = basic_string_view;
			}

			if(name.starts_with('[')){
				name.remove_prefix(1);

				if(name.empty())return;

				if(name.starts_with('#')){
					name.remove_prefix(1);
					func::push_color(context, name);
				}else if(std::isdigit(name[0])){
					context.size_history.push({static_cast<glyph_size_type::value_type>(func::string_cast(name, 64)), 0});
				}else if(name.starts_with('^')){
					func::begin_superscript(layout, context);
				}else if(name.starts_with('_')){
					func::begin_subscript(layout, context);
				}else if(name.starts_with('/')){
					func::end_script(layout, context);
				}else{
					if(auto* font = default_font_manager->find_face(name)){
						context.font_history.push(font);
					}
				}

			}else{
				if(const auto tokenParser = modifiers.try_find(name)){
					tokenParser->operator()(layout, context, static_cast<token_argument>(token));
				} else{
					// std::println(std::cerr, "[Parser] Failed To Find Token: {}", token.data);
				}
			}


		}
	};

	namespace func{

		tokenized_text::token_iterator exec_tokens(
			glyph_layout& layout,
			parse_context& context,
			const parser_base& parser,
			tokenized_text::token_iterator lastTokenItr,
			const tokenized_text& formattableText,
			const layout_index_t index
		){
			auto&& tokens = formattableText.get_token_group(index, lastTokenItr);

			for(const auto& token : tokens){
				parser.exec_token(layout, context, token);
			}

			return tokens.end();
		}

		std::optional<float> append_glyph_horizontal(
			glyph_layout& layout,
			parse_context& context,
			const code_point code,
			const unsigned layoutIndex,
			const layout_index_t rowIndex,
			const bool no_clip = false,
			const bool use_zero = false
		){
			auto& line = layout.get_row(rowIndex);

			const auto linePos = line.glyphs.size();
			auto& current = line.glyphs.emplace_back(
				use_zero ? code_point{U'\0', code.unit_index} : code,
				layout_abs_pos{{static_cast<layout_index_t>(linePos), rowIndex}, layoutIndex},
				context.get_glyph(code.code)
			);

			const auto font_region_scale = context.get_current_correction_scale();
			float advance = current.glyph.metrics.advance.x * font_region_scale.x * context.get_throughout_scale();
			if(is_space(code.code)){
				advance = math::min(advance, layout.get_clamp_size().x - context.pen_pos.x);
			}


			const auto off = context.get_current_offset();
			const auto localPenPos = context.pen_pos - line.src;
			const auto placementPos = localPenPos + off;

			current.region = current.glyph.metrics.place_to(placementPos, font_region_scale);
			if(current.region.get_src_x() < 0){
				advance -= current.region.get_src_x();
				current.region.src.x = 0;
			}

			if(
				(layout.policy() & layout_policy::reserve) == layout_policy{} ||
				(layout.policy() & layout_policy::auto_feed_line) != layout_policy{}
			){
				if(!no_clip && context.pen_pos.x + advance > layout.get_clamp_size().x){
					return std::optional{advance};
				}
			}


			current.color = context.get_color();

			context.line_rect.ascender = math::max(context.line_rect.ascender, placementPos.y - current.region.get_src_y());
			context.line_rect.descender = math::max(context.line_rect.descender, current.region.get_end_y() - placementPos.y);

			context.pen_pos.x += advance;
			current.region.expand(font_draw_expand * font_region_scale);
			return std::nullopt;
		}

		void redirect(
			const std::span<glyph_elem> elems,
			const math::vec2 src,
			const math::vec2 dst,
			const layout_abs_pos src_layout_pos,
			const layout_abs_pos dst_layout_pos
			){
			for (auto && elem : elems){
				elem.region.src += dst - src;
				elem.layout_pos.index += dst_layout_pos.index - src_layout_pos.index;
				elem.layout_pos.pos += dst_layout_pos.pos - src_layout_pos.pos;
			}
		}

		bool end_line(
			glyph_layout& layout,
			parse_context& context,
			const bool append,
			const bool no_clip = false
			){
			auto& line = layout.rows().back();
			const auto defualt_height = context.get_line_spacing();

			if(layout.row_size() == 1){
				const float f1 = std::max(defualt_height, context.line_rect.descender + context.line_rect.ascender) - context.line_rect.descender;
				const float f2 = std::max(defualt_height, context.line_rect.ascender);
				line.src.y = math::lerp(f1, f2, 0.5f);
			}else{
				auto& lastLine = std::next(layout.rows().crbegin()).operator*();

				line.src.y = lastLine.src.y + std::max(defualt_height, lastLine.bound.descender + context.line_rect.ascender);
			}

			const float max_height = line.src.y + context.line_rect.descender;

			//TODO clip
			if((layout.policy() & layout_policy::reserve) == layout_policy{}){
				if(!no_clip && max_height > layout.get_clamp_size().y){
					//discard line
					layout.pop_line();
					return false;
				}
			}


			context.captured_size.y = max_height;
			context.captured_size.max_x(context.pen_pos.x);

			context.line_rect.width = context.pen_pos.x;
			line.bound = context.line_rect;

			context.line_rect = {};
			context.pen_pos = {};


			if(append)layout.append_line();

			return true;
		}

		void end_parse(glyph_layout& layout){
			const float offset = layout[0].bound.ascender;
			for (auto && row : layout.rows()){
				row.src.y += offset;
			}
		}

	}


	struct parser : parser_base{

		glyph_layout operator()(
			std::string_view str,
			layout_policy policy = {},
			math::vec2 clip_space = math::vectors::constant2<float>::max_vec2) const{
			tokenized_text formatted_text{str};
			parse_context context{};


			glyph_layout layout{};
			layout.clamp_size = clip_space;
			layout.policy_ = policy;

			std::uint32_t currentRowIndex{};
			tokenized_text::token_iterator lastTokenItr = formatted_text.tokens.begin();

			bool requires_context_redo{false};

			auto view = formatted_text.codes | std::views::enumerate;
			auto itr = view.begin();
			for(; itr != view.end(); ++itr){
				auto [layout_index, code] = *itr;

				if(requires_context_redo){
					tokenized_text::token_iterator token_itr = formatted_text.tokens.begin();

					for(decltype(layout_index) i = 0; i < layout_index; ++i){
						token_itr = func::exec_tokens(layout, context, *this, token_itr, formatted_text, i);

					}
					requires_context_redo = false;
				}

				lastTokenItr = func::exec_tokens(layout, context, *this, lastTokenItr, formatted_text, layout_index);

				auto t = policy & layout_policy::reserve;

				if(auto excessive = func::append_glyph_horizontal(layout, context, code, layout_index, currentRowIndex)){
					if((policy & layout_policy::auto_feed_line) != layout_policy{}){
						if(excessive.value() > layout.get_clamp_size().x){
							layout.clip = true;
							return layout;
						}

						{//roll back to last seperator
							auto& line = layout.get_row(currentRowIndex);

							while(true){
								if(itr == view.begin()){
									layout.clip = true;
									return layout;
								}

								const auto c = (--itr).base()->code;
								line.glyphs.pop_back();

								if(c == U'\n' || c == U'\0'){
									layout.clip = true;
									return layout;
								}
								if(is_unicode_separator(c))break;
							}

							if(!line.glyphs.empty() && line.glyphs.back().code.code == U'\0'){
								layout.elements.resize(currentRowIndex);
								layout.clip = true;
								return layout;
							}
						}

						if(!func::end_line(layout, context, true)){
							layout.clip = true;
							return layout;
						}
						currentRowIndex++;


						if(func::append_glyph_horizontal(
							layout, context,
							{line_feed_character, code.unit_index}, layout_index, currentRowIndex, false, true)){

							layout.get_row(currentRowIndex).glyphs.pop_back();

							layout.clip = true;
							return layout;
							}

						requires_context_redo = true;
						continue;
					}else if((policy & layout_policy::reserve) != layout_policy{}){

					}else{
						auto& line = layout.get_row(currentRowIndex);
						line.glyphs.pop_back();
						while(itr != view.end()){
							if(itr++.base()->code == U'\0'){
								--itr;
								break;
							}
							if(itr++.base()->code == U'\n')break;
						}
						if(!func::end_line(layout, context, true)){
							return layout;
						}
						currentRowIndex++;

						requires_context_redo = true;
						continue;
					}
				}

				if(code.code == U'\n'){
					if(!func::end_line(layout, context, true)){
						return layout;
					}
					currentRowIndex++;
				}
			}

			if(layout.row_size() > 0 && layout.rows().back().size() > 0 && layout.rows().back().glyphs.back().code.code == '\0'){
				func::end_line(layout, context, false);
			}else{
				func::append_glyph_horizontal(layout, context, formatted_text.codes.back(), itr - view.begin() + 1, currentRowIndex, true);
				func::end_line(layout, context, false);
			}

			return layout;
		}


	};

	//全词换行
	//假换行提示
	//

	export
	void apd_default_modifiers(parser& parser){
		parser.modifiers["size"] = +[](glyph_layout& layout, parse_context& ctx, const token_argument token){

			const std::string_view arg = token.get_first_arg();
			if(arg.empty()){
				ctx.size_history.pop();
				return;
			}
			if(arg.starts_with('[')){
				const auto [arr, count] = func::string_cast_seq<std::uint16_t, 2>(arg.substr(1), 0);

				switch(count){
				case 1 : [[fallthrough]];
				case 2 : ctx.size_history.push(std::bit_cast<glyph_size_type>(arr));
					break;
				default : ctx.size_history.pop();
				}
			}
		};

		parser.modifiers["scl"] = +[](glyph_layout& layout, parse_context& context, const token_argument token){
			const std::string_view arg = token.get_first_arg();

			if(arg.empty()){
				context.size_history.pop();
				return;
			}

			const auto scale = func::string_cast<float>(arg);
			context.push_scaled_current_size(scale);
		};

		parser.modifiers["s"] = parser.modifiers["size"];

		parser.modifiers["color"] = +[](glyph_layout& layout, parse_context& context, const token_argument token){
			std::string_view arg = token.get_first_arg();

			if(arg.empty()){
				context.colorHistory.pop();
				return;
			}

			if(arg.starts_with('[')){
				arg.remove_prefix(1);
				func::push_color(context, arg);
			}
		};

		parser.modifiers["c"] = parser.modifiers["color"];

		parser.modifiers["off"] = +[](glyph_layout& layout, parse_context& context, const token_argument token){\
			const std::string_view arg = token.get_first_arg();

			if(arg.empty()){
				context.pop_offset();
				return;
			}

			const auto [arr, count] = func::string_cast_seq<float, 2>(arg, 0.f);

			switch(count){
			case 1 : context.push_offset({0, arr[0]});
				break;
			case 2 : context.push_offset({arr[0], arr[1]});
				break;
			default : context.pop_offset();
			}
		};
		//
		parser.modifiers["offs"] = +[](glyph_layout& layout, parse_context& context, const token_argument token){
			const std::string_view arg = token.get_first_arg();
			if(arg.empty()){
				context.pop_offset();
				return;
			}

			const auto [arr, count] = func::string_cast_seq<float, 2>(arg, 0.f);

			switch(count){
			case 1 : context.push_scaled_offset({0, arr[0]});
				break;
			case 2 : context.push_scaled_offset({arr[0], arr[1]});
				break;
			default : context.pop_offset();
			}
		};

		parser.modifiers["font"] = +[](glyph_layout& layout, parse_context& context, const token_argument token){
			const std::string_view arg = token.get_first_arg();
			if(arg.empty()){
				context.font_history.pop();
				return;
			}

			if(auto* font = default_font_manager->find_face(arg)){
				context.font_history.push(font);
			}
		};

		parser.modifiers["_"] = parser.modifiers["sub"] = +[](glyph_layout& layout, parse_context& context,
		                                                      const token_argument token){
			func::begin_subscript(layout, context);
		};

		parser.modifiers["^"] = parser.modifiers["sup"] = +[](glyph_layout& layout, parse_context& context,
		                                                      const token_argument token){
			func::begin_superscript(layout, context);
		};

		parser.modifiers["/"] = parser.modifiers["/sup"] = parser.modifiers["/sub"] = +[](
			glyph_layout& layout, parse_context& context, const token_argument token){
				func::end_script(layout, context);
			};
	}
}
