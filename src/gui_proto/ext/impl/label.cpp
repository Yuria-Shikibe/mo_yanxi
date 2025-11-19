module mo_yanxi.gui.elem.label;

import mo_yanxi.gui.renderer.frontend;
import mo_yanxi.graphic.draw.instruction;

namespace mo_yanxi::graphic::draw{
using namespace instruction;
void glyph_layout(
gui::renderer_frontend& renderer,
	const font::typesetting::glyph_layout& layout,
	const math::vec2 offset,
	const float opacityScl = 1.f){
	using namespace mo_yanxi;
	using namespace mo_yanxi::graphic;
	color tempColor{};

	gui::mode_guard _{renderer, gui::draw_mode_param{gui::draw_mode::msdf}};
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
					.image = glyph.glyph->view,
				},
				.v00 = region.vert_00(),
				.v11 = region.vert_11(),
				.uv00 = glyph.glyph->uv.v00(),
				.uv11 = glyph.glyph->uv.v11(),
				.vert_color = tempColor
			});
		}
	}
}

void glyph_layout(
gui::renderer_frontend& renderer,
	const font::typesetting::glyph_layout& layout,
	const math::vec2 offset,
	const graphic::color color_scl,
	const float opacityScl = 1.f){
	using namespace mo_yanxi;
	using namespace mo_yanxi::graphic;
	color tempColor{};

	gui::mode_guard _{renderer, gui::draw_mode_param{gui::draw_mode::msdf}};
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
					.image = glyph.glyph->view,
				},
				.v00 = region.vert_00(),
				.v11 = region.vert_11(),
				.uv00 = glyph.glyph->uv.v00(),
				.uv11 = glyph.glyph->uv.v11(),
				.vert_color = tempColor
			});
		}
	}
}

void glyph_layout_scaled(
gui::renderer_frontend& renderer,
	const font::typesetting::glyph_layout& layout,
	std::invocable<color&> auto&& color_modifier
){
	using namespace mo_yanxi;
	using namespace mo_yanxi::graphic;
	gui::mode_guard _{renderer, gui::draw_mode_param{gui::draw_mode::msdf}};

	color tempColor{};
	for(const auto& row : layout.rows()){
		const auto lineOff = row.src;
		for(auto&& glyph : row.glyphs){
			if(!glyph.glyph) continue;
			tempColor = glyph.color;

			std::invoke(color_modifier, tempColor);

			if(glyph.code.code == U'\0'){
				tempColor.mul_a(.65f);
			}

			const auto region = glyph.get_draw_bound().move(lineOff);

			renderer.push(rectangle_ortho{
				.generic = {
					.image = glyph.glyph->view,
				},
				.v00 = region.vert_00(),
				.v11 = region.vert_11(),
				.uv00 = glyph.glyph->uv.v00(),
				.uv11 = glyph.glyph->uv.v11(),
				.vert_color = tempColor
			});
		}
	}
}


}

namespace mo_yanxi::gui{



void record_glyph_draw_instructions(
	instr_buffer& buffer,
	const font::typesetting::glyph_layout& layout,
	graphic::color color_scl
	){
	using namespace mo_yanxi::graphic;
	color tempColor{};

	static constexpr auto instr_sz = draw::instruction::get_instr_size<draw::instruction::rectangle_ortho>();
	buffer.resize(std::ranges::distance(layout.all_glyphs()) * instr_sz);
	if(buffer.capacity() > buffer.size() * 4){
		buffer.shrink_to_fit();
	}

	std::byte* cur = buffer.data();
	for(const auto& row : layout.rows()){
		const auto lineOff = row.src;
		for(auto&& glyph : row.glyphs){
			if(!glyph.glyph) continue;
			tempColor = glyph.color * color_scl;

			if(glyph.code.code == U'\0'){
				tempColor.mul_a(.65f);
			}

			const auto region = glyph.get_draw_bound().move(lineOff);

			draw::instruction::place_instruction_at(cur, draw::instruction::rectangle_ortho{
				.generic = {
					.image = glyph.glyph->view,
				},
				.v00 = region.vert_00(),
				.v11 = region.vert_11(),
				.uv00 = glyph.glyph->uv.v00(),
				.uv11 = glyph.glyph->uv.v11(),
				.vert_color = tempColor
			});
			cur += instr_sz;
		}
	}

	const auto actual = (cur - buffer.data());
	buffer.resize(actual);
}


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
	math::mat3 mat;
	if(fit_){
		const auto reg_ext = align::embed_to(align::scale::fit, glyph_layout.extent(), content_extent().min(max_fit_scale_bound));
		mat.set_rect_transform({}, glyph_layout.extent(), get_glyph_abs_src(), reg_ext);
	}else{
		mat = math::mat3_idt;
		mat.set_translation(get_glyph_abs_src());
	}

	auto& renderer = get_scene().renderer();
	transform_guard _t{renderer, mat};

	const float opacity = get_draw_opacity() * (disabled ? 0.3f : 1.f);
	if(text_color_scl){
		graphic::draw::glyph_layout_scaled(get_scene().renderer(), glyph_layout, [&](graphic::color& color){
			color.mul_a(opacity);
			color *= *text_color_scl;
		});
	} else{
		graphic::draw::glyph_layout_scaled(get_scene().renderer(), glyph_layout, [&](graphic::color& color){
			color.mul_a(opacity);
		});
	}
}

void label::draw_content(const rect clipSpace) const{
	draw_background();
	draw_text();
}



void async_label_terminal::on_update(const font::typesetting::glyph_layout* const& data){
	if(data->extent().beyond(label->content_extent())){
		label->notify_layout_changed(propagate_mask::local | propagate_mask::force_upper);
	}

	label->update_draw_buffer(*data);
}

void async_label::draw_content(const rect clipSpace) const{
	draw_background();
	if(!terminal)return;
	auto opt_ptr = terminal->request(false);
	if(!opt_ptr)return;

	auto& layout = **opt_ptr;
	auto& renderer = get_scene().renderer();

	using namespace graphic;
	using namespace graphic::draw::instruction;

	gui::mode_guard _m{renderer, gui::draw_mode_param{gui::draw_mode::msdf}};

	math::mat3 mat;
	const auto reg_ext = align::embed_to(align::scale::fit, layout.extent(), content_extent());
	mat.set_rect_transform({}, layout.extent(), content_src_pos_abs(), reg_ext);
	gui::transform_guard _t{renderer, mat};

	renderer.push_same_instr(draw_instr_buffer_, get_instr_size<rectangle_ortho>());
}


}
