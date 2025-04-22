//
// Created by Matrix on 2024/4/18.
//

// ReSharper disable CppDFAUnreachableCode
export module mo_yanxi.game.ecs.component.chamber:tile;

export import mo_yanxi.math.rect_ortho;
export import mo_yanxi.math.vector2;
export import mo_yanxi.game.ecs.quad_tree_interface;
export import mo_yanxi.game.ecs.entity;
export import mo_yanxi.game.ecs.component.damage;
export import mo_yanxi.game.ecs.component.hit_point;
export import mo_yanxi.referenced_ptr;

import std;

namespace mo_yanxi::game::ecs{

	namespace chamber{
		export constexpr int tile_size_integral = 25;
		export constexpr float tile_size = tile_size_integral;

		export struct tile;
		export class tile_grid;
		export class chamber_grid;

		export using tile_coord = math::point2;
		export using tile_region = math::irect;

		export enum struct damage_consume_result{
			success,
			damage_exhaust,
			hitpoint_exhaust,

		};
		export
		struct building_data;

		export
		struct tile_damage_event{
			using damage_type = float;
			math::ushort_point2 local_coord;
			damage_type damage;
		};

		export
		struct building{
			friend building_data;
			friend game::ecs::component_custom_behavior<chamber::building_data>;

		private:
			building_data* data{};

		public:
			virtual ~building() = default;
		};

		struct tile_status{
			float valid_hit_point{50};

			constexpr float take_damage(float damage) noexcept{
				if(damage >= valid_hit_point){
					return std::exchange(valid_hit_point, 0);
				}
				valid_hit_point -= damage;
				return damage;
			}
		};

		struct building_data{
			friend chamber_grid;

		private:
			tile_region region_{};
			chamber_grid* grid_{};

		public:
			std::vector<tile_damage_event> damage_events{};
			std::vector<tile_status> tile_states{};
			hit_point hit_point{};

		private:
			float critical_damage_take{};

		public:
			[[nodiscard]] tile_region region() const noexcept{
				return region_;
			}

			[[nodiscard]] chamber_grid* grid() const noexcept{
				return grid_;
			}

			tile_status& operator[](const tile& tile) noexcept;
			const tile_status& operator[](const tile& tile) const noexcept;


			template <std::integral T>
			[[nodiscard]] std::size_t local_to_index(math::vector2<T> coord) const noexcept{
				return coord.x + coord.y * region_.width();
			}

			void clear_hit_events() noexcept{
				damage_events.clear();
				critical_damage_take = 0;
			}

			damage_consume_result consume_damage(tile_coord coord, damage_group& total_damage){
				if(critical_damage_take > hit_point.cur)return damage_consume_result::hitpoint_exhaust;
				auto sum = total_damage.sum();

				if(sum <= 0.f){
					return damage_consume_result::damage_exhaust;
				}

				auto local = (coord - region_.src);
				sum = tile_states[local_to_index(local)].take_damage(std::min(sum, 30.f));

				if(sum > 0.){
					critical_damage_take += sum;
					damage_events.emplace_back(local.as<unsigned short>(), sum);
					total_damage.material_damage.direct -= sum;
					return damage_consume_result::success;
				}else{
					return damage_consume_result::hitpoint_exhaust;
				}
			}
		//
		// private:
		// 	std::unique_ptr<building> actor{};

		public:
			[[nodiscard]] building_data() = default;

			[[nodiscard]] building_data(
				const tile_region& region, chamber_grid* grid)
				: region_(region),
				  grid_(grid){
				if (region.area() == 0){
					throw std::invalid_argument("tile region has no area");
				}
			}

			[[nodiscard]] constexpr math::frect get_bound() const noexcept{
				return region_.as<float>().scl(tile_size, tile_size);
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

		[[nodiscard]] building_data& data() const noexcept{
			return id()->at<building_data>();
		}
	};

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
				return building.data().region().src - tile_pos;
			}
			return {};
		}

		[[nodiscard]] constexpr bool is_building_holder() const noexcept{
			if(building){
				return building.data().region().src == tile_pos;
			}else{
				return false;
			}
		}

		[[nodiscard]] tile_status& get_status() const noexcept{
			return building.data()[*this];
		}
	};


	tile_status& building_data::operator[](const tile& tile) noexcept{
		return tile_states[local_to_index(tile.tile_pos - region_.src)];
	}

	const tile_status& building_data::operator[](const tile& tile) const noexcept{
		return tile_states[local_to_index(tile.tile_pos - region_.src)];
	}
}
