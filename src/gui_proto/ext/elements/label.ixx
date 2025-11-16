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
}
