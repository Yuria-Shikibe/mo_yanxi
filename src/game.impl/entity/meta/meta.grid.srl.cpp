module;

#include <srl/game.meta.chamber.grid.pb.h>
#include "srl/srl.game.hpp"

module mo_yanxi.game.meta.grid.srl;

import mo_yanxi.game.content;


const mo_yanxi::game::meta::chamber::basic_chamber* mo_yanxi::game::meta::srl::find_chamber_meta(
	const std::string_view content_name) noexcept{
	const auto* rst = content::chambers.find(content_name);
	return rst ? static_cast<const chamber::basic_chamber*>(rst) : nullptr;
}

mo_yanxi::game::srl::chunk_serialize_handle mo_yanxi::game::meta::srl::write_grid(std::ostream& stream, const chamber::grid& grid) try{
	io::pb::game::meta::grid gridMsg{};
	io::store(gridMsg.mutable_extent(), grid.get_extent());
	io::store(gridMsg.mutable_origin_coord(), grid.get_origin_coord());

	gridMsg.mutable_tiles()->Reserve(grid.get_extent().area());
	for (const auto & grid_tile : grid.get_tiles()){
		auto& tile = *gridMsg.add_tiles();
		tile.set_placeable(grid_tile.placeable);
	}

	const auto& buildings = grid.get_buildings();
	gridMsg.mutable_buildings()->Reserve(buildings.size());
	std::vector<chunk_serialize_handle> instance_handles{};
	instance_handles.reserve(std::ranges::count_if(buildings, &chamber::grid_building::has_instance_data));

	srl_size instance_chunk_size{};
	for (const auto & building : buildings){
		auto& b = *gridMsg.add_buildings();
		b.mutable_meta()->set_content_name(building.get_meta_info().name);
		io::store(b.mutable_region(), building.get_indexed_region());

		if(auto data = building.get_instance_data()){
			auto hdl = data->write(stream);

			if(auto chunk_size = hdl.get_offset()){
				instance_handles.push_back(std::move(hdl));
				b.set_instance_data_size(chunk_size);
				instance_chunk_size += chunk_size;
			}
		}
	}
	srl_size chunk_head_size{gridMsg.ByteSizeLong()};

	co_yield sizeof(srl_size) + chunk_head_size + instance_chunk_size;

	srl::write_chunk_head(stream, chunk_head_size);

	gridMsg.SerializeToOstream(&stream);
	std::ranges::for_each(instance_handles, &chunk_serialize_handle::resume);
}catch(...){
	throw srl_hard_error{};
}

void mo_yanxi::game::meta::srl::read_grid(std::istream& stream, chamber::grid& grid){
	srl_size chunk_head_size = read_chunk_head(stream);

	std::string buffer{};
	io::pb::game::meta::grid gridMsg{};

	{
		srl::read_to_buffer(stream, chunk_head_size, buffer);
		std::ispanstream head_reader{buffer};
		if(!gridMsg.ParseFromIstream(&head_reader)){
			//
			throw srl_logical_error{};
		}
	}

	{
		auto extent = io::extract<math::u32size2>(gridMsg.extent());
		auto origin_coord = io::extract<math::i32point2>(gridMsg.origin_coord());

		std::vector<chamber::grid_tile> tiles{extent.area()};
		for (auto&& [idx, tile] : tiles | std::views::enumerate){
			tile.placeable = gridMsg.tiles(idx).placeable();
		}

		grid = {extent, origin_coord, std::move(tiles)};
	}

	for (const auto & building : gridMsg.buildings()){
		const auto region = io::extract<math::urect>(building.region());
		const auto chunk_size = [&] -> srl_size {
			if(building.has_instance_data_size()){
				return building.instance_data_size();
			}
			return 0;
		}();

		stream_skipper _{stream, chunk_size};

		auto* meta = find_chamber_meta(building.meta().content_name());

		if(!meta || meta->extent.x > region.width() || meta->extent.y > region.height()){
			//size changed, skip to avoid overlap
			continue;
		}

		auto* build = grid.try_place_building_at(region.src, *meta);
		if(!build){
			continue;
		}

		if(const auto instance = build->get_instance_data(); chunk_size && instance){
			read_to_buffer(stream, chunk_size, buffer);
			std::ispanstream instance_data_reader{buffer};

			try{
				instance->read(instance_data_reader);
			}catch(...){
				//discard failed result, using default instead
				build->reset_instance_data();
			}

		}
	}
}
