module mo_yanxi.gui.elem.label;

import mo_yanxi.gui.renderer.frontend;
import mo_yanxi.graphic.draw.instruction;

namespace mo_yanxi::graphic::draw{
using namespace instruction;
void glyph_layout(
	const gui::renderer_frontend& renderer,
	const font::typesetting::glyph_layout& layout,
	const math::vec2 offset,
	const float opacityScl = 1.f){
	using namespace mo_yanxi;
	using namespace mo_yanxi::graphic;
	color tempColor{};

	for(const auto& row : layout.rows()){
		const auto lineOff = row.src + offset;
		for(auto&& glyph : row.glyphs){
			if(!glyph.glyph) continue;

			tempColor = glyph.color;

			if(opacityScl != 1.f){
				tempColor.a *= opacityScl;
			}

			if(glyph.code.code == U'\0'){
				tempColor.mul_a(.65f);
			}

			const auto region = glyph.get_draw_bound().move(lineOff);

			renderer.push(rectangle_ortho{
				.generic = {
					.image = glyph.glyph.get_cache().view,
				},
				.v00 = region.vert_00(),
				.v11 = region.vert_11(),
				.uv00 = glyph.glyph.get_cache().uv.v00(),
				.uv11 = glyph.glyph.get_cache().uv.v11(),
				.vert_color = tempColor
			});
		}
	}
}

void glyph_layout(
	const gui::renderer_frontend& renderer,
	const font::typesetting::glyph_layout& layout,
	const math::vec2 offset,
	const graphic::color color_scl,
	const float opacityScl = 1.f){
	using namespace mo_yanxi;
	using namespace mo_yanxi::graphic;
	color tempColor{};

	for(const auto& row : layout.rows()){
		const auto lineOff = row.src + offset;
		for(auto&& glyph : row.glyphs){
			if(!glyph.glyph) continue;

			tempColor = glyph.color * color_scl;

			if(opacityScl != 1.f){
				tempColor.a *= opacityScl;
			}

			if(glyph.code.code == U'\0'){
				tempColor.mul_a(.65f);
			}

			const auto region = glyph.get_draw_bound().move(lineOff);

			renderer.push(rectangle_ortho{
				.generic = {
					.image = glyph.glyph.get_cache().view,
				},
				.v00 = region.vert_00(),
				.v11 = region.vert_11(),
				.uv00 = glyph.glyph.get_cache().uv.v00(),
				.uv11 = glyph.glyph.get_cache().uv.v11(),
				.vert_color = tempColor
			});
		}
	}
}
}

namespace mo_yanxi::gui{
std::optional<mo_yanxi::font::typesetting::layout_pos_t> label::get_layout_pos(
	const math::vec2 globalPos) const{
	if(glyph_layout.empty() || !contains(globalPos)){
		return std::nullopt;
	}

	using namespace font::typesetting;
	auto textLocalPos = globalPos - get_glyph_abs_src();

	auto row =
		std::ranges::lower_bound(
			glyph_layout.rows(), textLocalPos.y,
			{}, &glyph_layout::row::bottom);


	if(row == glyph_layout.rows().end()){
		if(glyph_layout.rows().empty()){
			return std::nullopt;
		} else{
			row = std::ranges::prev(glyph_layout.rows().end());
		}
	}

	auto elem = row->line_nearest(textLocalPos.x);

	if(elem == row->glyphs.end() && !row->glyphs.empty()){
		elem = std::prev(row->glyphs.end());
	}

	return layout_pos_t{
			static_cast<layout_pos_t::value_type>(std::ranges::distance(row->glyphs.begin(), elem)),
			static_cast<layout_pos_t::value_type>(std::ranges::distance(glyph_layout.rows().begin(), row))
		};
}

void label::draw_text() const{
	if(text_color_scl){
		graphic::draw::glyph_layout(get_scene().renderer(), glyph_layout, get_glyph_abs_src(), *text_color_scl,
			get_draw_opacity() * (disabled ? 0.3f : 1.f));
	} else{
		graphic::draw::glyph_layout(get_scene().renderer(), glyph_layout, get_glyph_abs_src(),
			get_draw_opacity() * (disabled ? 0.3f : 1.f));
	}
}

void label::draw_content(const rect clipSpace) const{
	draw_background();
	draw_text();
}
}
