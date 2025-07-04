module mo_yanxi.ui.layout.cell;

import mo_yanxi.ui.basic;

void mo_yanxi::ui::basic_cell::apply_to_base(group& group, elem& elem, stated_extent real_cell_extent) const{
	elem.property.relative_src = get_relative_src();

	elem.update_abs_src(group.content_src_pos());

	//TODO scaling and offset depending on the align
	elem.resize((allocated_region.size() * scaling - margin.get_size()).max({}));

	if(!real_cell_extent.width.mastering() && !real_cell_extent.width.dependent()){
		real_cell_extent.width.promote(math::clamp_positive(allocated_region.width() - margin.width()));
	}

	if(!real_cell_extent.height.mastering() && !real_cell_extent.height.dependent()){
		real_cell_extent.height.promote(math::clamp_positive(allocated_region.height() - margin.height()));
	}

	if(real_cell_extent.width.mastering()){
		real_cell_extent.width.value = math::clamp_positive(allocated_region.width() - margin.width());
	}

	if(real_cell_extent.height.mastering()){
		real_cell_extent.height.value = math::clamp_positive(allocated_region.height() - margin.height());
	}

	elem.context_size_restriction = real_cell_extent;
	elem.layout();
}

void mo_yanxi::ui::mastering_cell::apply_to(
	this const mastering_cell& self,
	group& group_,
	elem& elem,
	ui::stated_extent real_cell_extent
	){
	self.basic_cell::apply_to(group_, elem, real_cell_extent);
	// if(self.stated_extent.dependent())elem.expandable = true;
}
