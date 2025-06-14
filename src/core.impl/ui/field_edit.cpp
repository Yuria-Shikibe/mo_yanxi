module mo_yanxi.ui.creation.field_edit;

import mo_yanxi.ui.graphic;
import mo_yanxi.ui.assets;
import mo_yanxi.graphic.draw.multi_region;

void mo_yanxi::ui::field_editor::draw_content(const rect clipSpace) const{
	auto draw_shadow = [&]{
		auto acq = ui::get_draw_acquirer(get_renderer());
		using namespace graphic;
		acq.proj.mode_flag = draw::mode_flags::sdf;
		draw::nine_patch(acq, theme::shapes::base, slider_->get_bound(), colors::black.copy().set_a(.85f));
	};

	if(slider_->visible && slider_->is_sliding()){
		input_area->draw(clipSpace);
		// unit_label->draw(clipSpace);
		draw_shadow();
		slider_->draw(clipSpace);
	}else{
		if(slider_->visible){
			slider_->draw(clipSpace);
			draw_shadow();
		}


		input_area->draw(clipSpace);
		// unit_label->draw(clipSpace);
	}

}
