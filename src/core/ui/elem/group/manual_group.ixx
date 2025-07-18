//
// Created by Matrix on 2025/3/13.
//

export module mo_yanxi.ui.elem.manual_table;

export import mo_yanxi.ui.celled_group;
import std;
namespace mo_yanxi::ui{
	using passive_cell_adaptor = cell_adaptor<scaled_cell>;

	export struct manual_table : celled_group<passive_cell_adaptor>{
		[[nodiscard]] manual_table(scene* scene, group* group)
			: universal_group(scene, group){
		}

		void layout() override{
			for (adaptor_type& cell : cells){
				layout_cell(cell);
			}

			elem::layout();
		}

	private:
		void on_add(adaptor_type& adaptor) override{
			layout_cell(adaptor);
		}

		void layout_cell(adaptor_type& adaptor){
			const auto bound = content_size();
			const auto src = adaptor.cell.region_scale.get_src() * bound;

			auto size = adaptor.cell.region_scale.extent() * bound;
			size = adaptor.cell.clamp_size(size);

			auto region = math::frect{tags::from_extent, src, size};

			region.src = align::transform_offset(adaptor.cell.align, bound, region);
			adaptor.cell.allocated_region = region;

			adaptor.apply(*this, {region.width(), region.height()});
		}
	};
}
