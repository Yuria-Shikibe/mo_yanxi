module;

#include <cassert>
#include <gch/small_vector.hpp>

export module mo_yanxi.game.ecs.component.chamber:grid;

export import mo_yanxi.game.ecs.component.manager;

export import mo_yanxi.dim2.grid;
export import mo_yanxi.open_addr_hash_map;
export import mo_yanxi.math.trans2;
export import mo_yanxi.math.rect_ortho;
export import mo_yanxi.math.quad;
export import mo_yanxi.game.ecs.quad_tree;

import mo_yanxi.concepts;


import :tile;

import std;

namespace mo_yanxi::game{
	// using namespace chamber;

	export
	template <>
	struct quad_tree_trait_adaptor<ecs::chamber::tile, float> : quad_tree_adaptor_base<ecs::chamber::tile, float>{
		[[nodiscard]] static rect_type get_bound(const_reference self) noexcept{
			return self.get_bound();
		}

		[[nodiscard]] static bool intersect_with(const_reference self, const_reference other) = delete;

		[[nodiscard]] static bool contains(const_reference self, typename vector_type::const_pass_t point){
			return self.get_bound().contains(point);
		}

		[[nodiscard]] static bool equals(const_reference self, const_reference other) noexcept{
			return self.tile_pos == other.tile_pos;
		}
	};


	

}

namespace mo_yanxi::game::ecs::chamber{

	class tile_grid{
		friend chamber_grid;
	public:
		using tile_type = tile;

		using grid_type = dim2::grid<tile_type, unsigned, {16u, 16u}>;

		void insert(entity_id building){
			assert(building);

			auto& region = building->at<building_data>();
			if(wrap_bound_.area() == 0){
				wrap_bound_ = region.get_bound();
			}else{
				wrap_bound_.expand_by(region.get_bound());
			}


			const bool quad_tree_valid = quad_tree_->get_boundary().contains_loose(wrap_bound_);

			// assert(!region.grid);
			region.region().each([&, this](tile_coord coord){
				auto& tile = tiles[coord];
				assert(!tile.building);
				tile.building = building;
				if(quad_tree_valid){
					quad_tree_->insert(tile);
				}
			});

			if(!quad_tree_valid){
				rebuild_tree();
			}
		}

		void erase(entity_id building) noexcept{
			assert(building);

			auto& region = building->at<building_data>();
			region.region().each([&, this](tile_coord coord){
				tile_type& tile = tiles.tile_at(coord);
				assert(tile.building == building);
				quad_tree_->erase(tile);
				tile.building = nullptr;
			});
		}

		[[nodiscard]] math::frect get_wrap_bound() const noexcept{
			return wrap_bound_;
		}

		void optimize() noexcept{
			grid_type new_grid{};
			std::optional<math::frect> min_bound;

			for (auto&& [chunk_coord, chunk] : tiles.chunks){
				for (auto&& tile : chunk){
					if(!tile.building)continue;

					if(min_bound){
						min_bound->expand_by(tile.get_bound());
					}else{
						min_bound = tile.get_bound();
					}

					new_grid[tile.tile_pos] = std::move(tile);
				}
			}

			wrap_bound_ = min_bound.value();
			tiles = std::move(new_grid);
			rebuild_tree();
		}

		void each_tile(const tile_region region, std::invocable<tile_type&> auto fn) {
			region.each([&](tile_coord coord){
				if(auto tile = tiles.find(coord)){
					std::invoke(fn, *tile);
				}
			});
		}

		void each_tile(const tile_region region, std::invocable<const tile_type&> auto fn) const {
			region.each([&](tile_coord coord){
				if(auto tile = tiles.find(coord)){
					std::invoke(fn, *tile);
				}
			});
		}

		void each_tile(std::invocable<tile_type&> auto fn) {
			for (auto && value : tiles){
				for (auto && second : value.second){
					std::invoke(fn, second);
				}
			}
		}

		void each_tile(std::invocable<const tile_type&> auto fn) const {
			for (auto && value : tiles){
				for (auto && second : value.second){
					std::invoke(fn, second);
				}
			}
		}

		[[nodiscard]] auto* find(tile_coord coord) const noexcept{
			return tiles.find(coord);
		}

		[[nodiscard]] auto* find(tile_coord coord) noexcept{
			return tiles.find(coord);
		}

		[[nodiscard]] game::quad_tree<tile_type>& quad_tree(){
			return quad_tree_;
		}

		[[nodiscard]] const game::quad_tree<tile_type>& quad_tree() const{
			return quad_tree_;
		}

	private:
		math::frect wrap_bound_{};
		game::quad_tree<tile_type> quad_tree_{{}};
		grid_type tiles{};

		void rebuild_tree(){
			std::destroy_at(&quad_tree_);
			std::construct_at(&quad_tree_, wrap_bound_.copy().expand(tile_size * 2));

			each_tile([this](const tile_type& tile){
				if(tile.building){
					quad_tree_->insert(tile);
				}
			});
		}
	};

	export
	struct box_tile_collider{
		constexpr static bool operator()(const math::rect_box<float>& rect, const math::frect& tileBound) noexcept{
			return rect.overlap_rough(tileBound) && rect.overlap_exact(tileBound);
		}
	};

	//TODO add archetype trait instead of derive from a archetype

	struct pierce_comp{
		math::vec2 pos;
		math::vec2 nor;

		constexpr bool operator()(math::vec2 lhs, math::vec2 rhs) const noexcept{
			using namespace math;
			const auto baseline = nor.copy().rotate_rt_clockwise();

			lhs -= pos;
			rhs -= pos;

			const auto lhs_len2 = lhs.length2();
			const auto rhs_len2 = rhs.length2();

			const auto lhs_nor_dot  = lhs.dot(nor);
			const auto lhs_tan_dot  = lhs.dot(baseline);

			const auto rhs_nor_dot  = rhs.dot(nor);
			const auto rhs_tan_dot  = rhs.dot(baseline);

			const auto lhs_nor_dst2 = lhs_len2 - sqr(lhs_nor_dot);
			const auto lhs_tan_dst2 = max(lhs_len2 - sqr(lhs_tan_dot), tile_size / 2.f);

			const auto rhs_nor_dst2 = rhs_len2 - sqr(rhs_nor_dot);
			const auto rhs_tan_dst2 = max(rhs_len2 - sqr(rhs_tan_dot), tile_size / 2.f);

			const auto lhs_nor_max_dst = max(lhs_nor_dst2 * .55f, tile_size / 2.f) + tile_size * .65f;
			const auto rhs_nor_max_dst = max(rhs_nor_dst2 * .55f, tile_size / 2.f) + tile_size * .65f;

			return lhs_tan_dst2 * lhs_nor_max_dst < rhs_tan_dst2 * rhs_nor_max_dst;
		}
	};

	export
	class chamber_grid{
	public:
		using grid = tile_grid;

		struct in_viewports{
			std::vector<grid::tile_type*> tiles{};
			std::vector<entity_id> builds{};
			math::rect_box<float> viewport_in_local{};

			[[nodiscard]] constexpr bool empty() const noexcept{
				return tiles.empty() && builds.empty();
			}

			void update(chamber_grid& grid, const math::rect_box<float>& viewport_in_global) noexcept{
				viewport_in_local = grid.box_to_local(viewport_in_global);
				tiles.clear();
				builds.clear();

				grid.local_grid.quad_tree_->intersect_then(
					viewport_in_local,
					box_tile_collider::operator(),
					[&, this](auto&&, grid::tile_type& tile){
					tiles.push_back(&tile);
					if(tile.building){
						if(const auto itr = std::ranges::lower_bound(builds, tile.building.id()); *itr != tile.building){
							builds.insert(itr, tile.building.id());
						}
					}
				});
			}
		};

		[[nodiscard]] chamber_grid() = default;
		component_manager manager{};
		grid local_grid{};

	private:
		/**
		 * @brief local to global
		 */
		math::trans2 transform{};

		/**
		 * @brief Wrapper In Global Coordinate
		 */
		math::rect_box<float> wrapper{};

		in_viewports inViewports{};
	public:

		template <math::quad_like T>
		[[nodiscard]] constexpr T box_to_local (const T& brief) const noexcept{
			return T{
				get_transform().apply_inv_to(brief.v00()),
				get_transform().apply_inv_to(brief.v10()),
				get_transform().apply_inv_to(brief.v11()),
				get_transform().apply_inv_to(brief.v01()),
			};
		}

		template <std::integral T>
		[[nodiscard]] constexpr math::vec2 local_to_global(math::vector2<T> coord) const noexcept{
			return (coord.template as<decltype(tile_size_integral)>() * tile_size_integral).template as<float>().add(tile_size / 2.f) | transform;
		}

		void update_transform(math::trans2 trans){
			static constexpr math::vec2 Offset = math::vec2{tile_size, tile_size} / 2.f;
			trans.vec -= Offset.copy().rotate_rad(trans.rot);

			if(transform != trans){
				transform = trans;

				const auto bound = local_grid.get_wrap_bound();
				wrapper = math::rect_box<float>{
					bound.vert_00() | trans,
					bound.vert_10() | trans,
					bound.vert_11() | trans,
					bound.vert_01() | trans,
				};
			}
		}

		[[nodiscard]] math::trans2 get_transform() const noexcept{
			return transform;
		}

		[[nodiscard]] const math::rect_box<float>& get_wrap_bound() const noexcept{
			return wrapper;
		}

		void update_in_viewport(const math::rect_box<float>& viewport_in_global){
			inViewports.update(*this, viewport_in_global);
		}

		[[nodiscard]] const in_viewports& get_in_viewports() const{
			return inViewports;
		}

		template <tuple_spec Tuple>
		void add_building_type(){
			if constexpr (std::same_as<building_data, std::tuple_element_t<0, Tuple>>){
				manager.add_archetype<Tuple>();
			}else{
				using building_ty = tuple_cat_t<std::tuple<building_data>, Tuple>;
				manager.add_archetype<building_ty>();
			}
		}

		template <tuple_spec Tuple, typename ...Args>
		building_ref add_building(tile_region region, Args&& ...args){
			if(region.area() == 0){
				throw std::invalid_argument("invalid region");
			}

			if constexpr (std::same_as<building_data, std::tuple_element_t<0, Tuple>>){
				return manager.create_entity_deferred<Tuple>(building_data{
					region, this
				}, std::forward<Args>(args)...);
			}else{
				using building_ty = tuple_cat_t<std::tuple<building_data>, Tuple>;
				return manager.create_entity_deferred<building_ty>(building_data{
					region, this
				}, std::forward<Args>(args)...);
			}
		}



		[[nodiscard]] std::vector<tile> get_dst_sorted_tiles(
			this const chamber_grid& grid,
			const math::frect_box& hitter_box_in_global,
			math::vec2 basepoint_in_global,
			math::nor_vec2 normal_in_global
		){
			const auto localBox = grid.box_to_local(hitter_box_in_global);

			const auto trans = grid.get_transform();
			math::vec2 pos{trans.apply_inv_to(basepoint_in_global)};
			const math::vec2 vel{normal_in_global.rotate_rad(-static_cast<float>(trans.rot))};

			std::vector<tile> under_hit;
			grid.local_grid.quad_tree()->intersect_then(
				localBox,
				box_tile_collider{},
				[&](const math::frect_box& box, const tile& tile){
					auto dot = (tile.get_real_pos() - pos).dot(vel);
					if(dot < 0.0f){
						pos += vel * dot;
					}
					under_hit.push_back(tile);
				});

			std::ranges::sort(under_hit, pierce_comp{pos, vel}, &tile::get_real_pos);
			return under_hit;
		}


		chamber_grid(chamber_grid&& other) noexcept
			: manager{(other.manager.do_deferred(), std::move(other.manager))},
			  local_grid{std::move(other.local_grid)},
			  transform{std::move(other.transform)},
			  wrapper{std::move(other.wrapper)},
			  inViewports{std::move(other.inViewports)}{

			manager.do_deferred();
			manager.sliced_each([this](building_data& data){
				data.grid_ = this;
			});
		}

		chamber_grid& operator=(chamber_grid&& other) noexcept{
			if(this == &other) return *this;
			other.manager.do_deferred();

			manager = std::move(other.manager);
			local_grid = std::move(other.local_grid);
			transform = std::move(other.transform);
			wrapper = std::move(other.wrapper);
			inViewports = std::move(other.inViewports);

			manager.sliced_each([this](building_data& data){
				data.grid_ = this;
			});
			return *this;
		}
	};


	export
	struct chamber_manifold : chamber_grid{
		[[nodiscard]] chamber_manifold() = default;
		// std::vector<chamber::tile> under_hit{};
	};
}

namespace mo_yanxi::game::ecs{
	using namespace chamber;

	export
	template <>
	struct component_custom_behavior<chamber::building_data> : component_custom_behavior_base<chamber::building_data>{
		static void on_init(const chunk_meta& meta, value_type& comp){
			comp.tile_states.resize(comp.region().area());
			comp.grid()->local_grid.insert(meta.id());
		}

		static void on_terminate(const chunk_meta& meta, const value_type& comp){
			comp.grid()->local_grid.erase(meta.id());
		}
	};
}
