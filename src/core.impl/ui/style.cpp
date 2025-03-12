module mo_yanxi.ui.style;

import mo_yanxi.ui.elem;

import mo_yanxi.ui.scene;
import mo_yanxi.graphic.draw.func;
// import Core.UI.Graphic;
// import Graphic.Draw.NinePatch;

namespace mo_yanxi{
// 	graphic::color ui::Palette::onInstance(const ui::elem& element) const{
// 		graphic::color color;
// 		if(element.disabled){
// 			color = disabled;
// 		} else if(element.getCursorState().pressed){
// 			color = onPress;
// 		} else if(element.getCursorState().focused){
// 			color = onFocus;
// 		} else if(element.activated){
// 			color = activated;
// 		} else{
// 			color = general;
// 		}
//
// 		return color.mulA(element.graphicProp().getOpacity());
// 	}
//
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
