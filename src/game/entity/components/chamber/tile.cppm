//
// Created by Matrix on 2024/4/18.
//

// ReSharper disable CppDFAUnreachableCode
export module mo_yanxi.game.ecs.component.chamber:tile;

export import mo_yanxi.math.rect_ortho;
export import mo_yanxi.math.vector2;
export import mo_yanxi.game.ecs.quad_tree_interface;
export import mo_yanxi.game.ecs.entity;
export import mo_yanxi.referenced_ptr;

import std;

namespace mo_yanxi::game::ecs::chamber{
	export constexpr int tile_size_integral = 25;
	export constexpr float tile_size = tile_size_integral;

	export class tile_grid;
	export class grid_instance;

	export using tile_coord = math::point2;
	export using tile_region = math::irect;


	export
	struct building_data{
		tile_region region{};
		grid_instance* grid{};

		[[nodiscard]] constexpr math::frect get_bound() const noexcept{
			return region.as<float>().scl(tile_size, tile_size);
		}
	};

	template <typename ...T>
	using building_of = std::tuple<building_data, T...>;



	export
	struct building_ref : entity_ref{
		using entity_ref::entity_ref;

		[[nodiscard]] building_data& get_region() const noexcept{
			return id()->at<building_data>();
		}
	};

	export
	struct tile{
		tile_coord tile_pos{};
		building_ref building{};
		// bool placeable{};

		constexpr explicit operator bool() const noexcept{
			return static_cast<bool>(building);
		}

		void set_pos(tile_coord p) noexcept{
			this->tile_pos = p;
		}

		[[nodiscard]] constexpr math::frect get_bound() const noexcept{
			return math::frect{tags::unchecked, tile_pos.x * tile_size, tile_pos.y * tile_size, tile_size, tile_size};
		}


		[[nodiscard]] constexpr math::vec2 get_real_pos() const noexcept{
			return (tile_pos.as<float>() * tile_size).add(tile_size / 2, tile_size / 2);
		}


		[[nodiscard]] constexpr tile_coord get_local_tile_pos() const noexcept{
			if(building){
				return building.get_region().region.src - tile_pos;
			}
			return {};
		}

		[[nodiscard]] constexpr bool is_building_holder() const noexcept{
			if(building){
				return building.get_region().region.src == tile_pos;
			}else{
				return false;
			}
		}
	};
}
