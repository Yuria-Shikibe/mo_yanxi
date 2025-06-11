//
// Created by Matrix on 2025/5/11.
//
export module mo_yanxi.ui.creation.generic;

import mo_yanxi.graphic.draw;
import mo_yanxi.ui.layout.cell;
import mo_yanxi.ui.elem.image_frame;
import mo_yanxi.ui.elem.canvas;
import mo_yanxi.ui.assets;

import std;

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

	export
	template <std::invocable<const elem&, rect> Fn>
	struct general_canvas : cell_creator_base<canvas<Fn>>{

	private:
		using base = cell_creator_base<canvas<Fn>>;
	public:
		Fn drawer;

		[[nodiscard]] explicit(false) general_canvas(const Fn& drawer)
			: drawer(drawer){
		}
		[[nodiscard]] explicit(false) general_canvas(Fn&& drawer)
			: drawer(std::move(drawer)){
		}

		elem_ptr operator()(scene* s, group* g) && {
			return elem_ptr{s, g, std::in_place_type<typename base::elem_type>, std::move(drawer)};
		}

		elem_ptr operator()(scene* s, group* g) const & requires (std::is_copy_constructible_v<Fn>) {
			return elem_ptr{s, g, std::in_place_type<typename base::elem_type>, std::move(drawer)};
		}
	};
}
