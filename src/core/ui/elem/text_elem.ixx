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
		[[nodiscard]] basic_text_elem(scene* scene, group* group, const std::string_view tyName = "basic_text_elem")
			: elem(scene, group, tyName){
		}

		std::optional<math::vec2> pre_acquire_size(stated_extent extent) override{
			math::vec2 bound{extent.potential_max_size()};

			// if(extent.width.dependent()){
			// 	bound.x = std::numeric_limits<float>::infinity();
			// }else{
			// 	bound.x = extent.width;
			// }
			//
			// if(extent.height.dependent()){
			// 	bound.y = std::numeric_limits<float>::infinity();
			// }else{
			// 	bound.y = extent.height;
			// }
			//
			// bound.min(max_bound);
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
			if(text_expired){
				auto maxSz = context_size_restriction.potential_max_size();
				auto resutlSz = layout_text(maxSz);
				if(resutlSz.beyond(content_size())){
					notify_layout_changed(spread_direction::from_content);
				}
			}
			elem::layout();
		}

		bool resize(const math::vec2 size) override{
			if(elem::resize(size)){
				layout_text(content_size());
				return true;
			}
			return false;
		}
	protected:
		math::vec2 layout_text(math::vec2 bound){
			text_expired = false;

			glyph_layout.set_clamp_size(bound);
			glyph_layout.clear();
			/*if(!glyph_layout)*/parser->operator()(glyph_layout);
			return glyph_layout.extent();
		}

		void draw_content(const rect clipSpace, rect redirect) const override;
	};
}


