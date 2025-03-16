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
		bool toLight,
		float opacityScl = 1.f){
		using namespace mo_yanxi;
		using namespace mo_yanxi::graphic;
		color tempColor{};

		acquirer.proj.mode_flag = vk::vertices::mode_flag_bits::sdf /*| vk::vertices::mode_flag_bits::uniformed*/;
		const font::typesetting::glyph_elem* last_elem{};

		for(const auto& row : layout.rows()){
			const auto lineOff = row.src + offset;

			for(auto&& glyph : row.glyphs){
				if(!glyph.glyph) continue;
				last_elem = &glyph;

				acquirer << glyph.glyph.get_cache();

				tempColor = glyph.color;

				if(opacityScl != 1.f){
					tempColor.a *= opacityScl;
				}

				if(glyph.code.code == U'\0'){
					tempColor.mulA(.65f);
				}


				fill::quad(
					acquirer.get(),
					lineOff + glyph.region.vert_00(),
					lineOff + glyph.region.vert_10(),
					lineOff + glyph.region.vert_11(),
					lineOff + glyph.region.vert_01(),
					tempColor.to_light_color_copy(toLight)
				);

				// acquirer << draw::white_region;
				// draw::line::rect_ortho(acquirer, glyph.region.copy().move(lineOff));
			}
		}


		if(layout.is_clipped() && last_elem){
			const auto lineOff = layout.rows().back().src + offset;

			fill::fill(
				acquirer[-1],
				lineOff + last_elem->region.vert_00(),
				lineOff + last_elem->region.vert_10(),
				lineOff + last_elem->region.vert_11(),
				lineOff + last_elem->region.vert_01(),
				last_elem->color.copy().mulA(opacityScl).to_light_color_copy(toLight),
				{},
				{},
				last_elem->color.copy().mulA(opacityScl).to_light_color_copy(toLight)
			);
		}

		acquirer.proj.mode_flag = vk::vertices::mode_flag_bits{};
		acquirer << draw::white_region;
		//
		draw::line::rect_ortho(acquirer, mo_yanxi::math::frect{mo_yanxi::tags::from_extent, offset, layout.extent()}, 2., graphic::colors::YELLOW);



	}
}
