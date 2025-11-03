module;

#include <cassert>

export module mo_yanxi.gui.elem.grid;

import mo_yanxi.gui.celled_group;
import std;


namespace mo_yanxi::gui{

enum struct grid_capture_type : std::uint8_t{
	src_extent,
	src_dst,
	margin,
	extent_dst, //Reversed src_extent
};

enum struct fallback_strategy : std::uint8_t{
	hide,
	shrink_or_hide,
	use_nearby,

};

struct grid_capture_size{
	grid_capture_type type{};
	fallback_strategy fall{};
	std::uint16_t p1{};
	std::uint16_t p2{};
};

struct grid_cell : layout::basic_cell{
	math::vector2<grid_capture_size> extent{};
};

enum struct grid_spec_type{
	uniformed_mastering,
	uniformed_passive,
	uniformed_scaling,
	all_mastering,
	all_passive,
	all_scaling,
	mixed,
};

template <layout::size_category retTag>
struct uniformed_extent{
	std::uint32_t extent{};
	float value{};

	[[nodiscard]] std::uint32_t size() const noexcept{
		return extent;
	}

	layout::stated_size operator[](unsigned idx) const noexcept{
		assert(idx < extent);
		return layout::stated_size{retTag, value};
	}
};

template <typename T>
using container_t = std::vector<T>;

template <layout::size_category retTag>
struct variable_extent{
	container_t<float> value{};

	[[nodiscard]] std::uint32_t size() const noexcept{
		return value.size();
	}

	layout::stated_size operator[](unsigned idx) const noexcept{
		return layout::stated_size{retTag, value[idx]};
	}
};

using grid_uniformed_mastering = uniformed_extent<layout::size_category::mastering>;
using grid_uniformed_passive = uniformed_extent<layout::size_category::passive>;
using grid_uniformed_scaling = uniformed_extent<layout::size_category::scaling>;

using grid_all_mastering = variable_extent<layout::size_category::mastering>;
using grid_all_passive = variable_extent<layout::size_category::passive>;
using grid_all_scaling = variable_extent<layout::size_category::scaling>;

struct grid_mixed{
	container_t<layout::stated_size> value{};

	[[nodiscard]] std::uint32_t size() const noexcept{
		return value.size();
	}

	layout::stated_size operator[](unsigned idx) const noexcept{
		return value[idx];
	}
};

struct grid_dim_spec{
	using variant_t = std::variant<
		grid_uniformed_mastering,
		grid_uniformed_passive,
		grid_uniformed_scaling,
		grid_all_mastering,
		grid_all_passive,
		grid_all_scaling,
		grid_mixed
		>;

	variant_t spec{};

	[[nodiscard]] std::uint32_t size() const noexcept{
		return std::visit([](const auto& v){return v.size(); }, spec);
	}

	layout::stated_size operator[](unsigned idx) const noexcept{
		return std::visit([idx](const auto& v){return v[idx]; }, spec);
	}

	bool operator==(const grid_dim_spec&) const noexcept = default;
};

struct grid : universal_group<grid_cell>{
private:
	math::vector2<grid_dim_spec> extent_spec_{};

public:
	[[nodiscard]] grid(scene& scene, elem* parent, math::vector2<grid_dim_spec>&& extent_spec)
		: universal_group(scene, parent), extent_spec_(std::move(extent_spec)){
	}


};
}
