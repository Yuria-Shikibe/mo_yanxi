module;

#include <cassert>

export module mo_yanxi.ui.elem.text_input_area;

export import mo_yanxi.ui.elem.text_elem;

import mo_yanxi.font.typesetting;
import mo_yanxi.graphic.color;
import mo_yanxi.math;
import mo_yanxi.encode;
import std;

namespace mo_yanxi::ui{
	bool caresAbout(const font::char_code code) noexcept{
		if(code > std::numeric_limits<std::uint8_t>::max()){
			return true;
		}
		return std::isalpha(code);
	}

	using Layout = font::typesetting::glyph_layout;

	struct caret_range{
		font::typesetting::layout_abs_pos src{};
		font::typesetting::layout_abs_pos dst{};

		[[nodiscard]] constexpr auto ordered() const noexcept{
			auto [min, max] = std::minmax(src, dst, less);
			return caret_range{min, max};
		}

		constexpr static bool less(const font::typesetting::layout_abs_pos l, const font::typesetting::layout_abs_pos r) noexcept{
			if(l.index == r.index){
				if(l.pos.y == r.pos.y)return l.pos.x < r.pos.x;
				return l.pos.y < r.pos.y;
			}

			return l.index < r.index;
		}

		[[nodiscard]] bool hasRegion() const noexcept{
			return src != dst;
		}

		[[nodiscard]] bool includedLine(const Layout& layout) const noexcept{
			if(src.pos.y != dst.pos.y)return false;

			const auto [min, max] = std::minmax(src.pos.x, dst.pos.x);
			if(min != 0)return false;
			if(layout[dst.pos.y].size() != max + 1)return false;
			return true;
		}

		[[nodiscard]] const font::typesetting::glyph_elem* getPrevGlyph(const Layout& layout) const noexcept{
			if(dst.index == 0)return nullptr;

			if(dst.pos.x == 0){
				return &layout[dst.pos.y - 1].glyphs.back();
			}else{
				return &layout[dst.pos.y][dst.pos.x - 1];
			}
		}

		[[nodiscard]] const font::typesetting::glyph_elem* getNextGlyph(const Layout& layout) const noexcept{
			if(layout.is_end(dst.pos))return nullptr;

			auto& row = layout[dst.pos.y];

			if(dst.pos.x == row.size() - 1){
				return &layout[dst.pos.y + 1].glyphs.front();
			}else{
				return &row[dst.pos.x + 1];
			}
		}

		[[nodiscard]] std::string_view getSelection(const Layout& layout) const noexcept{
			if(hasRegion()){
				auto sorted = ordered();
				const auto from = layout.at(sorted.src.pos).code.unit_index;
				const auto to = layout.at(sorted.dst.pos).code.unit_index;

				const std::string_view sv{layout.get_text()};
				return sv.substr(from, to - from);
			}else{
				return "";
			}
		}
	};

	struct caret{
		static constexpr float Stroke = 3;
		static constexpr float BlinkCycle = 60;
		static constexpr float BlinkThreshold = BlinkCycle * .5f;

		caret_range range{};
		float blink{};

		[[nodiscard]] caret() = default;

		[[nodiscard]] explicit caret(const Layout& layout, const font::typesetting::layout_pos_t begin)
			: range({begin}, {begin}){
			auto idx = layout.at(begin).layout_pos.index;
			range.src.index = idx;
			range.dst.index = idx;
		}

		[[nodiscard]] explicit caret(const caret_range& pos)
			: range(pos){
		}

		[[nodiscard]] constexpr bool shouldBlink() const noexcept{
			return blink < BlinkThreshold;
		}

		void toLowerLine(const Layout& layout, const bool append) noexcept{
			toLineAt(layout, range.dst.pos.y + 1, append);
		}

		void toUpperLine(const Layout& layout, const bool append) noexcept{
			if(range.dst.pos.y > 0){
				toLineAt(layout, range.dst.pos.y - 1, append);
			}
		}

		bool deleteAt(Layout& layout) noexcept{
			if(deleteRange(layout)){
				const auto min = std::min(range.src, range.dst, caret_range::less);
				mergeTo(min, false);
				return true;
			}else{
				mergeTo(range.dst, false);
				const auto idx = layout.at(range.dst.pos).code.unit_index;
				auto code = layout.get_text()[idx];
				if(code == 0)return false;
				const auto len = encode::getUnicodeLength(layout.get_text()[idx]);

				if(len){
					layout.text.erase(idx, len);

					if(range.dst.pos.y > 0){
						auto& lastLine = layout[range.dst.pos.y - 1];
						if(lastLine.isFakeLine() && !layout[range.dst.pos.y].has_valid_before(range.dst.pos.x)){
							mergeTo(layout, lastLine.size() - 1, range.dst.pos.y - 1, false);
						}
					}

					return true;
				}

			}

			return false;
		}

		bool backspaceAt(Layout& layout) noexcept{
			if(deleteRange(layout)){
				const auto min = std::min(range.src, range.dst, caret_range::less);
				mergeTo(min, false);
				return true;
			}else{
				const auto idx = layout.at(range.dst.pos).code.unit_index;
				if(idx == 0)return false;
				const auto end = layout.text.begin() + idx;
				const auto begin = encode::gotoUnicodeHead(std::ranges::prev(end));

				assert(begin != end);
				layout.text.erase(begin, end);

				if(range.dst.pos.y > 0){
					auto& lastLine = layout[range.dst.pos.y - 1];
					if(lastLine.isFakeLine() && !layout[range.dst.pos.y].has_valid_before(range.dst.pos.x)){
						mergeTo(layout, static_cast<int>(lastLine.size()) - 1, range.dst.pos.y - 1, false);
						advance(layout, 1, false);
					}else{
						toLeft(layout, false);
					}
				}else{
					toLeft(layout, false);
				}

				return true;
			}
		}

		void selectAll(const Layout& layout){
			range.src = {0, 0, 0};
			range.dst.pos.y = layout.row_size() - 1;
			range.dst.pos.x = layout.rows().back().size() - 1;
			range.dst.index = layout.at(range.dst.pos).layout_pos.index;
		}

		void toRight(const Layout& layout, const bool append, const bool skipAllPadder = false) noexcept{
			if(range.hasRegion() && !append){
				const auto max = std::max(range.src, range.dst, caret_range::less);
				mergeTo(max, false);
				return;
			}

			if(skipAllPadder){
				while(!layout.is_end(range.dst.pos) && layout.at(range.dst.pos).code.code == 0){
					toRight_impl(layout, append);
				}
			}else{
				if(!layout.is_end(range.dst.pos) && layout.at(range.dst.pos).code.code == 0){
					toRight_impl(layout, append);
				}
			}


			toRight_impl(layout, append);
		}

		void toLeft(const Layout& layout, const bool append) noexcept{
			if(range.hasRegion() && !append){
				const auto min = std::min(range.src, range.dst, caret_range::less);
				mergeTo(min, false);
				return;
			}

			toLeft_impl(layout, append);

			if(layout.at(range.dst.pos).code.code == 0){
				toLeft_impl(layout, append);
			}
		}

		void toLeft_jump(const Layout& layout, const bool append){
			if (range.dst.index != 0 && !caresAbout(layout.at(range.dst.pos).code.code)) {
				toLeft(layout, append);
			}

			auto* last{range.getPrevGlyph(layout)};

			while (last) {
				if(!caresAbout(last->code.code))break;
				toLeft(layout, append);
				last = range.getPrevGlyph(layout);
			}

			while (last) {
				if(caresAbout(last->code.code))break;
				toLeft(layout, append);
				last = range.getPrevGlyph(layout);
			}
		}

		void toRight_jump(const Layout& layout, const bool append){
			auto* next{range.getNextGlyph(layout)};

			while (next) {
				if(!caresAbout(next->code.code))break;
				toRight(layout, append);
				next = range.getNextGlyph(layout);
			}

			while (next) {
				if(caresAbout(next->code.code))break;
				toRight(layout, append);
				next = range.getNextGlyph(layout);
			}
		}

		std::size_t insertAt(Layout& layout, std::string&& buffer){
			if(range.hasRegion()){
				auto sorted = range.ordered();
				const auto from = layout.text.begin() + layout.at(sorted.src.pos).code.unit_index;
				const auto to = layout.text.begin() + layout.at(sorted.dst.pos).code.unit_index;

				layout.text.replace_with_range(from, to, std::move(buffer));
				mergeTo(sorted.src, false);
			}else{
				mergeTo(range.dst, false);
				const auto index = layout.at(range.dst.pos).code.unit_index;
				const auto where = layout.text.begin() + index;
				layout.text.insert_range(where, std::move(buffer));
			}

			return buffer.size();
		}

		void update(Layout& layout, const float delta) noexcept{
			blink += delta;
			if(blink > BlinkCycle){
				blink = 0;
			}
		}

		void checkIndex(const Layout& layout) noexcept{
			const bool hasRegion = range.hasRegion();

			if(!layout.contains(range.dst.pos) || layout.at(range.dst.pos).index() != range.dst.index){
				if(const auto elem = layout.find_valid_elem(range.dst.index)){
					range.dst = elem->layout_pos;
				}
			}

			if(!hasRegion){
				range.src = range.dst;
				return;
			}

			if(!layout.contains(range.src.pos) || layout.at(range.src.pos).index() != range.src.index){
				if(const auto elem = layout.find_valid_elem(range.src.index)){
					range.src = elem->layout_pos;
				}
			}
		}

		void toLineEnd(const Layout& layout, const bool append){
			if(layout.rows().empty())return;

			auto line = layout.rows().begin() + range.dst.pos.y;
			while(line != layout.rows().end() && line->isFakeLine()){
				++line;
				if(line == layout.rows().end()){
					--line;
					break;
				}
			}

			mergeTo(layout, line->size() - 1, line - layout.rows().begin(), append);
		}

		void toLineBegin(const Layout& layout, const bool append){
			auto line = layout.rows().begin() + range.dst.pos.y;
			while(line->isAppendLine() && line != layout.rows().begin()){
				--line;
			}

			mergeTo(layout, 0, line - layout.rows().begin(), append);
		}

		void advance(const Layout& layout, int adv, const bool append){
			range.dst.index += adv;
			if(!append)range.src = range.dst;

			checkIndex(layout);
		}

	private:

		void toRight_impl(const Layout& layout, const bool append) noexcept{
			auto& line = layout[range.dst.pos.y];
			if(line.size() <= range.dst.pos.x + 1){
				// line end, to next line
				if(layout.row_size() <= range.dst.pos.y + 1){
					//no next line
					tryMerge(append);
				}else{
					mergeTo(layout, 0, range.dst.pos.y + 1, append);
				}
			}else{
				mergeTo(layout, range.dst.pos.x + 1, range.dst.pos.y, append);
			}
		}

		void toLeft_impl(const Layout& layout, const bool append) noexcept{
			if(range.dst.pos.x == 0){
				// line end, to next line
				if(range.dst.pos.y == 0){
					//no next line
					tryMerge(append);
				}else{
					auto& nextLine = layout[range.dst.pos.y - 1];
					mergeTo(layout, math::max<int>(nextLine.size() - 1, 0), range.dst.pos.y - 1, append);
				}
			}else{
				mergeTo(layout, range.dst.pos.x - 1, range.dst.pos.y, append);
			}
		}

		auto deleteRange(Layout& layout) const{
			math::section rng{layout.at(range.src.pos).code.unit_index, layout.at(range.dst.pos).code.unit_index};
			rng = rng.to_ordered();
			const auto len = rng.length();
			if(len){
				layout.text.erase(layout.text.begin() + rng.from, layout.text.begin() + rng.to);
			}
			return len;
		}

		void toLineAt(const Layout& layout,
			const font::typesetting::layout_index_t nextRowIndex,
			const bool append) noexcept{
			if(layout.row_size() <= nextRowIndex)return;

			auto& lastLine = layout[range.dst.pos.y];
			const auto curPos = lastLine[range.dst.pos.x].region.src;

			auto& nextLine = layout[nextRowIndex];
			const auto next = nextLine.line_nearest(curPos.x + lastLine.src.x);
			if(next == nextLine.glyphs.end()){
				mergeTo(layout, math::max<int>(nextLine.size() - 1, 0), nextRowIndex, append);
			}else{

				mergeTo(next->layout_pos, append);
			}
		}

		void mergeTo(const font::typesetting::layout_abs_pos where, const bool append){
			this->mergeTo(where.pos.x, where.pos.y, where.index, append);
		}

		void mergeTo(const std::integral auto x, const std::integral auto y, const unsigned index, const bool append){
			range.dst = {static_cast<font::typesetting::layout_pos_t::value_type>(x), static_cast<font::typesetting::layout_pos_t::value_type>(y), index};
			if(!append)range.src = range.dst;
			blink = 0;
		}

		void mergeTo(const Layout& layout, const std::integral auto x, const std::integral auto y, const bool append){
			this->mergeTo(x, y, layout.at({static_cast<font::typesetting::layout_pos_t::value_type>(x), static_cast<font::typesetting::layout_pos_t::value_type>(y)}).layout_pos.index, append);
		}

		void tryMerge(const bool append){
			if(!append)range.src = range.dst;
			blink = 0;
		}
	};

	struct InputHistory{
		std::string text{};
		caret_range where{};
	};

	struct InputHistoryStack{
		static constexpr std::size_t MaxHistory = 64;
	private:
		using Stack = std::deque<InputHistory>;
		Stack stack{};
		Stack::iterator current{stack.begin()};

	public:
		[[nodiscard]] bool empty() const noexcept{
			return stack.empty();
		}

		[[nodiscard]] auto size() const noexcept{
			return stack.size();
		}

		void push(const std::string_view text, const caret_range caretPos){
			if(current != stack.end()){
				stack.resize(current - stack.begin() + 1);
			}

			stack.emplace_back(std::string{text}, caretPos);

			if(stack.size() > MaxHistory){
				stack.pop_front();
			}

			current = stack.end();
		}

		[[nodiscard]] const InputHistory* toPrev() noexcept{
			if(stack.empty())return nullptr;

			if(current == stack.end()){
				--current;
			}

			if(current == stack.begin()){
				return &stack.front();
			}else{
				--current;
				return std::to_address(current);
			}
		}

		[[nodiscard]] const InputHistory* toNext() noexcept{
			if(stack.empty())return nullptr;

			if(current == stack.end()){
				return &stack.back();
			}else{
				++current;
				if(current == stack.end()){
					return nullptr;
				}
				return std::to_address(current);
			}
		}

		[[nodiscard]] const InputHistory& get() const noexcept{
			return *current;
		}
		void pop() noexcept{
			if(!stack.empty()){
				stack.pop_back();
			}

			current = stack.end();
		}
	};

	export
	struct text_input_area : basic_text_elem{
		[[nodiscard]] text_input_area(scene* scene, group* group)
			: basic_text_elem(scene, group, "input_area"){
			/*events().on<events::click>([](const events::click& event, elem& e){
				auto& self = static_cast<text_input_area&>(e);

				if(event.code.action() == core::ctrl::act::release)return;
				auto layoutPos = self.get_layout_pos(event.pos);

				self.caret_ = layoutPos.transform([&self](const font::typesetting::layout_pos_t pos){
					return caret{self.glyph_layout, pos};
				});

				if(self.caret_){
					self.set_focused_key(true);
				}
			});*/

			events().on<events::drag>([](const events::drag& event, elem& e){
				auto& self = static_cast<text_input_area&>(e);

				auto layoutSrc = self.get_layout_pos(event.pos);

				if(!layoutSrc)return;

				auto layoutDst = self.get_layout_pos(event.dst);

				if(!layoutDst)return;

				self.caret_ = caret{
					{
						{layoutSrc.value(), self.glyph_layout.at(layoutSrc.value()).index()},
						{layoutDst.value(), self.glyph_layout.at(layoutDst.value()).index()}
					}};

				self.set_focused_key(true);
			});

			property.maintain_focus_until_mouse_drop = true;
			parser = &font::typesetting::global_empty_parser;
		}


	protected:
		std::string emptyHint{">_..."};
		bool isHinting{};
		float failedHintDuration{};
		std::size_t maximumUnits{std::numeric_limits<std::size_t>::max()};
		std::unordered_set<font::char_code> bannedCodePoints{};

		InputHistoryStack history{};
		std::optional<caret> caret_{};
		std::u32string buffer{};

		void update(const float delta_in_ticks) override{
			basic_text_elem::update(delta_in_ticks);

			if(failedHintDuration > 0.f){
				failedHintDuration -= delta_in_ticks;
			}

			if(!is_focused_key()){
				caret_ = std::nullopt;
			}

			if(caret_){
				if(isHinting){
					glyph_layout.text.clear();
					layoutTextThenResume({});
					isHinting = false;
				}

				caret_->checkIndex(glyph_layout);

				// if(is_focused_key()){
				// 	get_scene()->setIMEPos(prop().absoluteSrc.as<int>());
				// }

				if(!buffer.empty()){
					insert(encode::utf_32_to_8(buffer), buffer.size());
				}

				caret_->update(glyph_layout, delta_in_ticks);
			}else{
				if(!emptyHint.empty() && glyph_layout.text.empty() && !isHinting){
					isHinting = true;
					set_text(emptyHint);
				}
			}

			buffer.clear();
		}

		void layout() override{
			basic_text_elem::layout();
			if(caret_)caret_->checkIndex(glyph_layout);

		}


		events::click_result on_click(const events::click click_event) override{
			if(click_event.code.action() != core::ctrl::act::release){
				auto layoutPos = get_layout_pos(click_event.pos);

				caret_ = layoutPos.transform([this](const font::typesetting::layout_pos_t pos){
					return caret{glyph_layout, pos};
				});

				if(caret_){
					set_focused_key(true);
				}
			}

			return elem::on_click(click_event);;
		}

		void input_key(const core::ctrl::key_code_t key, const core::ctrl::key_code_t action, const core::ctrl::key_code_t mode) override{
			using namespace core::ctrl;
			if(caret_ && action == act::press || action == act::repeat){
				bool shift = mode & mode::Shift;
				bool ctrl = mode & mode::ctrl;
				switch(key){
				case key::Right :
					if(ctrl){
						caret_->toRight_jump(glyph_layout, shift);
					}else{
						caret_->toRight(glyph_layout, shift);
					}
					break;
				case key::Left :
					if(ctrl){
						caret_->toLeft_jump(glyph_layout, shift);
					}else{
						caret_->toLeft(glyph_layout, shift);
					}
					break;
				case key::Up : caret_->toUpperLine(glyph_layout, shift);
					break;
				case key::Down : caret_->toLowerLine(glyph_layout, shift);
					break;
				case key::Home : caret_->toLineBegin(glyph_layout, shift);
					break;
				case key::A :{
					if(ctrl){
						caret_->selectAll(glyph_layout);
					}
					break;
				}
				case key::C :{
					if(ctrl && caret_->range.hasRegion()){
						//no other better way to provide a zero terminated string
						get_scene()->set_clipboard(std::string{caret_->range.getSelection(glyph_layout)}.c_str());
					}
					break;
				}
				case key::V :{
					if(ctrl && caret_){
						auto str = get_scene()->get_clipboard();
						insert(std::string{str}, encode::count_code_points(str));
					}
					break;
				}
				case key::X :{
					if(ctrl && caret_->range.hasRegion()){
						get_scene()->set_clipboard(std::string{caret_->range.getSelection(glyph_layout)}.c_str());
						onDelete();
					}
					break;
				}
				case key::Z :{
					if(ctrl){
						if(shift){
							forwardtrace();
						}else{
							backtrace();
						}
					}
					break;
				}
				case key::End : caret_->toLineEnd(glyph_layout, shift);
					break;
				case key::Delete :{
					onDelete();
					break;
				}
				case key::Backspace :{
					onBackspace();
					break;
				}
				case key::Enter :{
					buffer.push_back(U'\n');
					break;
				}
				case key::Tab :{
					buffer.push_back(U'\t');
					break;
				}

				default : break;
				}
				// if(key == key::Right){
				// }
			}
		}

		void input_unicode(const char32_t val) override{
			if(bannedCodePoints.contains(val)){
				set_input_invalid();
				return;
			}
			buffer.push_back(val);
		}

		void draw_content(const rect clipSpace) const override;
		// void drawMain(Rect clipSpace) const override;

	private:
		esc_flag on_esc() override{
			if(elem::on_esc() == esc_flag::fall_through){
				if(caret_){
					caret_ = std::nullopt;
					set_focused_key(false);
					return esc_flag::intercept;
				}
				return esc_flag::fall_through;
			}

			return esc_flag::intercept;
		}

		void record(){
			if(!caret_)return;
			history.push(glyph_layout.text, caret_->range);
		}

		void backtrace(){
			if(const auto prev = history.toPrev()){
				if(set_text_quiet(prev->text)){
					layoutTextThenResume(prev->where);
				}
			}
		}

		void forwardtrace(){
			if(const auto prev = history.toNext()){
				if(set_text_quiet(prev->text)){
					layoutTextThenResume(prev->where);
				}
			}
		}

		void layoutTextThenResume(const caret_range pos){
			text_expired = true;
			layout();
			if(glyph_layout.contains(pos.dst.pos) && glyph_layout.contains(pos.src.pos)){
				if(!caret_){
					caret_.emplace(pos);
				}else{
					caret_->range = pos;
				}
			}
			notifyTextUpdated();
		}

		void insert(std::string&& string, const std::size_t advance){
			if(string.size() + get_text().size() >= maximumUnits){
				set_input_invalid();
				return;
			}

			record();

			caret_->insertAt(glyph_layout, std::move(string));

			layoutTextThenResume(caret_->range);
			if(glyph_layout.is_clipped()){
				backtrace();
				set_input_invalid();
				// glyph_layout.isCompressed = false;
				return;
			}

			caret_->advance(glyph_layout, advance, false);
			if(history.size() > 1)(void)history.toPrev();

			record();
		}

		void onDelete(){
			if(!caret_)return;
			record();
			if(caret_->deleteAt(glyph_layout)){
				layoutTextThenResume(caret_->range);
			}else{
				history.pop();
			}
		}

		void onBackspace(){
			if(!caret_)return;
			record();
			if(caret_->backspaceAt(glyph_layout)){
				layoutTextThenResume(caret_->range);
			}else{
				history.pop();
			}
		}

	public:
		void add_file_banned_characters(){
			bannedCodePoints.insert_range(std::initializer_list{U'<', U'>', U':', U'"', U'/', U'\\', U'|', U'*', U'?'});
		}

		void set_banned_characters(std::initializer_list<char32_t> chars){
			bannedCodePoints = chars;
		}

		[[nodiscard]] std::string_view get_text() const noexcept{
			if(on_hint()){
				return std::string_view{};
			}

			return basic_text_elem::get_text();
		}

		[[nodiscard]] bool on_hint() const noexcept{
			return isHinting;
		}

		[[nodiscard]] bool on_failed() const noexcept{
			return failedHintDuration > 0.f;
		}

	protected:
		void set_input_invalid(){
			failedHintDuration = 10.f;
		}

		void set_input_valid(){
			failedHintDuration = 0.;
		}

		virtual void notifyTextUpdated(){

		}
	};


	// export
	// struct ActiveTextInputArea : TextInputArea{
	// 	std::move_only_function<void(ActiveTextInputArea&, std::string_view)> onTextUpdated{};
	// 	std::move_only_function<bool(ActiveTextInputArea&, std::string_view)> invalidInputChecker{};
	//
	// 	void notifyTextUpdated() override{
	// 		if(onTextUpdated)onTextUpdated(*this, getText());
	// 	}
	//
	// 	void update(float delta_in_ticks) override{
	// 		TextInputArea::update(delta_in_ticks);
	// 		if(!isHinting && invalidInputChecker){
	// 			if(invalidInputChecker(*this, getText())){
	// 				setInputInvalid();
	// 			}else{
	// 				setInputValid();
	// 			}
	// 		}
	// 	}
	// };
}
