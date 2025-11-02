module;

#include "gch/small_vector.hpp"

module mo_yanxi.gui.elem.sequence;

namespace mo_yanxi::gui{

[[nodiscard]] sequence_pre_layout_result get_list_layout_minor_mastering_length(
	const seq_list& list,
	float layout_major_size,
	gch::small_vector<float, 16, std::pmr::polymorphic_allocator<float>>* cache = nullptr
	) {
	auto [majorTarget, minorTarget] = layout::get_vec_ptr(list.get_layout_policy());

	float masterings_capture{};
	float passive_total{};

	const auto cells = list.cells();

	const float minorScaling = list.get_scaling().*minorTarget;

	for (auto && cell : cells){
		masterings_capture += cell.cell.pad.length();
		switch(cell.cell.stated_size.type){
		case layout::size_category::dependent:{
			math::vec2 vec;
			vec.*majorTarget = layout_major_size;
			vec.*minorTarget = std::numeric_limits<float>::infinity();

			auto rst = cell.element->pre_acquire_size(vec).value_or({}).*minorTarget;
			if(cache)cache->push_back(rst);
			masterings_capture += rst;
			break;
		}
		case layout::size_category::mastering:{
			float value = cell.cell.stated_size.value * minorScaling;
			if(cache)cache->push_back(value);
			masterings_capture += value;
			break;
		}
		case layout::size_category::passive:{
			if(cache)cache->push_back(cell.cell.stated_size.value);
			passive_total += cell.cell.stated_size.value;
			break;
		}
		case layout::size_category::scaling:{
			float value = cell.cell.stated_size.value * layout_major_size * minorScaling;

			if(cache)cache->push_back(value);
			masterings_capture += value;
			break;
		}
		}
	}

	if(!cells.empty()){
		masterings_capture -= cells.front().cell.pad.pre;
		masterings_capture -= cells.back().cell.pad.post;
	}

	return {masterings_capture, passive_total};
}


std::optional<math::vec2> seq_list::pre_acquire_size_impl(layout::optional_mastering_extent extent){
	switch(policy_){
	case layout::layout_policy::hori_major :{
		if(extent.width_dependent()) return std::nullopt;
		break;
	}
	case layout::layout_policy::vert_major :{
		if(extent.height_dependent()) return std::nullopt;
		break;
	}
	case layout::layout_policy::none :{
		if(extent.fully_mastering()) return extent.potential_extent();
		return std::nullopt;
	}
	default: std::unreachable();
	}

	const auto lines = children_.size();
	if(lines == 0) return std::nullopt;

	auto potential = extent.potential_extent();
	const auto dep = extent.get_dependent();


	auto [majorTargetDep, minorTargetDep] = layout::get_vec_ptr<bool>(policy_);

	if(dep.*minorTargetDep){
		auto [majorTarget, minorTarget] = layout::get_vec_ptr(policy_);

		if(expand_policy_ == layout::expand_policy::immutable){
			potential.*minorTarget = this->content_extent().*minorTarget;
		}else{
			auto [minor_length, _] = get_list_layout_minor_mastering_length(*this, potential.*majorTarget);
			potential.*minorTarget = minor_length;
		}

	}

	return potential;
}

void seq_list::layout(){
	if(cells_.empty()) return;
	auto [majorTarget, minorTarget] = layout::get_vec_ptr(policy_);

	gch::small_vector<float, 16, std::pmr::polymorphic_allocator<float>> sizes(get_allocator<float>());
	sizes.reserve(cells_.size());

	auto content_sz = content_extent();
	auto [masterings, passives] = get_list_layout_minor_mastering_length(*this, content_sz.*majorTarget, &sizes);

	if(expand_policy_ != layout::expand_policy::immutable){
		math::vec2 size;
		size.*majorTarget = content_sz.*majorTarget;
		size.*minorTarget = masterings;
		size += boarder().extent();

		if(expand_policy_ == layout::expand_policy::prefer){
			size.max(get_prefer_extent());
		}

		size.min(restriction_extent.potential_extent());
		resize(size, propagate_mask::force_upper);

		content_sz = content_extent();
	}

	float minor_offset = (align_to_tail_ && passives == 0) ? content_sz.*minorTarget - masterings : 0;

	const auto remains = std::fdim(content_sz.*minorTarget, masterings);
	const auto passive_unit = remains / passives;

	math::vec2 currentOff{};
	currentOff.*minorTarget = minor_offset - cells_.front().cell.pad.pre;

	for (auto&& [idx, cell] : cells_ | std::views::enumerate){
		currentOff.*minorTarget += cell.cell.pad.pre;
		auto minor = sizes[idx];
		if(cell.cell.stated_size.type == layout::size_category::passive)minor *= passive_unit;
		math::vec2 cell_sz;
		cell_sz.*majorTarget = content_sz.*majorTarget;
		cell_sz.*minorTarget = minor;

		cell.cell.allocated_region = {tags::from_extent, currentOff, cell_sz};

		cell_sz.*majorTarget = this->restriction_extent.potential_extent().*majorTarget;
		if(cell.cell.stated_size.dependent())cell_sz.*minorTarget = std::numeric_limits<float>::infinity();

		cell.apply(*this, cell_sz);
		currentOff.*minorTarget += cell.cell.pad.post + minor;
	}
}
}
