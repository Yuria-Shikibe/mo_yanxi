//
// Created by Matrix on 2025/3/13.
//

export module mo_yanxi.ui.layout.cell;

export import mo_yanxi.ui.primitives;
export import mo_yanxi.ui.layout.policies;

export import mo_yanxi.math.rect_ortho;
export import align;
import std;

namespace mo_yanxi::ui{
	export
	template <typename Elem, typename Cell>
	struct cell_create_result{
		Elem& elem;
		Cell& cell; //Dangling Caution
	};

	export
	template <typename Elem>
	struct cell_creator_base{
		using elem_type = Elem;
	};

	export
	template <typename Create>
	concept cell_creator = requires{
		typename std::remove_cvref_t<Create>::elem_type;
	};

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
			return self.margin.top_lft() + self.allocated_region.get_src();
		}

		void apply_to(group& group, elem& elem, optional_mastering_extent real_cell_extent) const;

	};

	export
	struct scaled_cell : basic_cell{
		math::frect region_scale{};
	};

	export
	struct partial_mastering_cell : basic_cell{
		stated_size stated_size{size_category::passive, 1};
		align::padding1d<float> pad{};

		auto& set_size(const ui::stated_size stated_size){
			this->stated_size = stated_size;
			return *this;
		}

		auto& set_size(const float size){
			this->stated_size = {size_category::mastering, size};
			return *this;
		}

		auto& set_external(){
			this->stated_size = {size_category::dependent, 1};
			return *this;
		}

		auto& set_passive(float weight){
			this->stated_size = {size_category::passive, weight};
			return *this;
		}

		auto& set_from_ratio(float ratio = 1){
			this->stated_size = {size_category::scaling, ratio};
			return *this;
		}

		auto& set_pad(const align::padding1d<float> pad){
			this->pad = pad;
			return *this;
		}
	};

	export
	struct mastering_cell : basic_cell{
		stated_extent stated_extent{};
		align::spacing pad{};

		/**
		 * @brief indicate the cell to grow to the whole line space
		 */
		bool saturate{};

		constexpr auto& set_size(math::vec2 sz) noexcept {
			stated_extent.width = {size_category::mastering, sz.x};
			stated_extent.height = {size_category::mastering, sz.y};

			return *this;
		}

		constexpr auto& set_size(float sz) noexcept {
			return set_size({sz, sz});
		}

		constexpr auto& set_pad(align::spacing pad) noexcept {
			this->pad = pad;
			return *this;
		}
		constexpr auto& set_pad(float pad) noexcept {
			this->pad.set(pad);
			return *this;
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

		constexpr auto& set_external_weight(math::vec2 weight) noexcept {
			if(weight.x > 0){
				stated_extent.width.type = size_category::dependent;
				stated_extent.width.value = weight.x;
			}

			if(weight.y > 0){
				stated_extent.height.type = size_category::dependent;
				stated_extent.height.value = weight.y;
			}

			return *this;
		}

		constexpr auto& set_external(math::vector2<bool> weight) noexcept {
			if(weight.x){
				stated_extent.width.type = size_category::dependent;
				stated_extent.width.value = static_cast<float>(weight.x);
			}

			if(weight.y){
				stated_extent.height.type = size_category::dependent;
				stated_extent.height.value = static_cast<float>(weight.y);
			}

			return *this;
		}

		constexpr auto& set_external() noexcept {
			return set_external({true, true});
		}

	};
}
