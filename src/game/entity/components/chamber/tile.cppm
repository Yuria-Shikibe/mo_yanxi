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

namespace mo_yanxi::game::ecs{

	namespace chamber{
		export constexpr int tile_size_integral = 25;
		export constexpr float tile_size = tile_size_integral;

		export class tile_grid;
		export class chamber_grid;

		export using tile_coord = math::point2;
		export using tile_region = math::irect;
		export
		struct building_data;

		export
		struct building{
			friend building_data;
			friend game::ecs::component_custom_behavior<chamber::building_data>;

		private:
			building_data* data{};

		public:
			virtual ~building() = default;
		};


		struct building_data{
			tile_region region{};
			chamber_grid* grid{};

		private:
			std::unique_ptr<building> actor{};

		public:
			[[nodiscard]] building_data() = default;

			[[nodiscard]] building_data(
				const tile_region& region, chamber_grid* grid,
				std::unique_ptr<building>&& actor = {})
				: region(region),
				  grid(grid),
				  actor(std::move(actor)){
			}

			[[nodiscard]] const chamber::building& building() const{
				return *actor;
			}

			[[nodiscard]] chamber::building& building(){
				return *actor;
			}

			[[nodiscard]] bool has_building(){
				return static_cast<bool>(actor);
			}

			[[nodiscard]] constexpr math::frect get_bound() const noexcept{
				return region.as<float>().scl(tile_size, tile_size);
			}
		};
	}
}


namespace mo_yanxi::game::ecs::chamber{
	template <typename ...T>
	using building_of = std::tuple<building_data, T...>;



	export
	struct building_ref : entity_ref{
		using entity_ref::entity_ref;

		[[nodiscard]] explicit(false) building_ref(entity_ref&& entity_id)
			: entity_ref(std::move(entity_id)){
		}

		[[nodiscard]] explicit(false) building_ref(const entity_ref& entity_id)
			: entity_ref(entity_id){
		}

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
