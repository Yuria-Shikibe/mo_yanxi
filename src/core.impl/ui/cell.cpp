module mo_yanxi.ui.layout.cell;

import mo_yanxi.ui.primitives;

void mo_yanxi::ui::basic_cell::apply_to(group& group, elem& elem, optional_mastering_extent real_cell_extent) const{
	elem.property.relative_src = get_relative_src();

	elem.update_abs_src(group.content_src_pos());

	//TODO scaling and offset depending on the align
	elem.resize((allocated_region.extent() * scaling - margin.extent()).max({}));

	if(!real_cell_extent.width_dependent()){
		real_cell_extent.set_width(std::fdim(allocated_region.width(), margin.width()));
	}

	if(!real_cell_extent.height_dependent()){
		real_cell_extent.set_height(std::fdim(allocated_region.height(), margin.height()));
	}

	elem.context_size_restriction = real_cell_extent;
	elem.layout();
}
