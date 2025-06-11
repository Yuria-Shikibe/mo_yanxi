//
// Created by Matrix on 2024/4/18.
//

// ReSharper disable CppDFAUnreachableCode
module;

#include <cassert>

export module mo_yanxi.game.ecs.component.chamber:tile;

export import mo_yanxi.math.rect_ortho;
export import mo_yanxi.math.vector2;
export import mo_yanxi.game.ecs.quad_tree_interface;
export import mo_yanxi.game.ecs.component.manage;
export import mo_yanxi.game.ecs.component.damage;
export import mo_yanxi.game.ecs.component.hit_point;

import mo_yanxi.referenced_ptr;
import mo_yanxi.game.ecs.component.ui.builder;

export import mo_yanxi.game.ecs.world.top;

import std;

namespace mo_yanxi::game::ecs{

	namespace chamber{
		export constexpr int tile_size_integral = 64;
		export constexpr float tile_size = tile_size_integral;

		export struct tile;
		export class tile_grid;
		export class chamber_grid;
		export struct chamber_manifold;
		export struct chamber_manifold_dump;

		export using tile_coord = math::point2;
		export using tile_region = math::irect;

		export enum struct damage_consume_result{
			success,
			damage_exhaust,
			hitpoint_exhaust,

		};

		struct building_meta{
			math::vector2<unsigned short> size;


			//hitpoint
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
		struct building : ui_builder{
			friend building_data;
			friend game::ecs::component_custom_behavior<chamber::building_data>;
			friend game::ecs::component_custom_behavior<chamber::building>;

		private:
			building_data* data_{};

		protected:
			[[nodiscard]] float get_update_delta() const noexcept;

			[[nodiscard]] math::vec2 get_local_to_global(math::vec2 p) const noexcept;
			[[nodiscard]] math::trans2 get_local_to_global_trans(math::vec2 p) const noexcept;
			[[nodiscard]] math::trans2 get_local_to_global_trans(math::trans2 p) const noexcept;

		public:
			[[nodiscard]] building_data& data() const noexcept{
				assert(data_ != nullptr);
				return *data_;
			}

			~building() override = default;

			virtual void draw_hud(graphic::renderer_ui& renderer) const{

			}

			virtual void update(const chunk_meta& chunk_meta, world::entity_top_world& top_world){

			}
		};

		export
		template <typename ...Bases>
		struct building_trait_base{
			using base_types = std::tuple<building, ui_builder, Bases ...>;
		};

		struct tile_damage{
			float building_damage;
			float structural_damage;

			float sum() const noexcept{
				return building_damage + structural_damage;
			}
		};

		export
		struct tile_status{
			static constexpr float threshold_factor = 1 / 4.f;
			float valid_hit_point{};
			float valid_structure_hit_point{};

			constexpr tile_damage take_damage(float damage, float tile_max) noexcept{
				float structure_damage_threshold = tile_max * threshold_factor;

				float build_consumed;
				if(damage >= valid_hit_point){
					build_consumed = valid_hit_point;
					valid_hit_point = 0;
				}else{
					build_consumed = damage;
					valid_hit_point -= damage;
				}

				damage -= build_consumed;

				float struct_consumes{};
				if(valid_hit_point + build_consumed < structure_damage_threshold){
					float factor = 1 - (valid_hit_point + build_consumed) / structure_damage_threshold;
					auto dmg = factor * math::min(damage, valid_structure_hit_point);
					auto last = valid_structure_hit_point;
					valid_structure_hit_point -= dmg;
					valid_structure_hit_point = math::max(valid_structure_hit_point, valid_hit_point);
					struct_consumes = last - valid_structure_hit_point;
				}

				return {build_consumed, struct_consumes};
			}
		};


		struct building_data{
			friend chamber_grid;
			friend chamber_manifold;
			friend ecs::component_custom_behavior<building_data>;
			friend chamber_manifold_dump;

		private:
			tile_region region_{};
			const chunk_meta* component_head_{};
			chamber_manifold* grid_{};

		public:
			hit_point hit_point{};

			std::vector<tile_damage_event> damage_events{};

			//TODO should this be public?
			std::vector<tile_status> tile_states{};
			float building_damage_take{};
			float structural_damage_take{};

		private:

		public:
			[[nodiscard]] tile_region region() const noexcept{
				return region_;
			}

			[[nodiscard]] chamber_manifold& grid() const noexcept{
				assert(grid_ != nullptr);
				return *grid_;
			}

			[[nodiscard]] const chunk_meta* get_component_head() const noexcept{
				return component_head_;
			}

			[[nodiscard]] const entity& entity() const noexcept{
				return *component_head_->id();
			}

			[[nodiscard]] entity_id get_id() const noexcept{
				return component_head_->id();
			}

			decltype(auto) operator[](this auto&& self, tile_coord global_pos) noexcept{
				return self.tile_states[self.local_to_index(global_pos - self.region_.src)];
			}

			decltype(auto) operator[](this auto&& self, const tile& tile) noexcept;

			[[nodiscard]] float get_tile_individual_max_hitpoint() const noexcept{
				return hit_point.max / region_.area() * 2;
			}

			template <std::integral T = tile_coord::value_type>
			[[nodiscard]] std::size_t local_to_index(math::vector2<T> coord) const noexcept{
				return coord.x + coord.y * region_.width();
			}

			void clear_hit_events() noexcept{
				damage_events.clear();
				structural_damage_take = building_damage_take = 0;
			}


			damage_consume_result consume_damage(tile_coord coord, damage_group& total_damage){
				// if(critical_damage_take > hit_point.cur)return damage_consume_result::hitpoint_exhaust;
				auto sum = total_damage.sum();

				if(sum <= 0.f){
					return damage_consume_result::damage_exhaust;
				}

				auto local = (coord - region_.src);
				auto dmg = tile_states[local_to_index(local)].take_damage(sum, get_tile_individual_max_hitpoint());

				if(dmg.sum() > 0.){
					building_damage_take += dmg.building_damage;
					structural_damage_take += dmg.structural_damage;
					damage_events.emplace_back(local.as<unsigned short>(), sum);
					total_damage.material_damage.direct -= dmg.sum();
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
				const tile_region& region, chamber_manifold* grid)
				: region_(region),
				  grid_(grid){
				if (region.area() == 0){
					throw std::invalid_argument("tile region has no area");
				}
			}

			[[nodiscard]] constexpr math::frect get_bound() const noexcept{
				return region_.as<float>().scl(tile_size, tile_size);
			}

			math::trans2 get_trans() const noexcept;
		};

	}


	// export
	// template <>
	// struct component_custom_behavior<chamber::building> : component_custom_behavior_base<chamber::building>{
	// 	using base_types = ui_builder;
	//
	// 	static void on_relocate(const chunk_meta& meta, value_type& comp){
	// 		comp.data_ = std::addressof(meta.id()->at<chamber::building_data>());
	// 	}
	// };

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
			return building.data()[tile_pos];
		}
	};


	decltype(auto) building_data::operator[](this auto&& self, const tile& tile) noexcept{
		return std::forward_like<decltype(self)>(self[tile.tile_pos]);
	}
}
