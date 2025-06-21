module;

#include <cassert>
#include <plf_hive.h>

export module mo_yanxi.game.meta.grid;

export import mo_yanxi.game.meta.chamber;
import mo_yanxi.game.meta.hitbox;
import mo_yanxi.algo;

namespace mo_yanxi::game::meta::chamber{
	constexpr int tile_size_integral = 64;
	constexpr float tile_size = tile_size_integral;


	using chamber_meta = const basic_chamber*;

	export
	struct grid_building{
	private:
		math::upoint2 identity_pos{};
		chamber_meta meta_info;
		std::unique_ptr<chamber_instance_data> instance_data{};

	public:
		[[nodiscard]] grid_building(const math::upoint2& identity_pos, const basic_chamber& chamber)
			: identity_pos(identity_pos),
			  meta_info(std::addressof(chamber)), instance_data(chamber.create_instance_data()){
		}

		void set_chamber(const basic_chamber& chamber){
			meta_info = std::addressof(chamber);
			instance_data = chamber.create_instance_data();
		}

		[[nodiscard]] math::urect get_indexed_region() const noexcept{
			return {tags::from_extent, identity_pos, meta_info->extent};
		}

		[[nodiscard]] const basic_chamber& get_meta_info() const noexcept{
			assert(meta_info != nullptr);
			return *meta_info;
		}

		[[nodiscard]] chamber_instance_data* get_instance_data() const noexcept{
			return instance_data.get();
		}

		void reset_instance_data(){
			instance_data = meta_info->create_instance_data();
		}

		[[nodiscard]] math::upoint2 get_identity_pos() const noexcept{
			return identity_pos;
		}

		[[nodiscard]] bool has_instance_data() const noexcept{
			return instance_data != nullptr;
		}
	};


	export
	struct grid_tile{
		bool placeable;
		grid_building* building;

		[[nodiscard]] bool is_building_identity(const math::upoint2 tile_pos) const noexcept{
			return building && building->get_identity_pos() == tile_pos;
		}

		[[nodiscard]] bool is_idle() const noexcept{
			return placeable && !building;
		}
	};

	export
	struct grid{
		using index_coord = math::upoint2;

	private:

		/**
		 * @brief Coord of the tile to be the real original tile after instancing
		 */
		math::point2 origin_coord{};
		math::usize2 extent{};
		std::vector<grid_tile> tiles{};
		plf::hive<grid_building> buildings{};

		std::vector<grid_building*> energy_consumer{};
		std::vector<grid_building*> energy_generator{};

		unsigned maximum_energy_generate_{};
		unsigned maximum_energy_consume_{};

	private:
		void update_energy_status_on_erase(grid_building& building){
			if(auto e_usage = building.get_meta_info().get_energy_usage()){
				if(e_usage > 0){
					algo::erase_unique_unstable(energy_generator, &building);
					maximum_energy_generate_ -= e_usage;
				}else{
					algo::erase_unique_unstable(energy_consumer, &building);
					maximum_energy_consume_ -= static_cast<decltype(maximum_energy_consume_)>(-e_usage);
				}
			}
		}

		void update_energy_status_on_add(grid_building& building) noexcept {
			if(auto e_usage = building.get_meta_info().get_energy_usage()){
				if(e_usage > 0){
					energy_generator.push_back(&building);
					maximum_energy_generate_ += e_usage;
				}else{
					energy_consumer.push_back(&building);
					maximum_energy_consume_ += static_cast<decltype(maximum_energy_consume_)>(-e_usage);
				}
			}
		}

	public:
		[[nodiscard]] grid() noexcept = default;

		[[nodiscard]] explicit grid(const hitbox& hitbox);

		[[nodiscard]] grid(math::usize2 extent, math::point2 origin_coord, std::span<const grid_tile> tiles) :
			origin_coord(origin_coord), extent(extent), tiles(std::from_range, tiles){}

		[[nodiscard]] grid(math::usize2 extent, math::point2 origin_coord, std::vector<grid_tile>&& tiles) noexcept :
			origin_coord(origin_coord), extent(extent), tiles(std::move(tiles)){}

		[[nodiscard]] math::vector2<int> get_origin_coord() const noexcept{
			return origin_coord;
		}

		[[nodiscard]] math::vector2<int> get_origin_offset() const noexcept{
			return -origin_coord;
		}

		[[nodiscard]] const decltype(buildings)& get_buildings() const noexcept{
			return buildings;
		}

		[[nodiscard]] auto get_maximum_energy_generate() const noexcept{
			return maximum_energy_generate_;
		}

		[[nodiscard]] auto get_maximum_energy_consume() const noexcept{
			return maximum_energy_consume_;
		}

		[[nodiscard]] std::span<const grid_building* const> get_generators() const noexcept{
			return energy_generator;
		}

		[[nodiscard]] std::span<const grid_building* const> get_energy_consumers() const noexcept{
			return energy_consumer;
		}

		[[nodiscard]] std::span<const grid_tile> get_tiles() const noexcept{
			return tiles;
		}

		[[nodiscard]] math::usize2 get_extent() const noexcept{
			return extent;
		}

		static math::point2 pos_to_tile_coord(math::vec2 coord) noexcept{
			return coord.div(tile_size).floor().as<int>();
		}

		[[nodiscard]] math::irect get_grid_region() const noexcept{
			return math::irect{tags::from_extent, -origin_coord, extent.as<int>()};
		}

		[[nodiscard]] std::optional<index_coord> coord_to_index(math::point2 world_tile_coord) const noexcept{
			world_tile_coord += origin_coord;
			if(!math::irect{extent.as<int>()}.contains(world_tile_coord))return {};
			return world_tile_coord.as<unsigned>();
		}

		[[nodiscard]] std::optional<math::urect> coord_to_index(math::irect world_tile_region) const noexcept{
			world_tile_region.src += origin_coord;
			if(!math::irect{extent.as<int>()}.contains_loose(world_tile_region))return {};
			return world_tile_region.as<unsigned>();
		}

		grid_building* try_place_building_at(index_coord indexed_pos, const basic_chamber& info){
			if(!is_building_placeable_at(indexed_pos, info))return nullptr;

			const auto ext = info.extent;
			const math::urect region{tags::from_extent, indexed_pos.as<unsigned>(), ext};

			auto& b = buildings.insert(grid_building{indexed_pos.as<unsigned>(), info}).operator*();

			update_energy_status_on_add(b);

			region.each([&](math::upoint2 p){
				tile_at(p).building = std::addressof(b);
			});

			return &b;
		}

		bool erase_building_at(math::upoint2 indexed_pos) noexcept {
			if(indexed_pos.beyond_equal(extent))return false;

			auto& tile = tile_at(indexed_pos);
			auto* b = tile.building;
			if(!b)return false;

			update_energy_status_on_erase(*b);

			tile.building->get_indexed_region().each([this](math::upoint2 p){
				tile_at(p).building = nullptr;
			});

			const auto itr = buildings.get_iterator(b);
			assert(itr != buildings.end());
			buildings.erase(itr);
			return true;
		}

		bool is_building_placeable_at(index_coord indexed_pos, const basic_chamber& info) const noexcept{
			auto ext = info.extent;
			math::urect region{tags::from_extent, indexed_pos, ext};
			if(!math::urect{extent}.contains_loose(region))return false;

			for(auto y = region.src.y; y < region.get_end_y(); ++y){
				for(auto x = region.src.x; x < region.get_end_x(); ++x){
					auto& tile = tile_at({x, y});
					if(!tile.placeable || tile.building != nullptr)return false;
				}
			}

			return true;
		}

		[[nodiscard]] bool is_placeable_at(math::urect region_in_index) const noexcept{
			for(unsigned y = 0; y < region_in_index.height(); ++y){
				for(unsigned x = 0; x < region_in_index.width(); ++x){
					if(!tile_at(region_in_index.src + math::vector2{x, y}).placeable)return false;
				}
			}

			return true;
		}

		[[nodiscard]] grid_tile& tile_at(math::upoint2 index_pos) noexcept{
			assert(index_pos.within(extent));
			return tiles[extent.x * index_pos.y + index_pos.x];
		}

		[[nodiscard]] const grid_tile& tile_at(math::upoint2 index_pos) const noexcept{
			assert(index_pos.within(extent));
			return tiles[extent.x * index_pos.y + index_pos.x];
		}

		template <typename S>
		decltype(auto) operator[](this S&& self, math::upoint2 index_pos) noexcept{
			return self.tile_at(index_pos);
		}
		template <typename S>
		decltype(auto) operator[](this S&& self, unsigned x, unsigned y) noexcept{
			return self.tile_at({x, y});
		}

		//TODO move draw to other place?

		void draw(graphic::renderer_ui& renderer, const graphic::camera2& camera) const;
	};
}