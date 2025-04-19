module;

#include <cassert>

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

namespace mo_yanxi::game::ecs::chamber{

	export
	template <>
	struct quad_tree_trait_adaptor : quad_tree_adaptor_base<tile, float>{
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

	class tile_grid{
		friend grid_instance;
	public:
		using tile_type = tile;

		using grid_type = dim2::grid<tile_type>;

		void insert(entity_id building){
			assert(building);

			auto& region = building->at<building_data>();
			wrap_bound_.expand_by(region.get_bound());

			const bool quad_tree_valid = quad_tree_.get_boundary().contains_loose(wrap_bound_);

			assert(!region.grid);
			region.region.each([&, this](tile_coord coord){
				auto& tile = tiles[coord];
				assert(!tile.building);
				tile.building = building;
				if(quad_tree_valid){
					quad_tree_.insert(tile);
				}
			});

			if(!quad_tree_valid){
				rebuild_tree();
			}
		}

		void erase(entity_id building) noexcept{
			assert(building);

			auto& region = building->at<building_data>();
			region.region.each([&, this](tile_coord coord){
				tile_type& tile = tiles.tile_at(coord);
				assert(tile.building == building);
				quad_tree_.erase(tile);
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

		[[nodiscard]] auto* find(tile_coord coord) const noexcept{
			return tiles.find(coord);
		}

		[[nodiscard]] auto* find(tile_coord coord) noexcept{
			return tiles.find(coord);
		}

	private:
		math::frect wrap_bound_{};
		quad_tree<tile_type> quad_tree_{{}};
		grid_type tiles{};

		void rebuild_tree(){
			decltype(quad_tree_) new_quad_tree{wrap_bound_};
			quad_tree_.each([&, this](tile_type& coord){
				new_quad_tree.insert(std::move(coord));
			});
			quad_tree_ = std::move(new_quad_tree);
		}
	};

	struct GridBoxCollidePred{
		constexpr static bool operator()(const math::rect_box<float>& rect, const math::frect& tileBound) noexcept{
			return rect.overlap_rough(tileBound) && rect.overlap_exact(tileBound);
		}
	};



	export
	template <>
	struct component_custom_behavior<building_data> : component_custom_behavior_base<building_data>{
		static void on_init(const chunk_meta& meta, const component<value_type>& comp);

		static void on_terminate(const chunk_meta& meta, const component<value_type>& comp);
	};


	template <typename Tpl>
	struct building_archetype : archetype<Tpl>{
		tile_grid* grid{};

		[[nodiscard]] explicit building_archetype(tile_grid& grid)
			: grid(&grid){
		}

		void terminate(typename archetype<Tpl>::components& comps) override{
			auto& meta = archetype_base::meta_of(comps);
			grid->erase(meta);
		}

		void init(typename archetype<Tpl>::components& comps) override{
			auto& meta = archetype_base::meta_of(comps);
			grid->insert(meta);
		}
	};

	class grid_instance{
	public:
		using grid = tile_grid;
		grid local_grid{};

		component_manager manager{};


		struct in_viewports{
			std::vector<grid::tile_type*> tiles{};
			std::vector<entity_id> builds{};
			math::rect_box<float> viewport_in_local{};

			[[nodiscard]] constexpr bool empty() const noexcept{
				return tiles.empty() && builds.empty();
			}

			void update(grid_instance& grid, const math::rect_box<float>& viewport_in_global) noexcept{
				viewport_in_local = grid.box_to_local(viewport_in_global);
				tiles.clear();
				builds.clear();

				grid.local_grid.quad_tree_.intersect_then(viewport_in_local, GridBoxCollidePred{},
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

		template <math::quad_like T>
		[[nodiscard]] constexpr T box_to_local (const T& brief) const noexcept{
			return T{
				get_transform().apply_inv_to(brief.v00()),
				get_transform().apply_inv_to(brief.v10()),
				get_transform().apply_inv_to(brief.v11()),
				get_transform().apply_inv_to(brief.v01()),
			};
		}

		void update_transform(math::trans2 trans){
			static constexpr math::vec2 Offset = math::vec2{tile_size, tile_size} / 2.f;
			trans.vec -= Offset.copy().rotate(trans.rot);

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

		template <tuple_spec Tuple, typename Args>
		building_ref add_building(tile_region region, Args&& ...args){
			static_assert(!std::same_as<building_data, std::tuple_element_t<0, Tuple>>);
			using building_ty = tuple_cat_t<std::tuple<building_data>, Tuple>;
			return manager.create_entity_deferred<building_ty>(building_data{
				.region = region,
				.grid = this
			}, std::forward<Args>(args)...);
		}

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
	};


	template <>
	void component_custom_behavior<>::on_init(const chunk_meta& meta, const component<value_type>& comp){
		comp.grid->local_grid.insert(meta.id());
	}

	template <>
	void component_custom_behavior<>::on_terminate(const chunk_meta& meta, const component<value_type>& comp){
		comp.grid->local_grid.erase(meta.id());
	}
}
