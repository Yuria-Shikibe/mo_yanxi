

export module mo_yanxi.ui.elem.check_box;

export import mo_yanxi.ui.elem.image_frame;
import mo_yanxi.ui.table;
import mo_yanxi.ui.elem.button;
import std;

namespace mo_yanxi::ui{
	export
	struct check_box : image_frame {
		[[nodiscard]] check_box(scene* s, group* g)
			: image_frame(s, g){

			cursor_state.registerDefEvent(events());
			interactivity = interactivity::enabled;
			property.maintain_focus_until_mouse_drop = true;

			register_event([](const events::click& e, check_box& self){
				if(self.drawables_.empty())return;
				if(e.code.action() != core::ctrl::act::release)return;
				if(!self.contains(e.pos))return;

				switch(e.code.key()){
				case core::ctrl::mouse::LMB:
					self.current_frame_index = (self.current_frame_index + 1) % self.drawables_.size();
					break;
				case core::ctrl::mouse::RMB:
					if(self.has_tooltip_builder()){
						if(self.has_tooltip())self.tooltip_notify_drop();
						else self.build_tooltip();
					}
					break;
				default:
					break;
				}

			});
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
					img->set_drawable<drawable_ref>(drawable.drawable.get());
					img->set_button_callback(button_tags::general, [&owner, idx](elem& elem){
						owner.current_frame_index = idx;
						owner.tooltip_notify_drop();
					});

					img->checkers.setActivatedProv([&owner, idx]{
						return owner.get_frame_index() == idx;
					});
					// img->property.set_empty_drawer();

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
