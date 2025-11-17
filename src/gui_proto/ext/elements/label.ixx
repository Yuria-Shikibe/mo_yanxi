export module mo_yanxi.gui.elem.label;

export import mo_yanxi.gui.infrastructure;
export import mo_yanxi.font;
export import mo_yanxi.font.typesetting;
import std;

namespace mo_yanxi::gui{
struct text_style{
	font::typesetting::layout_policy policy;
	align::pos align;
	float scale;
};

export
struct label : elem{
protected:
	const font::typesetting::parser* parser{&font::typesetting::global_parser};
	font::typesetting::glyph_layout glyph_layout{};

	float scale{1.f};
	bool text_expired{false};
	bool fit{};

public:
	std::optional<graphic::color> text_color_scl{};
	align::pos text_entire_align{align::pos::top_left};

	using elem::elem;

	void set_text(std::string_view text){
		if(glyph_layout.get_text() != text){
			glyph_layout.set_text(std::string{text});
			text_expired = true;
			notify_isolated_layout_changed();
		}
	}

	void set_text(const char* text){
		set_text(std::string_view{text});
	}

	void set_text(std::string&& text){
		if(glyph_layout.get_text() != text){
			glyph_layout.set_text(std::move(text));
			text_expired = true;
			notify_isolated_layout_changed();
		}
	}

	void set_policy(font::typesetting::layout_policy policy){
		if(glyph_layout.set_policy(policy)){
			if(policy != font::typesetting::layout_policy::reserve) fit = false;
			text_expired = true;
			notify_isolated_layout_changed();
		}
	}

	void set_fit(float max_scale){
		if(!fit){
			set_policy(font::typesetting::layout_policy::reserve);
			glyph_layout.set_clamp_size(math::vectors::constant2<float>::inf_positive_vec2);

			fit = true;
			scale = max_scale;

			text_expired = true;
			notify_isolated_layout_changed();
		}
	}

	void set_fit(){
		set_fit(std::numeric_limits<float>::infinity());
	}

	void set_scale(float scale){
		if(util::try_modify(this->scale, scale)){
			text_expired = true;
			notify_isolated_layout_changed();
		}
	}

	void layout_elem() override{
		elem::layout_elem();

		if(text_expired){
			auto maxSz = restriction_extent.potential_extent();
			const auto resutlSz = layout_text(maxSz.fdim(boarder_extent()));
			if(!resutlSz.equals(content_extent())){
				notify_layout_changed(propagate_mask::force_upper);
			}
		}
	}

	[[nodiscard]] std::string_view get_text() const noexcept{
		return glyph_layout.get_text();
	}

protected:
	void draw_content(const rect clipSpace) const override;

	bool resize_impl(const math::vec2 size) override{
		if(elem::resize_impl(size)){
			layout_text(content_extent());
			return true;
		}
		return false;
	}

	std::optional<math::vec2> pre_acquire_size_impl(layout::optional_mastering_extent extent) override{
		auto ext = layout_text(extent.potential_extent());
		extent.apply(ext);
		return extent.potential_extent();
	}

	bool set_text_quiet(std::string_view text){
		if(glyph_layout.get_text() != text){
			glyph_layout.set_text(std::string{text});
			return true;
		}
		return false;
	}

	[[nodiscard]] math::vec2 get_glyph_abs_src() const noexcept{
		const auto sz = glyph_layout.extent();
		const auto rst = align::get_offset_of(text_entire_align, sz, content_bound_abs());

		return rst;
	}

	math::vec2 layout_text(math::vec2 bound){
		if(bound.area() < 1) return {};
		if(fit){
			if(text_expired){
				glyph_layout.clear();
				parser->operator()(glyph_layout, 1);
				text_expired = false;
			}

			const auto sz = glyph_layout.extent();
			const auto scaled = align::get_fit_embed_scale(sz, bound.copy().max({1, 1}));
			const auto srcsz = glyph_layout.extent();
			glyph_layout.scale(std::min(scaled, scale));

			auto [w, h] = srcsz * scaled;
			math::vec2 rst{
					std::isfinite(bound.x) ? bound.x : w,
					std::isfinite(bound.y) ? bound.y : h
				};
			return rst;
		} else{
			//TODO loose the relayout requirement
			if(text_expired){
				glyph_layout.set_clamp_size(bound);
				glyph_layout.clear();
				parser->operator()(glyph_layout, scale);
				text_expired = false;
			} else{
				if(glyph_layout.get_clamp_size() != bound){
					glyph_layout.set_clamp_size(bound);
					if(!glyph_layout.extent().equals(bound)){
						glyph_layout.clear();
					}
					if(glyph_layout.empty()){
						parser->operator()(glyph_layout, scale);
					}
				}
			}
		}

		return glyph_layout.extent();
	}


	[[nodiscard]] std::optional<font::typesetting::layout_pos_t> get_layout_pos(math::vec2 globalPos) const;

	void draw_text() const;
};

export struct async_label;

export
struct async_label_terminal : react_flow::terminal<const font::typesetting::glyph_layout*>{
	async_label* label;

	[[nodiscard]] explicit async_label_terminal(async_label& label)
	: label(std::addressof(label)){
	}

	void on_update(const font::typesetting::glyph_layout* const& data) override;
};


struct async_label : elem{
private:
	bool scale_text_to_fit{true};
	async_label_terminal* terminal{};

	layout::expand_policy expand_policy_{};

public:
	std::optional<graphic::color> text_color_scl{};
	align::pos text_entire_align{align::pos::top_left};

	[[nodiscard]] async_label(scene& scene, elem* parent)
	: elem(scene, parent), terminal{&*get_scene().request_react_node<async_label_terminal>(*this)}{
	}

	~async_label() override{
		if(terminal)get_scene().erase_independent_react_node(*terminal, true);
	}

	void set_fit(bool is_scale_text_to_fit){
		if(util::try_modify(scale_text_to_fit, is_scale_text_to_fit)){
			notify_layout_changed(propagate_mask::force_upper);
		}
	}

protected:
	void draw_content(const rect clipSpace) const override;

public:
	void set_dependency(react_flow::node& glyph_layout_prov){
		if(terminal->get_inputs()[0] != std::addressof(glyph_layout_prov)){
			terminal->disconnect_self_from_context();
			terminal->connect_predecessor(glyph_layout_prov);
			notify_layout_changed(propagate_mask::all);
		}
	}

	// void layout_elem() override{
	// 	auto& layout = *terminal->request(true).value();
	// }

protected:

	std::optional<math::vec2> pre_acquire_size_impl(layout::optional_mastering_extent extent) override{
		if(!terminal)return std::nullopt;
		auto& layout = *terminal->request(true).value();
		return extent.potential_extent().inf_to0().max(layout.extent());
	}
};

export
struct label_layout_node : react_flow::modifier_transient<const font::typesetting::glyph_layout*/*, math::vec2*/, std::string>{
private:
	const font::typesetting::parser* parser_{&font::typesetting::global_parser};
	font::typesetting::glyph_layout layout_{};

public:
	[[nodiscard]] label_layout_node()
	: modifier_transient(react_flow::async_type::async_latest){
	}

protected:
	react_flow::request_pass_handle<const font::typesetting::glyph_layout*> request_raw(bool allow_expired) override{
		if(get_dispatched() > 0 && !allow_expired){
			return std::unexpected{react_flow::data_state::expired};
		}
		return react_flow::request_pass_handle<const font::typesetting::glyph_layout*>{&layout_};
	}

	std::optional<const font::typesetting::glyph_layout*> operator()(
		const std::stop_token& stop_token/*, const math::vec2& bound*/, const std::string& str) override{

		if(layout_.get_text() != str){
			layout_.set_text(std::string{str});
			layout_.clear();
		}

		layout_.set_clamp_size(math::vectors::constant2<float>::inf_positive_vec2);

		// if(layout_.get_clamp_size() != bound){
		// 	layout_.set_clamp_size(bound);
		// 	if(!layout_.extent().equals(bound)){
		// 		layout_.clear();
		// 	}
		// }

		if(stop_token.stop_requested())return std::nullopt;

		if(layout_.empty()){
			parser_->operator()(layout_, 1);
		}
		return std::addressof(layout_);
	}
};
}
