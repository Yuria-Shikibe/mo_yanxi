//
// Created by Matrix on 2025/4/28.
//

export module mo_yanxi.ui.assets:icons;

export import mo_yanxi.graphic.image_region;

namespace mo_yanxi::ui::theme{
	namespace icons{
		export using icon_raw_present = graphic::combined_image_region<graphic::size_awared_uv<graphic::uniformed_rect_uv>>;

		export inline icon_raw_present up{};
		export inline icon_raw_present down{};
		export inline icon_raw_present left{};
		export inline icon_raw_present right{};
		export inline icon_raw_present file_general{};
		export inline icon_raw_present folder_general{};
		export inline icon_raw_present close{};
		export inline icon_raw_present check{};
		export inline icon_raw_present plus{};

		export inline icon_raw_present blender_icon_pivot_active{};
		export inline icon_raw_present blender_icon_pivot_individual{};
		export inline icon_raw_present blender_icon_pivot_median{};
	}
}
