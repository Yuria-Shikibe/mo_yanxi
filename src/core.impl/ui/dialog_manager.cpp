module mo_yanxi.ui.primitives;

import :scene;

import mo_yanxi.basic_util;
import mo_yanxi.graphic.renderer.ui;

void mo_yanxi::ui::dialog_manager::draw_all(rect clipspace) const{

	auto& batch = graphic::renderer_from_erased(scene_->renderer)->batch;

	for (auto dialog : draw_sequence){
		dialog->try_draw(clipspace);
		batch->consume_all();
		batch.blit_viewport(dialog->get_bound());
	}

}

void mo_yanxi::ui::dialog_manager::update(float delta_in_tick){
	modifiable_erase_if(fading_dialogs, [&, this](dialog_fading& dialog){
		dialog.duration -= delta_in_tick;
		if(dialog.duration <= 0){
			std::erase(draw_sequence, dialog.get());

			return true;
		}else{
			dialog.elem->update_opacity(dialog.duration / dialog_fading::fading_time);
			return false;
		}
	});

	for (auto& value : dialogs){
		value.update_bound(scene_->get_extent());
		value.get()->update(delta_in_tick);
		value.get()->try_layout();
	}
}

void mo_yanxi::ui::dialog_manager::clear_tooltip() const{

	scene_->tooltip_manager.clear();
}

void mo_yanxi::ui::dialog_manager::update_top() noexcept{
	if(dialogs.empty()){
		top_ = nullptr;
	}else{
		top_ = dialogs.back().elem.get();
	}

	scene_->swap_event_focus_to_null();
	scene_->on_cursor_pos_update(true);
}

mo_yanxi::ui::esc_flag mo_yanxi::ui::dialog_manager::on_esc() noexcept{
	for (auto&& elem : dialogs | std::views::reverse){
		if(util::thoroughly_esc(elem.elem.get()) != esc_flag::fall_through){
			return esc_flag::intercept;
		}
	}

	if(!dialogs.empty()){
		truncate(std::prev(dialogs.end()));
		return esc_flag::intercept;
	}

	return esc_flag::fall_through;
}
