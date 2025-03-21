//
// Created by Matrix on 2025/3/13.
//

export module mo_yanxi.ui.layout.cell;

export import mo_yanxi.ui.layout.policies;

export import mo_yanxi.math.rect_ortho;
export import align;
import std;

namespace mo_yanxi::ui{
	export
	struct basic_cell{
		align::pos align{align::pos::center};

		math::vec2 minimum_size{};
		math::vec2 maximum_size{math::vectors::constant2<float>::inf_positive_vec2};

		math::frect allocated_region{};
		float scaling{1.f};

		align::spacing margin{};

		[[nodiscard]] constexpr math::vec2 clamp_size(math::vec2 sz) const noexcept{
			return sz.clamp_xy(minimum_size, maximum_size);
		}

		template <std::derived_from<basic_cell> T>
		constexpr math::vec2 get_relative_src(this const T& self) noexcept{
			return self.margin.top_rit() + self.allocated_region.get_src();
		}

		void apply_to_base(struct group& group, struct elem& elem, stated_extent real_cell_extent) const;

		template <std::derived_from<basic_cell> T>
		void apply_to(
			this const T& self,
			struct group& group,
			struct elem& elem,
			stated_extent real_cell_extent
			){
			self.apply_to_base(group, elem, real_cell_extent);
		}
	};

	export
	struct scaled_cell : basic_cell{
		math::frect region_scale{};
	};

	export
	struct mastering_cell : basic_cell{
		stated_extent stated_extent{};
		align::spacing pad{};

		bool saturate{};

		constexpr auto& set_size(math::vec2 sz) noexcept {
			stated_extent.width = {size_category::mastering, sz.x};
			stated_extent.height = {size_category::mastering, sz.y};

			return *this;
		}

		constexpr auto& set_size(float sz) noexcept {
			return set_size({sz, sz});
		}

		constexpr auto& set_width(float sz) noexcept {
			stated_extent.width = {size_category::mastering, sz};

			return *this;
		}

		constexpr auto& set_height(float sz) noexcept {
			stated_extent.height = {size_category::mastering, sz};

			return *this;
		}

		constexpr auto& set_height_from_scale(float scale = 1.){
			if(stated_extent.width.type == size_category::scaling){
				throw illegal_layout{};
			}
			stated_extent.height.type = size_category::scaling;
			stated_extent.height.value = scale;
			return *this;
		}

		constexpr auto& set_width_from_scale(float scale = 1.){
			if(stated_extent.height.type == size_category::scaling){
				throw illegal_layout{};
			}
			stated_extent.width.type = size_category::scaling;
			stated_extent.width.value = scale;
			return *this;
		}

		constexpr auto& set_height_passive(float weight = 1.){
			stated_extent.height.type = size_category::passive;
			stated_extent.height.value = weight;
			return *this;
		}

		constexpr auto& set_width_passive(float weight = 1.){
			stated_extent.width.type = size_category::passive;
			stated_extent.width.value = weight;
			return *this;
		}

		constexpr auto& set_external(math::vec2 weight) noexcept {
			if(weight.x > 0){
				stated_extent.width.type = size_category::external;
				stated_extent.width.value = weight.x;
			}

			if(weight.y > 0){
				stated_extent.height.type = size_category::external;
				stated_extent.height.value = weight.y;
			}

			return *this;
		}
		constexpr auto& set_external() noexcept {
			return set_external({true, true});
		}

		constexpr auto& set_external_weight(math::vector2<bool> weight) noexcept {
			if(weight.x){
				stated_extent.width.type = size_category::external;
				stated_extent.width.value = static_cast<float>(weight.x);
			}

			if(weight.y){
				stated_extent.height.type = size_category::external;
				stated_extent.height.value = static_cast<float>(weight.y);
			}

			return *this;
		}

		void apply_to(
			this const mastering_cell& self,
			struct group& group,
			struct elem& elem,
			ui::stated_extent real_cell_extent);
	};
}
