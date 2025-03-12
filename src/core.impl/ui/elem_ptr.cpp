module;

#include <cassert>

module mo_yanxi.ui.elem_ptr;

import mo_yanxi.ui.pre_decl;
import mo_yanxi.ui.elem;
import mo_yanxi.ui.group;
import mo_yanxi.ui.scene;


mo_yanxi::ui::elem_ptr::~elem_ptr(){
	delete element;
}


void mo_yanxi::ui::elem_ptr::check_group_set() const noexcept{
	assert(element != nullptr);
	assert(element->get_parent() != nullptr);
}

void mo_yanxi::ui::elem_ptr::check_scene_set() const noexcept{
	assert(element != nullptr);
	assert(element->get_scene() != nullptr);
}
