module;

#include <cassert>
#include <plf_hive.h>

export module mo_yanxi.game.ecs.component.chamber:grid_meta;

import :chamber_meta;
import mo_yanxi.game.meta.hitbox;
import mo_yanxi.open_addr_hash_map;
import mo_yanxi.algo;
import mo_yanxi.algo.timsort;

namespace mo_yanxi::game::meta::chamber{
	export constexpr inline int tile_size_integral = ecs::chamber::tile_size_integral;
	export constexpr inline float tile_size = tile_size_integral;

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

		void try_draw(math::ivec2 origin_offset, graphic::renderer_ui_ref renderer, const float camera_scale) const{
			if(auto* p = get_instance_data()){
				p->draw(
					get_meta_info(),
					math::irect{tags::from_extent, identity_pos.as<int>() + origin_offset, meta_info->extent.as<int>()}.
					scl(tile_size_integral, tile_size_integral).as<float>(),
					renderer, camera_scale
				);
			}
		}
	};


	struct cascade_unqualified_buildings{
		std::vector<math::upoint2> building_pos{};
	};


	export
	struct grid_complex{
		plf::hive<grid_building> buildings{};
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
}

namespace mo_yanxi::game{

	// template <>
	// struct quad_tree_trait_adaptor<const meta::chamber::grid_building*, int> : quad_tree_adaptor_base<const meta::chamber::grid_building*, int>{
	// 	[[nodiscard]] static rect_type get_bound(const_reference self) noexcept{
	// 		return self->get_indexed_region().as<int>().expand(self->get_meta_info().structural_adjacent_distance());
	// 	}
	//
	// 	[[nodiscard]] static bool contains(const_reference self, vector_type::const_pass_t point){
	// 		return get_bound(self).contains(point);
	// 	}
	// };

	namespace meta::chamber{
		export
		struct grid{
			using index_coord = math::upoint2;

		private:

			/**
			 * @brief Coord of the tile to be the real original tile after instancing
			 */
			math::point2 origin_coord_{};
			math::usize2 extent_{};
			std::vector<grid_tile> tiles_{};
			plf::hive<grid_building> buildings_{};

			struct structural_status{
			private:
				struct tile_structural_status{
					unsigned char is_source : 1;
					unsigned char count : 7;

					bool any() const noexcept{
						return count > 0;
					}

				};

				std::vector<tile_structural_status> valid_indexed_points_{};
				unsigned count{};

				tile_structural_status& value_at(math::upoint2 pos, math::usize2 extent) noexcept{
					return  valid_indexed_points_[pos.x + pos.y * extent.x];
				}

			public:
				[[nodiscard]] const tile_structural_status& value_at(math::upoint2 pos, math::usize2 extent) const noexcept{
					return  valid_indexed_points_[pos.x + pos.y * extent.x];
				}

				[[nodiscard]] structural_status() = default;

				[[nodiscard]] explicit structural_status(math::isize2 region)
					: valid_indexed_points_(region.area()){
				}

				[[nodiscard]] bool empty() const noexcept{
					return count == 0;
				}

				void update_energy_status_on_erase(const grid_building& building, math::usize2 extent) noexcept{
					if(auto dst = building.get_meta_info().structural_adjacent_distance()){
						--count;

						building.get_indexed_region().each([&, this](math::upoint2 pos){
							value_at(pos, extent).is_source = false;
						});

						building.get_indexed_region().as<int>().expand(dst).intersection_with(math::irect{extent}).as<unsigned>().each([&, this](math::upoint2 pos){
							--value_at(pos, extent).count;
						});

						//TODO erase proximity check
					}
				}

				void update_energy_status_on_add(const grid_building& building, math::usize2 extent) {
					if(auto dst = building.get_meta_info().structural_adjacent_distance()){
						++count;

						building.get_indexed_region().each([&, this](math::upoint2 pos){
							value_at(pos, extent).is_source = true;
						});

						building.get_indexed_region().as<int>().expand(dst).intersection_with(math::irect{extent}).as<unsigned>().each([&, this](math::upoint2 pos){
							++value_at(pos, extent).count;
						});
					}
				}

				[[nodiscard]] bool is_within_structural(math::urect indexed_region, math::usize2 extent) const noexcept{
					return indexed_region.each_any([&, this](math::upoint2 pos){
						return value_at(pos, extent).count;
					});

					// return quad_tree_->intersect_any(indexed_region.as<int>()) != nullptr;
				}

				[[nodiscard]] bool is_within_structural(math::upoint2 indexed_pos, math::usize2 extent) const noexcept{
					return value_at(indexed_pos, extent).count;

					// return quad_tree_->intersect_any(indexed_region.as<int>()) != nullptr;
				}


			};

			struct energy_status{
				friend grid;

			private:
				std::vector<grid_building*> energy_consumer{};
				std::vector<grid_building*> energy_generator{};

				unsigned maximum_energy_generate_{};
				unsigned maximum_energy_consume_{};

				void update_energy_status_on_erase(grid_building& building) noexcept{
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

				void update_energy_status_on_add(grid_building& building){
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
				[[nodiscard]] auto max_generate() const noexcept{
					return maximum_energy_generate_;
				}

				[[nodiscard]] auto max_consume() const noexcept{
					return maximum_energy_consume_;
				}

				[[nodiscard]] std::span<const grid_building* const> generators() const noexcept{
					return energy_generator;
				}

				[[nodiscard]] std::span<const grid_building* const> consumers() const noexcept{
					return energy_consumer;
				}

			};

			energy_status energy_status_{};
			structural_status structural_status_{};

		public:

			[[nodiscard]] grid() noexcept = default;

			[[nodiscard]] explicit grid(const hitbox& hitbox);

			[[nodiscard]] grid(const math::usize2 extent, const math::point2 origin_coord, std::span<const grid_tile> tiles) :
				origin_coord_(origin_coord), extent_(extent), tiles_(std::from_range, tiles), structural_status_(extent.as<int>()){}

			[[nodiscard]] grid(const math::usize2 extent, const math::point2 origin_coord, std::vector<grid_tile>&& tiles) noexcept :
				origin_coord_(origin_coord), extent_(extent), tiles_(std::move(tiles)), structural_status_(extent.as<int>()){}

			[[nodiscard]] const energy_status& get_energy_status() const noexcept{
				return energy_status_;
			}

			[[nodiscard]] math::vector2<int> get_origin_coord() const noexcept{
				return origin_coord_;
			}

			[[nodiscard]] math::vector2<int> get_origin_offset() const noexcept{
				return -origin_coord_;
			}

			[[nodiscard]] const decltype(buildings_)& get_buildings() const noexcept{
				return buildings_;
			}

			[[nodiscard]] std::span<const grid_tile> get_tiles() const noexcept{
				return tiles_;
			}

			void move(math::point2 offset) noexcept;

			[[nodiscard]] math::usize2 get_extent() const noexcept{
				return extent_;
			}

			static math::point2 pos_to_tile_coord(math::vec2 coord) noexcept{
				return coord.div(tile_size).floor().as<int>();
			}

			[[nodiscard]] math::irect get_grid_region() const noexcept{
				return math::irect{tags::from_extent, -origin_coord_, extent_.as<int>()};
			}

			[[nodiscard]] std::optional<index_coord> coord_to_index(math::point2 world_tile_coord) const noexcept{
				world_tile_coord += origin_coord_;
				if(!math::irect{extent_.as<int>()}.contains(world_tile_coord))return {};
				return world_tile_coord.as<unsigned>();
			}

			[[nodiscard]] std::optional<math::urect> coord_to_index(math::irect world_tile_region) const noexcept{
				world_tile_region.src += origin_coord_;
				if(!math::irect{extent_.as<int>()}.contains_loose(world_tile_region))return {};
				return world_tile_region.as<unsigned>();
			}

			grid_building* try_place_building_at(const index_coord indexed_pos, const basic_chamber& info){
				if(!is_building_placeable_at(indexed_pos, info))return nullptr;
				return &place_building_at(indexed_pos, info);
			}

			template <std::invocable<const grid_building&> Fn = std::identity>
			cascade_unqualified_buildings erase_building_at(const math::upoint2 indexed_pos, Fn before_erase = {}) noexcept {
				if(indexed_pos.beyond_equal(extent_))return {};

				const auto& tile = tile_at(indexed_pos);
				auto* b = tile.building;
				if(!b)return {};

				energy_status_.update_energy_status_on_erase(*b);
				structural_status_.update_energy_status_on_erase(*b, extent_);
				cascade_unqualified_buildings result{};

				if(const auto dst = b->get_meta_info().structural_adjacent_distance()){
					const auto region = b->get_indexed_region().as<int>().expand(dst).intersection_with({tags::unchecked, tags::from_extent, {}, extent_.as<int>()}).as<unsigned>();
					region.each([&, this](math::upoint2 pos){
						if(!std::as_const(structural_status_).value_at(pos, extent_).any()){
							if(auto bd = tile_at(pos).building){
								if(!std::ranges::contains(result.building_pos, bd->get_identity_pos())){
									result.building_pos.push_back(bd->get_identity_pos());
								}
							}
						}
					});

					std::erase_if(result.building_pos, [&, this](math::upoint2 pos){
						return structural_status_.is_within_structural(tile_at(pos).building->get_indexed_region(), extent_);
					});
				}

				std::invoke(before_erase, *b);
				tile.building->get_indexed_region().each([this](const math::upoint2 p){
					tile_at(p).building = nullptr;
				});

				const auto itr = buildings_.get_iterator(b);
				assert(itr != buildings_.end());
				buildings_.erase(itr);
				return result;
			}

			[[nodiscard]] bool is_building_placeable_at(const index_coord indexed_pos, const basic_chamber& info) const noexcept{
				const math::urect region{tags::from_extent, indexed_pos, info.extent};

				return is_building_basic_placeable_at(region) && is_building_structural_placeable_at(indexed_pos, info);
			}

			grid_building& place_building_at(const index_coord indexed_pos, const basic_chamber& info){
				auto& b = buildings_.insert(grid_building{indexed_pos.as<unsigned>(), info}).operator*();
				const math::urect region{tags::from_extent, indexed_pos.as<unsigned>(), info.extent};

				energy_status_.update_energy_status_on_add(b);
				structural_status_.update_energy_status_on_add(b, extent_);

				region.each([&](const math::upoint2 p){
					tile_at(p).building = std::addressof(b);
				});

				return b;
			}

			[[nodiscard]] bool is_building_basic_placeable_at(const math::urect region) const noexcept{
				if(!math::urect{extent_}.contains_loose(region))return false;

				for(auto y = region.src.y; y < region.get_end_y(); ++y){
					for(auto x = region.src.x; x < region.get_end_x(); ++x){
						auto& tile = tile_at({x, y});
						if(!tile.placeable || tile.building != nullptr)return false;
					}
				}

				return true;
			}

			bool is_building_structural_placeable_at(const index_coord indexed_pos, const basic_chamber& info) const noexcept{
				const math::urect region{tags::from_extent, indexed_pos, info.extent};
				if(auto dst = info.structural_adjacent_distance()){
					// if(!structural_status_.empty()){
					// 	if(
					// 		auto nearby = math::rect::get_proximity_of(region.as<int>());
					// 		std::ranges::none_of(nearby, [this](math::irect rect_ortho){
					//
					// 		return rect_ortho.each_any([this](math::point2 pos){
					// 			if(pos.x < 0 || pos.y < 0)return false;
					// 			return (bool)std::as_const(structural_status_).value_at(pos.as<unsigned>(), extent_).is_source;
					// 		});
					//
					// 	})){
					// 		return false;
					// 	}
					// }

				}else{
					if(!structural_status_.is_within_structural(region, extent_))return false;
				}

				return true;
			}

		public:

			[[nodiscard]] bool is_placeable_at(const math::urect region_in_index) const noexcept{
				for(unsigned y = 0; y < region_in_index.height(); ++y){
					for(unsigned x = 0; x < region_in_index.width(); ++x){
						if(!tile_at(region_in_index.src + math::vector2{x, y}).placeable)return false;
					}
				}

				return true;
			}

			[[nodiscard]] grid_tile& tile_at(const math::upoint2 index_pos) noexcept{
				assert(index_pos.within(extent_));
				return tiles_[extent_.x * index_pos.y + index_pos.x];
			}

			[[nodiscard]] const grid_tile& tile_at(const math::upoint2 index_pos) const noexcept{
				assert(index_pos.within(extent_));
				return tiles_[extent_.x * index_pos.y + index_pos.x];
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

			void draw(graphic::renderer_ui_ref renderer, const graphic::camera2& camera, math::point2 offset = {}, float opacity_scl = 1.f) const;

			void dump(ecs::chamber::chamber_manifold& clear_grid_manifold) const;
		};

		export
		struct grid_structural_statistic{
		private:
			struct structural_cluster{
				std::vector<const grid_building*> buildings;

				[[nodiscard]] structural_cluster() = default;

				[[nodiscard]] explicit(false) structural_cluster(const grid_building* idt) : buildings{idt}{

				}

				[[nodiscard]] const grid_building* identity() const noexcept{
					return buildings.front();
				}

				void add(const grid_building* b){
					buildings.push_back(b);
				}

				auto size() const noexcept{
					return buildings.size();
				}

				[[nodiscard]] float get_total_hitpoint() const noexcept{
					return std::ranges::fold_left(buildings | std::views::transform([](const grid_building* b){
						return b->get_meta_info().hit_point.max;
					}), 0.f, std::plus{});
				}
			};

			struct union_set_solver{
				struct cluster_node{
					const grid_building* identity{};
					const grid_building* parent{};

					[[nodiscard]] cluster_node() = default;

					[[nodiscard]] explicit(false) cluster_node(const grid_building& building) : identity(&building), parent{identity}{

					}

					bool operator==(const cluster_node&) const noexcept = default;

					auto operator<=>(const cluster_node& other) const noexcept{
						return std::compare_three_way{}(identity, other.identity);
					}
				};

				std::vector<cluster_node> structurals{};

				[[nodiscard]] union_set_solver() = default;

				[[nodiscard]] union_set_solver(const grid& grid) :
					structurals{
						std::from_range, grid.get_buildings() | std::views::filter([](const grid_building& b){
							return b.get_meta_info().structural_adjacent_distance();
						})
					}{
					algo::timsort(structurals);

					const math::irect bound{grid.get_extent().as<int>()};
					for(auto& structural : structurals){
						const auto nearby = math::rect::get_proximity_of(
							structural.identity->get_indexed_region().as<int>());
						for(const auto& rect_ortho : nearby){
							rect_ortho.each([&](math::point2 pos){
								if(!bound.contains(pos)) return;
								const auto building = grid.tile_at(pos.as<unsigned>()).building;
								if(building && building->get_meta_info().is_structural()){
									union_clusters(structural, (*this)[building]);
								}
							});
						}
					}
				}

				// 查找根节点（路径压缩）
				cluster_node& find(cluster_node& c) noexcept{
					if(c.parent != c.identity){
						auto root = find((*this)[c.parent]);
						c.parent = root.identity;
					}
					return (*this)[c.parent];
				}

				// 合并两个集群
				void union_clusters(cluster_node& a, cluster_node& b) noexcept{
					auto& rootA = find(a);
					auto& rootB = find(b);

					if(rootA != rootB){
						rootB.parent = rootA.identity;
					}
				}

				cluster_node& operator[](const grid_building* where) noexcept{
					return std::ranges::lower_bound(structurals, where, {}, &cluster_node::identity).operator*();
				}

				// 获取所有连通分量的方法
				[[nodiscard]] std::vector<structural_cluster> get_connected_components() {
					std::vector<structural_cluster> result{};

					for (const auto & structural : structurals){
						if(structural.identity == structural.parent){
							result.push_back({structural.identity});
						}
					}

					std::ranges::sort(result, {}, &structural_cluster::identity);

					for (auto& cluster : structurals) {
						auto& root = find(cluster);
						if(cluster.identity == root.identity)continue;
						std::ranges::lower_bound(result, root.identity, {}, &structural_cluster::identity)->add(cluster.identity);
					}

					return result;
				}
			};

			//clusters sort by identity
			std::vector<structural_cluster> clusters_{};
		public:
			[[nodiscard]] grid_structural_statistic() = default;

			[[nodiscard]] explicit(false) grid_structural_statistic(const grid& grid) : clusters_(union_set_solver{grid}.get_connected_components()){
			}

			[[nodiscard]] const structural_cluster& operator[](const grid_building* cluster_identity) const noexcept{
				const auto itr = std::ranges::lower_bound(clusters_, cluster_identity, {}, &structural_cluster::identity);
				assert(itr != clusters_.end());
				return *itr;
			}

			[[nodiscard]] auto size() const noexcept{
				return clusters_.size();
			}

			[[nodiscard]] std::span<const structural_cluster> get_clusters() const noexcept{
				return clusters_;
			}

			[[nodiscard]] float get_structural_hitpoint() const noexcept{
				std::size_t sum{};
				std::size_t max_count{};

				for (const auto & cluster : clusters_){
					max_count = std::ranges::max(max_count, cluster.size());
					sum += std::ranges::fold_left(cluster.buildings | std::views::transform([](const grid_building* b){
						return b->get_meta_info().hit_point.max;
					}), 0.f, std::plus{}) * cluster.size();
				}

				return static_cast<float>(static_cast<double>(sum) / static_cast<double>(std::max<std::size_t>(max_count + clusters_.size() - 1, 1)));
			}
		};
	}
}