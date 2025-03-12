//
// Created by Matrix on 2024/10/7.
//

export module mo_yanxi.ui.comp.drawer;

import mo_yanxi.math.rect_ortho;

namespace mo_yanxi::ui{
	export inline constexpr float BoarderStroke = 8.;

	export
	template <typename T>
	struct style_drawer{
		virtual ~style_drawer() = default;

		virtual void draw(const T& element, math::frect region, float opacityScl) const = 0;

		[[nodiscard]] virtual float content_opacity(const T& element) const{
			return 1.0f;
		}

		/**
		 * @return true if element changed
		 */
		virtual bool apply_to(T& element) const{
			return false;
		}
	};
}
