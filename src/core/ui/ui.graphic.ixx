//
// Created by Matrix on 2025/3/12.
//

export module mo_yanxi.ui.graphic;

export import mo_yanxi.graphic.renderer.ui;
export import mo_yanxi.graphic.draw;
export import mo_yanxi.graphic.draw.func;
export import mo_yanxi.graphic.draw.multi_region;

namespace mo_yanxi::ui{
	export using draw_acquirer = graphic::draw::ui_acquirer;
	export enum struct draw_layers : unsigned char{
		def = 0,

		light = 0,
		base,
		background,
	};
}
