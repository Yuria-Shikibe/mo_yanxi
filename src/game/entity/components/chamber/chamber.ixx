module;

#include <cassert>

export module mo_yanxi.game.ecs.component.chamber;

export import :grid_meta;
export import :chamber;
export import :grid;
export import :chamber_meta;

namespace mo_yanxi::game::ecs::chamber{

	const meta::chamber::grid_building* building_data::get_meta() const noexcept{
		assert(grid().meta_grid != nullptr);
		auto& g = *this->grid().meta_grid;
		auto index = g.coord_to_index(region_.src);

		return g.tile_at(index.value()).building;
	}

}
