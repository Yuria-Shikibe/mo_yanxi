export module mo_yanxi.ui.action.generic;

import mo_yanxi.ui.primitives;
export import mo_yanxi.ui.action;
import mo_yanxi.graphic.color;

namespace mo_yanxi::ui::action{
	export
	struct color_action final : action<elem>{
	private:
		graphic::color initialColor{};

	public:
		graphic::color dst_color{};

		color_action(const float lifetime, const graphic::color& dst_color)
			: action<elem>(lifetime),
			  dst_color(dst_color){}

		color_action(const float lifetime, const graphic::color& dst_color, const interp_type& interpFunc)
			: action<elem>(lifetime, interpFunc),
			  dst_color(dst_color){}

	protected:
		void apply(elem& elem, const float progress) override{
			elem.prop().graphic_data.style_color_scl = initialColor.create_lerp(dst_color, progress);
		}

		void begin(elem& elem) override{
			initialColor = elem.prop().graphic_data.style_color_scl;
		}

		void end(elem& elem) override{
			elem.prop().graphic_data.style_color_scl = dst_color;
		}
	};

	export
	struct alpha_action final : action<elem>{
	private:
		float initialAlpha{};

	public:
		float dst_alpha{};

		alpha_action(const float lifetime, const float endAlpha, const interp_type& interpFunc)
			: action<elem>(lifetime, interpFunc),
			  dst_alpha(endAlpha){}

		alpha_action(const float lifetime, const float endAlpha)
			: action<elem>(lifetime),
			  dst_alpha(endAlpha){}

		void apply(elem& elem, const float progress) override{
			elem.update_opacity(math::lerp(initialAlpha, dst_alpha, progress));
		}

		void begin(elem& elem) override{
			initialAlpha = elem.gprop().get_opacity();
		}

		void end(elem& elem) override{
			elem.update_opacity(dst_alpha);
		}
	};

	export
	struct remove_action : action<elem>{
		remove_action() = default;

		void begin(elem& elem) override{
			elem.remove_self_from_parent();
		}
	};
}

