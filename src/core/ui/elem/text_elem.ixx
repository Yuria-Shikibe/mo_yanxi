//
// Created by Matrix on 2025/3/14.
//

export module mo_yanxi.ui.elem.label;

export import mo_yanxi.ui.primitives;
export import mo_yanxi.font;
export import mo_yanxi.font.typesetting;
import std;

namespace mo_yanxi::ui{
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
		align::pos text_entire_align{align::pos::top_left};

		[[nodiscard]] label(scene* scene, group* group)
			: elem(scene, group){
			interactivity = interactivity::disabled;
		}

		std::optional<math::vec2> pre_acquire_size_impl(optional_mastering_extent extent) override{
			auto ext = layout_text(extent.potential_extent());
			extent.apply(ext);
			return extent.potential_extent();
		}

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
				if(policy != font::typesetting::layout_policy::reserve)fit = false;
				text_expired = true;
				notify_isolated_layout_changed();
			}
		}

		void set_fit(){
			if(!fit){
				set_policy(font::typesetting::layout_policy::reserve);
				glyph_layout.set_clamp_size(math::vectors::constant2<float>::inf_positive_vec2);

				fit = true;

				text_expired = true;
				notify_isolated_layout_changed();
			}
		}

		void set_scale(float scale){
			if(util::try_modify(this->scale, scale)){
				text_expired = true;
				notify_isolated_layout_changed();
			}
		}

		void layout() override{
			elem::layout();

			if(text_expired){
				const auto maxSz = context_size_restriction.potential_extent();
				const auto resutlSz = layout_text(property.evaluate_valid_size(maxSz));
				if(!resutlSz.equals(content_size())){
					notify_layout_changed(spread_direction::all_visible);
				}

				// if(resutlSz.beyond(content_size())){

				// }
			}
		}

		bool resize(const math::vec2 size) override{
			if(elem::resize(size)){
				layout_text(content_size());
				return true;
			}
			return false;
		}

		[[nodiscard]] std::string_view get_text() const noexcept{
			return glyph_layout.get_text();
		}
	protected:

		bool set_text_quiet(std::string_view text){
			if(glyph_layout.get_text() != text){
				glyph_layout.set_text(std::string{text});
				return true;
			}
			return false;
		}


		[[nodiscard]] math::vec2 get_glyph_abs_src() const noexcept{
			const auto sz = glyph_layout.extent();
			const auto rst = align::get_offset_of(text_entire_align, sz, property.content_bound_absolute());

			return rst;
		}

		math::vec2 layout_text(math::vec2 bound){
			if(fit){
				if(text_expired){
					glyph_layout.clear();
					parser->operator()(glyph_layout, scale);
					text_expired = false;
				}

				const auto sz = glyph_layout.extent();
				const auto scaled = align::get_fit_embed_scale(sz, bound.copy().max({1, 1}));
				glyph_layout.scale(scaled);

				auto [w, h] = glyph_layout.extent();
				math::vec2 rst{
					std::isfinite(bound.x) ? bound.x : w,
					std::isfinite(bound.y) ? bound.y : h
				};
				return rst;
			}else{
				//TODO loose the relayout requirement
				if(text_expired){
					glyph_layout.set_clamp_size(bound);
					glyph_layout.clear();
					parser->operator()(glyph_layout, scale);
					text_expired = false;
				}else{
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

		void draw_content(const rect clipSpace) const override;
	};
}


