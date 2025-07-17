module;

#include <cassert>

export module mo_yanxi.ui.graphic;

export import mo_yanxi.graphic.renderer.ui;
export import mo_yanxi.graphic.draw;
export import mo_yanxi.graphic.draw.func;
export import mo_yanxi.graphic.draw.multi_region;

namespace mo_yanxi::ui{
	export using draw_acquirer = graphic::draw::ui_acquirer;

	export
	[[nodiscard]] draw_acquirer get_draw_acquirer(graphic::renderer_ui& renderer) noexcept{
		return draw_acquirer{renderer.batch};
	}

	export
	[[nodiscard]] draw_acquirer get_draw_acquirer(graphic::renderer_ui_ref renderer) noexcept{
		return get_draw_acquirer(graphic::renderer_from_erased(renderer));
	}

	export
	[[nodiscard]] draw_acquirer get_draw_acquirer(graphic::renderer_ui_ptr renderer) noexcept{
		assert(renderer != nullptr);
		return get_draw_acquirer(*renderer);
	}

	export enum struct draw_layers : unsigned char{
		def = 0,

		light = 0,
		base,
		background,
	};
}
