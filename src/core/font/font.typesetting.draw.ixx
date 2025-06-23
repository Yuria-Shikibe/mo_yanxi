export module mo_yanxi.font.typesetting.draw;

export import mo_yanxi.font.typesetting;
export import mo_yanxi.graphic.draw.func;
export import mo_yanxi.graphic.draw;
export import mo_yanxi.graphic.renderer.ui;

import std;

namespace mo_yanxi::graphic::draw{
	export void glyph_layout(
		draw::ui_acquirer acquirer,
		const font::typesetting::glyph_layout& layout,
		const math::vec2 offset,
		float opacityScl = 1.f){
		using namespace mo_yanxi;
		using namespace mo_yanxi::graphic;
		color tempColor{};

		acquirer.proj.mode_flag = vk::vertices::mode_flag_bits::sdf | vk::vertices::mode_flag_bits::uniformed;
		const font::typesetting::glyph_elem* last_elem{};

		for(const auto& row : layout.rows()){
			const auto lineOff = row.src + offset;

			// acquirer << draw::white_region;
			// draw::line::line_ortho(acquirer.get(), lineOff, lineOff.copy().add_x(row.bound.width));
			// draw::line::rect_ortho(acquirer, row.getRectBound().move(offset));

			for(auto&& glyph : row.glyphs){
				if(!glyph.glyph) continue;
				last_elem = &glyph;

				// acquirer << draw::white_region;
				// draw::line::rect_ortho(acquirer, glyph.region.copy().move(lineOff));

				acquirer << glyph.glyph.get_cache();

				tempColor = glyph.color;

				if(opacityScl != 1.f){
					tempColor.a *= opacityScl;
				}

				if(glyph.code.code == U'\0'){
					tempColor.mul_a(.65f);
				}

				const auto region = glyph.get_draw_bound().move(lineOff);

				fill::quad(
					acquirer.get(),
					region.vert_00(),
					region.vert_10(),
					region.vert_11(),
					region.vert_01(),
					tempColor
				);

			}
		}


		if(layout.is_clipped() && last_elem){
			const auto lineOff = layout.rows().back().src + offset;
			const auto region = last_elem->get_draw_bound().move(lineOff);
			fill::fill(
				acquirer[-1],
				region.vert_00(),
				region.vert_10(),
				region.vert_11(),
				region.vert_01(),
				last_elem->color.copy().mul_a(opacityScl),
				{},
				{},
				last_elem->color.copy().mul_a(opacityScl)
			);
		}

		// acquirer.proj.mode_flag = vk::vertices::mode_flag_bits{};
		// acquirer << draw::white_region;
		// //
		// draw::line::rect_ortho(acquirer, mo_yanxi::math::frect{mo_yanxi::tags::from_extent, offset, layout.extent()}, 2., graphic::colors::YELLOW);



	}
}
