module mo_yanxi.ui.style;

import mo_yanxi.ui.primitives;

import mo_yanxi.ui.graphic;
import mo_yanxi.graphic.draw.func;
import mo_yanxi.graphic.draw.multi_region;


namespace mo_yanxi{
	graphic::color ui::style::palette::on_instance(const ui::elem& element) const{
		graphic::color color;
		if(element.disabled){
			color = disabled;
		} else if(element.get_cursor_state().pressed){
			color = on_press;
		} else if(element.get_cursor_state().focused){
			color = on_focus;
		} else if(element.activated){
			color = activated;
		} else{
			color = general;
		}

		return color.mul_a(element.gprop().get_opacity());
	}

	void ui::style::round_style::draw(const elem& element, math::frect region, float opacityScl) const{
		using namespace graphic;

		draw_acquirer acquirer{renderer_from_erased(element.get_renderer()).get_batch(), {}};
		acquirer.proj.mode_flag = vk::vertices::mode_flag_bits::sdf;


		acquirer.proj.set_layer(draw_layers::def);
		draw::nine_patch(acquirer, edge, region, edge.pal.on_instance(element).mul_a(opacityScl).mul(element.gprop().style_color_scl));
		draw::nine_patch(acquirer, base, region, base.pal.on_instance(element).mul_a(opacityScl).mul(element.gprop().style_color_scl));

		acquirer.proj.set_layer(draw_layers::background);
		draw::nine_patch(acquirer, back, region, back.pal.on_instance(element).mul_a(opacityScl));
	}

	float ui::style::round_style::content_opacity(const elem& element) const{
		if(element.disabled){
			return disabledOpacity;
		} else{
			return style_drawer::content_opacity(element);
		}
	}

	bool ui::style::round_style::apply_to(elem& element) const{
		return util::try_modify(element.prop().boarder, boarder);
	}

	// 	void Core::UI::Style::RoundStyle::draw(const Element& element, const Geom::Rect_Orthogonal<float> region,
// 	                                       const float opacityScl) const{
// 		using namespace Graphic;
//
// 		AutoParam param{element.getBatch()};
//
// 		const auto rect = region;
//
// 		Draw::drawNinePatch(param, edge, rect, edge.palette.onInstance(element).mulA(opacityScl));
//
// 		param.modifier.setLayer(UI_DrawLayer::background);
// 		Draw::drawNinePatch(param, base, rect, base.palette.onInstance(element).mulA(opacityScl));
// #if DEBUG_CHECK
// 		// param << Draw::WhiteRegion;
// 		// Draw::Drawer<Vulkan::Vertex_UI>::Line::rectOrtho(++param, 1.f, element.prop().getValidBound_absolute(), graphic::colors::PINK);
// #endif
// 	}
//
// 	float Core::UI::Style::RoundStyle::contentOpacity(const Element& element) const{
// 		if(element.disabled){
// 			return disabledOpacity;
// 		} else{
// 			return StyleDrawer::contentOpacity(element);
// 		}
// 	}
//
// 	bool Core::UI::Style::RoundStyle::applyTo(Element& element) const{
// 		return Util::tryModify(element.prop().boarder, boarder);
// 	}
}
