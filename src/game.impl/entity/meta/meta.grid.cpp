module;

#include <cassert>

module mo_yanxi.game.ecs.component.chamber;

import mo_yanxi.game.ecs.component.hitbox;
import mo_yanxi.ui.graphic;

import mo_yanxi.game.ecs.component.chamber;
import mo_yanxi.open_addr_hash_map;

mo_yanxi::game::meta::chamber::grid::grid(const hitbox& hitbox){
	auto bound = hitbox.get_bound();
	bound.trunc_vert(tile_size).scl(1 / tile_size, 1 / tile_size);
	extent_ = bound.extent().round<unsigned>();
	origin_coord_ = -bound.src.round<int>();
	structural_status_ = structural_status{extent_.as<int>()};

	tiles_.resize(extent_.area());

	auto bx = ccd_hitbox{hitbox};

	auto hitboxContains = [&](math::frect region){
		if(!region.overlap_exclusive(bx.min_wrap_bound()))return false;
		for (auto && comp : bx.get_comps()){
			if(comp.box.contains(region))return true;
		}
		return false;
	};

	for(unsigned x = 0; x < extent_.x; ++x){
		for(unsigned y = 0; y < extent_.y; ++y){
			math::vector2 pos{x, y};
			auto& info = tile_at(pos);

			if(hitboxContains(math::irect{tags::from_extent, pos.as<int>() - origin_coord_, 1, 1}.as<float>().scl(tile_size, tile_size))){
				info.placeable = true;
			}
		}
	}

	total_mass_ = get_grid_mass(*this);
	total_moment_of_inertia_ = get_grid_tile_moment_of_inertia(*this);
}

mo_yanxi::game::meta::chamber::grid::grid(const math::usize2 extent, const math::point2 origin_coord,
	std::vector<grid_tile>&& tiles) noexcept:
	origin_coord_(origin_coord), extent_(extent), tiles_(std::move(tiles)), structural_status_(extent.as<int>()){

	total_mass_ = get_grid_mass(*this);
	total_moment_of_inertia_ = get_grid_tile_moment_of_inertia(*this);
}

void mo_yanxi::game::meta::chamber::grid::move(math::point2 offset) noexcept{
	origin_coord_ -= offset.as<int>();
}

void mo_yanxi::game::meta::chamber::grid::dump(ecs::chamber::chamber_manifold& clear_grid_manifold) const{

	//TODO assert mf is empty?
	for(unsigned y = 0; y < extent_.y; ++y){
		for(unsigned x = 0; x < extent_.x; ++x){
			auto& tile = (*this)[x, y];

			if(tile.is_building_identity({x, y})){
				tile.building->get_meta_info().create_instance_chamber(clear_grid_manifold, math::vector2{x, y}.as<int>() + get_origin_offset());
			}else if(tile.is_idle()){
				// empty_chamber.create_instance_chamber(clear_grid_manifold, math::vector2{x, y}.as<int>() + get_origin_offset());
			}
		}
	}

	clear_grid_manifold.manager.do_deferred();

	clear_grid_manifold.manager.sliced_each([this](ecs::chamber::building& building){
		const auto pos = coord_to_index(building.data().region().src).value();
		const auto build = (*this)[pos].building;

		assert(build != nullptr);

		build->get_meta_info().install(building);
		if(auto ist = build->get_instance_data()){
			ist->install(building);
		}
	});
}
