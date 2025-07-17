module;

#include <cassert>

module mo_yanxi.ui.primitives;

import :pre_decl;
import :elem;
import :group;

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
