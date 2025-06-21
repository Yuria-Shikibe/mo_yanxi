//
// Created by Matrix on 2025/6/11.
//

export module mo_yanxi.ui.elem.canvas;

export import mo_yanxi.ui.basic;
import std;

namespace mo_yanxi::ui{
	export
	template <std::invocable<const elem&, rect> DFn>
	struct canvas : elem{
		DFn fn;

		template <typename... Args>
		[[nodiscard]] canvas(scene* scene, group* group, Args&& ...args)
			: elem(scene, group, "canvas"), fn(std::forward<Args>(args)...){
			interactivity = interactivity::disabled;
		}

		void draw_content(const rect clipSpace) const override{
			elem::draw_content(clipSpace);
			std::invoke(fn, *this, clipSpace);
		}
	};
}