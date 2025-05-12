//
// Created by Matrix on 2025/5/11.
//
export module mo_yanxi.ui.creation.seperator_line;

import mo_yanxi.graphic.draw;
import mo_yanxi.ui.layout.cell;
import mo_yanxi.ui.elem.image_frame;
import mo_yanxi.ui.assets;

namespace mo_yanxi::ui::creation{
	export struct general_seperator_line : cell_creator_base<single_image_frame<image_caped_region_drawable>>{

		float stroke = 20;
		float scale = .05f;
		style::palette palette{style::general_palette};
		bool vertical{false};

		void operator()(elem_type& frame) const{
			frame.set_style();
			frame.style.palette = palette;
			if(vertical){
				frame.style.scaling = align::scale::stretchY;
			} else{
				frame.style.scaling = align::scale::stretchX;
			}

			frame.set_drawable({
					graphic::draw::mode_flags::sdf,
					theme::shapes::line, scale
				});
		}

		void operator()(const cell_create_result<elem_type, mastering_cell> rst) const noexcept{
			if(vertical){
				rst.cell.set_width(stroke);
			} else{
				rst.cell.set_height(stroke);
			}
			rst.cell.saturate = true;
		}
	};
}
