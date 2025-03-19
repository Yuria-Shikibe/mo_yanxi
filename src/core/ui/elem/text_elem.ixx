//
// Created by Matrix on 2025/3/14.
//

export module mo_yanxi.ui.elem.text_elem;

export import mo_yanxi.ui.elem;
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
	struct basic_text_elem : elem{
	protected:
		const font::typesetting::parser* parser{&font::typesetting::global_parser};
		font::typesetting::glyph_layout glyph_layout{};


		bool text_expired{false};
	public:
		align::pos text_entire_align{align::pos::top_left};

		[[nodiscard]] basic_text_elem(scene* scene, group* group, const std::string_view tyName = "basic_text_elem")
			: elem(scene, group, tyName){
		}

		std::optional<math::vec2> pre_acquire_size(stated_extent extent) override{
			math::vec2 bound{extent.potential_max_size()};

			bound -= property.boarder.get_size();
			bound.max({});

			auto rst = layout_text(bound);
			return rst + property.boarder.get_size();
		}

		void set_text(std::string_view text){
			if(glyph_layout.get_text() != text){
				glyph_layout.set_text(std::string{text});
				text_expired = true;

				mark_independent_layout_changed();

			}
		}

		void set_policy(font::typesetting::layout_policy policy){
			if(glyph_layout.set_policy(policy)){
				text_expired = true;
				mark_independent_layout_changed();

			}
		}

		void layout() override{
			elem::layout();

			if(text_expired){
				const auto maxSz = context_size_restriction.potential_max_size();
				const auto resutlSz = layout_text(maxSz);
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
			text_expired = false;

			glyph_layout.set_clamp_size(bound);
			glyph_layout.clear();
			/*if(!glyph_layout)*/parser->operator()(glyph_layout);
			return glyph_layout.extent();
		}

		[[nodiscard]] std::optional<font::typesetting::layout_pos_t> get_layout_pos(math::vec2 globalPos) const;


		void draw_content(const rect clipSpace, rect redirect) const override;
	};
}


