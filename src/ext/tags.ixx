//
// Created by Matrix on 2025/2/24.
//

export module mo_yanxi.tags;

namespace mo_yanxi::tags{
	export struct from_rad_t{
	};

	export struct unchecked_t{
	};

	export constexpr inline from_rad_t from_rad{};
	export constexpr inline unchecked_t unchecked{};


	export struct from_extent_t{};
	export constexpr from_extent_t from_extent{};
}

