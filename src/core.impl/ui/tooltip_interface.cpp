module mo_yanxi.ui.primitives;


void mo_yanxi::ui::tooltip_owner::tooltip_notify_drop(){
	if(tooltip_handle)tooltip_handle->get_scene()->tooltip_manager.request_drop(this);
}

mo_yanxi::ui::group* mo_yanxi::ui::tooltip_owner::tooltip_deduce_parent(scene& scene) const{
	if(auto p = tooltip_specfied_parent()){
		return p;
	}

	return scene.root;
}
