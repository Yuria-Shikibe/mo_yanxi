/*
module;

#include <cassert>

export module Game.Chamber.Grid;

export import Game.Chamber;
export import mo_yanxi.dim2.grid;
export import ext.open_addr_hash_map;
export import ext.referenced_ptr;
export import Geom.quad_tree;

export import Geom.Transform;
export import Geom.Shape.RectBox;

import std;

namespace Game::Chamber{
	// using ETy = int;

	export
	template <
		typename ETy,
		typename /*std::derived_from<Building<ETy>>#1# SpecBuildType = Building<ETy>,
		typename /*std::derived_from<Tile<ETy, SpecBuildType>>#1# TileTy = Tile<ETy, SpecBuildType>>
	class Grid{
	public:
		using EntityType = ETy;
		using TileType = TileTy;
		using BuildType = SpecBuildType;

		// static_assert(std::same_as<typename TileType::BuildType, SpecBuildType>);

		using BuildPtr = ext::referenced_ptr<BuildType>;
		using GridType = ext::dim2::grid<TileType>;
		static constexpr Geom::OrthoRectFloat DefaultTreeBound{Geom::Vec2{}, TileSize * 256};

		void add_placeable(const Geom::Point2 where){

		}

		void add(BuildPtr&& build){
			assert(build != nullptr);
			assert(build->prop.bound.area() > 0);

			if constexpr (requires (BuildType& b, Grid& self){
				b << self;
			}){
				*build << *this;
			}

			build->prop.bound.each([&, this](const Geom::Point2 pos){
				auto& tile = addTile(pos);
				assert(tile.build == nullptr);
				tile.build = build;
			});
			minimumWrapBound.expandBy(build->getBound());


			const auto [itr, suc] = builds.try_emplace(build->prop.bound.src, std::move(build));
			assert(suc);
		}

		bool erase_build(const Geom::Point2 where) noexcept requires (std::derived_from<SpecBuildType, Building<ETy>>){
			//TODO how to shrink the bound??

			if (const auto itr = builds.find(where); itr != builds.end()){
				assert(itr->second != nullptr);

				itr->second->prop.bound.each([&, this](const Geom::Point2 pos){
					TileType& tile = tiles[pos];
					tile.resetBuild();
					quadTree.remove(tile);
				});


				builds.erase(itr);
				return true;
			}

			if(const auto& tile = tiles.find(where)){
				if(tile->build){
					assert(builds.contains(tile->build->pos()));
					return this->erase_build(tile->build->pos());
				}
			}

			return false;
		}

		[[nodiscard]] Geom::OrthoRectFloat getWrapBound() const noexcept{
			return minimumWrapBound;
		}

		void each_tile(const Geom::OrthoRectInt region, std::invocable<TileType&> auto fn) {
			for(int x = region.getSrcX(); x < region.getEndX(); ++x){
				for(int y = region.getSrcY(); y < region.getEndY(); ++y){
					if(auto tile = tiles.find({x, y})){
						std::invoke(fn, *tile);
					}
				}
			}
		}


	private:
		Geom::OrthoRectFloat minimumWrapBound{};

		TileType& addTile(Geom::Point2 where){
			TileType& tile = tiles[where];
			const auto inserted = quadTree.insert(tile);
			assert(inserted);
			return tile;
		}

	public:
		//TODO make these private
		GridType tiles{};

		ext::fixed_open_addr_hash_map<
			Geom::Point2, BuildPtr,
			Geom::Point2, (Geom::Vec::constant<int>::lowest_vec2)>
			builds{};

		Geom::quad_tree<TileType> quadTree{DefaultTreeBound};

		void shrink_quad_tree_to_fit_builds(){
			tree_resize(getBuildsBound());
		}

	private:
		[[nodiscard]] Geom::OrthoRectFloat getBuildsBound() const noexcept{
			if(builds.empty()){
				return {};
			}

			Geom::OrthoRectFloat initial = builds.begin()->second->getBound();

			for (const auto & build : builds){
				initial.expandBy(build.second->getBound());
			}

			return initial;
		}

		void tree_resize(const Geom::OrthoRectFloat bound){
			quadTree = Geom::quad_tree<TileType>{bound};

			for (typename GridType::value_type& chunk : tiles | std::views::values){
				for (auto && build : chunk){
					quadTree.insert(build);
				}

			}
		}
	};

	export
	template <
		typename ETy,
		typename TileTy>
	using GridOf = Grid<ETy, typename TileTy::BuildType, TileTy>;

	export
	struct GridBoxCollidePred{
		constexpr bool operator()(const Geom::OrthoRectFloat& tileBound, const Geom::RectBoxBrief& rect) const noexcept{
			return rect.overlapRough(tileBound) && rect.overlapExact(tileBound);
		}
	};

	export
	template <
		typename ETy,
		typename /*std::derived_from<Building<ETy>>#1# SpecBuildType = Building<ETy>,
		typename /*std::derived_from<Tile<ETy, SpecBuildType>>#1# TileTy = Tile<ETy, SpecBuildType>>
	class Grid_InWorld{
	public:
		using Grid = Grid<ETy, SpecBuildType, TileTy>;
		Grid localGrid{};

		struct InViewports{
			std::vector<typename Grid::TileType*> tiles{};
			std::vector<typename Grid::BuildType*> builds{};
			Geom::RectBoxBrief viewport_in_local{};

			[[nodiscard]] constexpr bool empty() const noexcept{
				return tiles.empty() && builds.empty();
			}

			void update(const Grid_InWorld& grid, const Geom::RectBoxBrief& viewport_in_global) noexcept{
				// const auto next = ;

				viewport_in_local = grid.boxToLocal(viewport_in_global);
				tiles.clear();
				builds.clear();

				std::unordered_set<typename Grid::BuildType*> unique_builds{};

				grid.localGrid.quadTree.intersect_then(viewport_in_local, GridBoxCollidePred{},
					[&, this](typename Grid::TileType& tile, auto&&){
					tiles.push_back(&tile);
					unique_builds.insert(tile.build.get());
				});

				builds = {std::from_range, unique_builds};
			}
		};

	public:

		[[nodiscard]] constexpr Geom::RectBoxBrief boxToLocal (const Geom::RectBoxBrief& brief) const noexcept{
			return Geom::RectBoxBrief{
				getTransform().applyInvTo(brief.v0),
				getTransform().applyInvTo(brief.v1),
				getTransform().applyInvTo(brief.v2),
				getTransform().applyInvTo(brief.v3),
			};
		}

		void updateTransform(Geom::Transform trans){
			static constexpr Geom::Vec2 Offset = Geom::Vec2{TileSize, TileSize} / 2.f;
			trans.vec -= Offset.copy().rotate(trans.rot);

			if(transform != trans){
				transform = trans;

				const auto bound = localGrid.getWrapBound();
				wrapper = {
					bound.vert_00() | trans,
					bound.vert_10() | trans,
					bound.vert_11() | trans,
					bound.vert_01() | trans,
				};
			}
		}

		[[nodiscard]] Geom::Transform getTransform() const noexcept{
			return transform;
		}

		[[nodiscard]] const Geom::RectBoxBrief& getWrapper() const noexcept{
			return wrapper;
		}

		void updateInViewport(const Geom::RectBoxBrief& viewport_in_global){
			inViewports.update(*this, viewport_in_global);
		}

		[[nodiscard]] const InViewports& getInViewports() const{
			return inViewports;
		}

		bool empty() const noexcept{
			return localGrid.tiles.empty();
		}

	private:
		/**
		 * @brief local to global
		 #1#
		Geom::Transform transform{};

		/**
		 * @brief Wrapper In Global Coordinate
		 #1#
		Geom::RectBoxBrief wrapper{};

		InViewports inViewports{};

		//TODO changed_tag?
	};
}
*/
