module;

#include <cassert>

module mo_yanxi.game.meta.grid;

import mo_yanxi.game.ecs.component.hitbox;
import mo_yanxi.ui.graphic;

import mo_yanxi.game.ecs.component.chamber;
import mo_yanxi.open_addr_hash_map;

mo_yanxi::game::meta::chamber::grid::grid(const hitbox& hitbox){
	auto bound = hitbox.get_bound();
	bound.trunc_vert(tile_size).scl(1 / tile_size, 1 / tile_size);
	extent_ = bound.extent().round<unsigned>();
	origin_coord_ = -bound.src.round<int>();


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
}

void mo_yanxi::game::meta::chamber::grid::draw(graphic::renderer_ui& renderer, const graphic::camera2& camera) const{
	auto acquirer = ui::get_draw_acquirer(renderer);
	using namespace graphic;
	
	acquirer.proj.set_layer(ui::draw_layers::base);
	
	auto region = get_grid_region().as<float>().scl(tile_size, tile_size);
	const auto [w, h] = get_extent();

	constexpr auto color = colors::gray.copy().set_a(.5f);

	for(unsigned x = 0; x <= w; ++x){
		static constexpr math::vec2 off{tile_size, 0};
		draw::line::line_ortho(acquirer.get(), region.vert_00() + off * x, region.vert_01() + off * x, 4, color, color);
	}

	for(unsigned y = 0; y <= h; ++y){
		static constexpr math::vec2 off{0, tile_size};
		draw::line::line_ortho(acquirer.get(), region.vert_00() + off * y, region.vert_10() + off * y, 4, color, color);
	}

	acquirer.proj.set_layer(ui::draw_layers::def);
	acquirer.proj.mode_flag = draw::mode_flags::slide_line;
	auto off = get_origin_offset();
	for(unsigned y = 0; y < h; ++y){
		for(unsigned x = 0; x < w; ++x){
			auto& info = (*this)[x, y];

			if(info.is_idle()){
				auto sz = math::irect{tags::from_extent, off.copy().add(x, y), 1, 1}.as<float>().scl(tile_size, tile_size);
				draw::fill::rect_ortho(acquirer.get(), sz, colors::aqua.copy().set_a(.1f));
			}else if (info.is_building_identity({x, y})){
				acquirer.proj.mode_flag = {};

				auto rg = info.building->get_indexed_region().as<int>().move(get_origin_offset()).as<float>().scl(tile_size, tile_size);
				draw::line::rect_ortho(acquirer, rg, 4, colors::aqua.copy());
				info.building->get_meta_info().draw(rg, renderer, camera);
				if(auto ist = info.building->get_instance_data()){
					ist->draw(info.building->get_meta_info(), rg, renderer, camera);
				}
				acquirer.proj.mode_flag = draw::mode_flags::slide_line;
			}
		}
	}
}

void mo_yanxi::game::meta::chamber::grid::dump(ecs::chamber::manifold_ref clear_grid_manifold) const{

	auto& mf = ecs::chamber::to_manifold(clear_grid_manifold);
	//TODO assert mf is empty?
	for(unsigned y = 0; y < extent_.y; ++y){
		for(unsigned x = 0; x < extent_.x; ++x){
			auto& tile = (*this)[x, y];

			if(tile.is_building_identity({x, y})){
				tile.building->get_meta_info().create_instance_chamber(clear_grid_manifold, math::vector2{x, y}.as<int>() + get_origin_offset());
			}else if(tile.is_idle()){
				empty_chamber.create_instance_chamber(clear_grid_manifold, math::vector2{x, y}.as<int>() + get_origin_offset());
			}
		}
	}

	mf.manager.do_deferred();

	mf.manager.sliced_each([this](ecs::chamber::building& building){
		const auto pos = coord_to_index(building.data().region().src).value();
		const auto build = (*this)[pos].building;

		assert(build != nullptr);

		build->get_meta_info().install(building);
		if(auto ist = build->get_instance_data()){
			ist->install(building);
		}
	});
}
