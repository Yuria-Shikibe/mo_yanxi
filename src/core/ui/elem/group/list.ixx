//
// Created by Matrix on 2025/7/17.
//

export module mo_yanxi.ui.elem.list;

export import mo_yanxi.ui.primitives;
export import mo_yanxi.ui.celled_group;
import std;

namespace mo_yanxi::ui{

	struct list_pre_layout_result{
		float masterings;
		float passive;
	};

	export
	struct list : celled_group<cell_adaptor<partial_mastering_cell>>{
	private:
		layout_policy policy_{layout_policy::hori_major};

	public:
		[[nodiscard]] list(scene* scene, group* parent)
			: universal_group(scene, parent){
		}

	protected:
		std::optional<math::vec2> pre_acquire_size_impl(optional_mastering_extent extent) override{
			switch(policy_){
			case layout_policy::hori_major : if(extent.width_dependent()) return std::nullopt;
				break;
			case layout_policy::vert_major : if(extent.height_dependent()) return std::nullopt;
				break;
			case layout_policy::none : if(!extent.fully_mastering()) return std::nullopt;
				else return extent.potential_extent();
			default: std::unreachable();
			}

			const auto lines = children.size();
			if(lines == 0) return std::nullopt;

			auto potential = extent.potential_extent();
			auto dep = extent.get_dependent();


			auto [majorTargetDep, minorTargetDep] = get_vec_ptr<bool>(policy_);

			if(dep.*minorTargetDep){
				auto [majorTarget, minorTarget] = get_vec_ptr(policy_);
				auto [minor_length, _] = get_list_layout_minor_mastering_length(potential.*majorTarget);

				auto bsize = get_pad_extent(policy_, prop().boarder);
				potential.*minorTarget = minor_length + bsize.minor;
			}

			return potential;
		}

		[[nodiscard]] list_pre_layout_result get_list_layout_minor_mastering_length(float layout_major_size, std::vector<float>* cache = nullptr) const{
			auto [majorTarget, minorTarget] = get_vec_ptr(policy_);

			float masterings_capture{};
			float passive_total{};

			for (auto && cell : cells){
				masterings_capture += cell.cell.pad.length();
				switch(cell.cell.stated_size.type){
				case size_category::dependent:{
					math::vec2 vec;
					vec.*majorTarget = layout_major_size;
					vec.*minorTarget = std::numeric_limits<float>::infinity();

					auto rst = cell.element->pre_acquire_size(vec).value_or({}).*minorTarget;
					if(cache)cache->push_back(rst);
					masterings_capture += rst;
					break;
				}
				case size_category::mastering:{
					if(cache)cache->push_back(cell.cell.stated_size.value);
					masterings_capture += cell.cell.stated_size.value;
					break;
				}
				case size_category::passive:{
					if(cache)cache->push_back(cell.cell.stated_size.value);
					passive_total += cell.cell.stated_size.value;
					break;
				}
				case size_category::scaling:{
					if(cache)cache->push_back(cell.cell.stated_size.value * layout_major_size);
					masterings_capture += cell.cell.stated_size.value * layout_major_size;
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


	public:
		void set_layout_policy(layout_policy policy) noexcept{
			if(util::try_modify(policy_, policy)){
				notify_layout_changed(spread_direction::all_visible);
			}
		}

		void layout() override{
			if(cells.empty()) return;
			auto [majorTarget, minorTarget] = get_vec_ptr(policy_);
			std::vector<float> sizes{};
			sizes.reserve(cells.size());

			const auto content_sz = content_size();
			auto [masterings, passives] = get_list_layout_minor_mastering_length(content_sz.*majorTarget, &sizes);

			{
				auto bsize = get_pad_extent(policy_, prop().boarder);
				math::vec2 size;
				size.*majorTarget = content_sz.*majorTarget + bsize.major;
				size.*minorTarget = math::max(masterings, content_sz.*minorTarget) + bsize.minor;

				size.min(context_size_restriction.potential_extent());
				elem::resize_masked(size);
			}

			const auto remains = std::fdim(content_sz.*minorTarget, masterings);
			const auto passive_unit = remains / passives;

			math::vec2 currentOff{};
			currentOff.*minorTarget = -cells.front().cell.pad.pre;
			for (auto&& [idx, cell] : cells | std::views::enumerate){
				currentOff.*minorTarget += cell.cell.pad.pre;
				auto minor = sizes[idx];
				if(cell.cell.stated_size.type == size_category::passive)minor *= passive_unit;
				math::vec2 cell_sz;
				cell_sz.*majorTarget = content_sz.*majorTarget;
				cell_sz.*minorTarget = minor;

				cell.cell.allocated_region = {tags::from_extent, currentOff, cell_sz};

				cell_sz.*majorTarget = this->context_size_restriction.potential_extent().*majorTarget;
				if(cell.cell.stated_size.dependent())cell_sz.*minorTarget = std::numeric_limits<float>::infinity();
				cell.apply(*this, cell_sz);
				currentOff.*minorTarget += cell.cell.pad.post + minor;
			}
		}
	};
}