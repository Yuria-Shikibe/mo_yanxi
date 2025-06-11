

export module mo_yanxi.ui.elem.check_box;

export import mo_yanxi.ui.elem.image_frame;
import mo_yanxi.ui.table;
import mo_yanxi.ui.elem.button;
import std;

//TODO move styles to other place?
import mo_yanxi.ui.assets;


namespace mo_yanxi::ui{
	export
	struct check_box : image_frame {
		[[nodiscard]] check_box(scene* s, group* g)
			: image_frame(s, g){

			cursor_state.registerDefEvent(events());
			interactivity = interactivity::enabled;
			property.maintain_focus_until_mouse_drop = true;

		}

		input_event::click_result on_click(const input_event::click click_event) override{
			if(drawables_.empty())return input_event::click_result::intercepted;

			elem::on_click(click_event);

			if(click_event.code.action() != core::ctrl::act::release)return input_event::click_result::intercepted;
			if(!contains(click_event.pos))return input_event::click_result::intercepted;

			switch(click_event.code.key()){
			case core::ctrl::mouse::LMB:
				current_frame_index = (current_frame_index + 1) % drawables_.size();
				break;
			case core::ctrl::mouse::RMB:
				if(has_tooltip_builder()){
					if(has_tooltip())tooltip_notify_drop();
					else build_tooltip();
				}
				break;
			default:
				break;
			}

			return input_event::click_result::intercepted;
		}

		void add_multi_select_tooltip(tooltip_layout_info layout){
			set_tooltip_state({
				.layout_info = layout,
				.use_stagnate_time = false,
				.auto_release = true,
				.min_hover_time = tooltip_create_info::disable_auto_tooltip
			}, [](check_box& owner, table& table){
				for (const auto & [idx, drawable] : owner.drawables_ | std::views::enumerate){
					auto img = table.emplace<button<image_frame>>();
					img.cell().set_pad({.left = 4, .right = 4}).set_size(96);

					img->set_style(theme::styles::no_edge);
					img->set_drawable<drawable_ref>(drawable.drawable.get());
					img->set_button_callback(button_tags::general, [&owner, idx](elem& elem){
						owner.current_frame_index = idx;
						owner.tooltip_notify_drop();
					});

					img->checkers.setActivatedProv([&owner, idx]{
						return owner.get_frame_index() == idx;
					});
				}

				table.set_edge_pad(0);
			});
		}

	private:
		// void notify_checkbox_event(){
		//
		// }
	};
}
