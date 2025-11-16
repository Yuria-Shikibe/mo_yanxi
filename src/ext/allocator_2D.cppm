module;

#include <cassert>
#include "adapted_attributes.hpp"

export module mo_yanxi.allocator_2D;

import mo_yanxi.math.vector2;
import mo_yanxi.math.rect_ortho;
import std;

namespace mo_yanxi{

export
template <typename Alloc = std::allocator<std::byte>>
struct allocator2d{
protected:
	using T = std::uint32_t;

public:
	using size_type = T;
	using extent_type = math::vector2<T>;
	using point_type = math::vector2<T>;
	using rect_type = math::rect_ortho<T>;
	using sub_region_type = std::array<rect_type, 3>;
	using allocator_type = Alloc;

private:
	extent_type extent_{};
	//Using size_t instead?
	size_type remain_area_{};

	struct split_point;
	using map_type =
	std::unordered_map<
		point_type, split_point,
		std::hash<point_type>, std::equal_to<point_type>,
		typename std::allocator_traits<allocator_type>::template rebind_alloc<std::pair<const point_type, split_point>>
	>;

	map_type map{};

	using inner_tree_type = std::multimap<
		size_type, point_type,
		std::less<size_type>,
		typename std::allocator_traits<allocator_type>::template rebind_alloc<std::pair<const size_type, point_type>>
	>;

	using tree_type = std::map<
		size_type, inner_tree_type,
		std::less<size_type>,
		std::scoped_allocator_adaptor<typename std::allocator_traits<allocator_type>::template rebind_alloc<std::pair<
			const size_type, inner_tree_type>>>
	>;

	tree_type nodes_XY{};
	tree_type nodes_YX{};

	using ItrOuter = tree_type::iterator;
	using ItrInner = tree_type::mapped_type::iterator;

	struct ItrPair{
		ItrOuter outer{};
		ItrInner inner{};

		[[nodiscard]] auto value() const{
			return inner->second;
		}

		/**
		 * @return [rst, possible to find]
		 */
		void locateNextInner(const size_type second){
			inner = outer->second.lower_bound(second);
		}

		bool locateNextOuter(tree_type& tree){
			++outer;
			if(outer == tree.end()) return false;
			return true;
		}

		[[nodiscard]] bool valid(const tree_type& tree) const noexcept{
			return outer != tree.end() && inner != outer->second.end();
		}
	};

	struct split_point{
		point_type parent{};

		point_type bot_lft{};
		point_type top_rit{};

		point_type split{top_rit};

		bool idle{true};
		bool idle_top_lft{true};
		bool idle_top_rit{true};
		bool idle_bot_rit{true};


		[[nodiscard]] split_point() = default;

		[[nodiscard]] split_point(
			const point_type parent,
			const point_type bot_lft,
			const point_type top_rit)
		: parent(parent),
		bot_lft(bot_lft), top_rit(top_rit){
		}

		[[nodiscard]] bool is_leaf() const noexcept{
			return split == top_rit;
		}

		[[nodiscard]] bool is_root() const noexcept{
			return parent == bot_lft;
		}

		[[nodiscard]] bool is_split_idle() const noexcept{
			return idle_top_lft && idle_top_rit && idle_bot_rit;
		}

		void mark_captured(map_type& map) noexcept{
			idle = false;

			if(is_root()) return;
			auto& p = get_parent(map);
			if(parent.x == bot_lft.x){
				p.idle_top_lft = false;
			} else if(parent.y == bot_lft.y){
				p.idle_bot_rit = false;
			} else{
				p.idle_top_rit = false;
			}
		}

		bool check_merge(allocator2d& alloc) noexcept{
			if(idle && is_split_idle()){
				//resume split
				if(region_top_lft().area()) alloc.erase_split(src_top_lft(), {split.x, top_rit.y});
				if(region_top_rit().area()) alloc.erase_split(src_top_rit(), top_rit);
				if(region_bot_rit().area()) alloc.erase_split(src_bot_rit(), {top_rit.x, split.y});
				alloc.erase_mark(bot_lft, split);

				split = top_rit;

				if(is_root()) return false;

				auto& p = get_parent(alloc.map);
				if(parent.x == bot_lft.x){
					p.idle_top_lft = true;
				} else if(parent.y == bot_lft.y){
					p.idle_bot_rit = true;
				} else{
					p.idle_top_rit = true;
				}

				return true;
			}

			return false;
		}

		[[nodiscard]] extent_type get_extent() const noexcept{
			return split - bot_lft;
		}

		split_point& get_parent(map_type& map) const noexcept{
			return map.at(parent);
		}

		[[nodiscard]] constexpr point_type src_top_lft() const noexcept{
			return {bot_lft.x, split.y};
		}

		[[nodiscard]] constexpr point_type src_bot_rit() const noexcept{
			return {split.x, bot_lft.y};
		}

		[[nodiscard]] constexpr point_type src_top_rit() const noexcept{
			return split;
		}

		[[nodiscard]] constexpr rect_type region_bot_lft() const noexcept{
			return {tags::unchecked, bot_lft, split};
		}

		[[nodiscard]] constexpr rect_type region_bot_rit() const noexcept{
			return {tags::unchecked, {split.x, bot_lft.y}, {top_rit.x, split.y}};
		}

		[[nodiscard]] constexpr rect_type region_top_rit() const noexcept{
			return {tags::unchecked, split, top_rit};
		}

		[[nodiscard]] constexpr rect_type region_top_lft() const noexcept{
			return {tags::unchecked, {bot_lft.x, split.y}, {split.x, top_rit.y}};
		}

		void acquire_and_split(allocator2d& alloc, const math::usize2 extent){
			assert(idle);

			if(is_leaf()){
				// first split
				split = bot_lft + extent;
				alloc.erase_mark(bot_lft, top_rit);

				if(const auto b_r = region_bot_rit(); b_r.area()){
					alloc.add_split(bot_lft, b_r.get_src(), b_r.get_end());
				}

				if(const auto t_r = region_top_rit(); t_r.area()){
					alloc.add_split(bot_lft, t_r.get_src(), t_r.get_end());
				}

				if(const auto t_l = region_top_lft(); t_l.area()){
					alloc.add_split(bot_lft, t_l.get_src(), t_l.get_end());
				}

				mark_captured(alloc.map);
			} else{
				alloc.erase_mark(bot_lft, split);
			}
		}

		split_point* mark_idle(allocator2d& alloc){
			CHECKED_ASSUME(!idle);
			idle = true;

			auto* p = this;
			while(p->check_merge(alloc)){
				auto* next = &p->get_parent(alloc.map);
				p = next;
			}

			if(p->is_leaf()) alloc.mark_size(p->bot_lft, p->top_rit);
			return p;
		}
	};

	std::optional<point_type> getValidNode(const extent_type size){
		assert(size.area() > 0);

		if(remain_area_ < size.area()){
			return std::nullopt;
		}

		ItrPair itrPairXY{nodes_XY.lower_bound(size.x)};
		ItrPair itrPairYX{nodes_YX.lower_bound(size.y)};

		std::optional<point_type> node{};

		bool possibleX{itrPairXY.outer != nodes_XY.end()};
		bool possibleY{itrPairYX.outer != nodes_YX.end()};

		while(true){
			if(!possibleX && !possibleY) break;

			if(possibleX){
				itrPairXY.locateNextInner(size.y);
				if(itrPairXY.valid(nodes_XY)){
					node = itrPairXY.value();
					break;
				} else{
					possibleX = itrPairXY.locateNextOuter(nodes_XY);
				}
			}

			if(possibleY){
				itrPairYX.locateNextInner(size.x);
				if(itrPairYX.valid(nodes_YX)){
					node = itrPairYX.value();
					break;
				} else{
					possibleY = itrPairYX.locateNextOuter(nodes_YX);
				}
			}
		}

		return node;
	}

	void mark_size(const point_type src, const point_type dst) noexcept{
		const auto size = dst - src;

		nodes_XY[size.x].insert({size.y, src});
		nodes_YX[size.y].insert({size.x, src});
	}

	void add_split(const point_type parent, const point_type src, const point_type dst){
		map.insert_or_assign(src, split_point{parent, src, dst});
		mark_size(src, dst);
	}

	void erase_split(const point_type src, const point_type dst){
		map.erase(src);
		erase_mark(src, dst);
	}

	void erase_mark(const point_type src, const point_type dst){
		const auto size = dst - src;

		erase(nodes_XY, src, size.x, size.y);
		erase(nodes_YX, src, size.y, size.x);
	}

	static void erase(tree_type& map, const point_type src, const size_type outerKey,
		const size_type innerKey) noexcept{
		auto& inner = map[outerKey];
		auto [begin, end] = inner.equal_range(innerKey); //->second.erase(inner);
		for(auto cur = begin; cur != end; ++cur){
			if(cur->second == src){
				inner.erase(cur);
				return;
			}
		}
	}

public:
	[[nodiscard]] allocator2d() = default;

	[[nodiscard]] explicit allocator2d(const allocator_type& allocator) : map(allocator), nodes_XY(allocator),
	nodes_YX(allocator){
	}

	[[nodiscard]] explicit allocator2d(const extent_type extent) :
	extent_(extent), remain_area_(extent.area()){
		add_split({}, {}, extent);
	}

	[[nodiscard]] allocator2d(const allocator_type& allocator, const extent_type extent) :
	extent_(extent), remain_area_(extent.area()), map(allocator),
	nodes_XY(allocator), nodes_YX(allocator){
		add_split({}, {}, extent);
	}


	std::optional<point_type> allocate(const math::usize2 extent){
		if(extent.area() == 0){
			std::println(std::cerr, "try allocate region with zero area");
			std::terminate();
		}

		if(extent.beyond(extent_)) return std::nullopt;
		if(remain_area_ < extent.as<std::uint64_t>().area()) return std::nullopt;

		auto chamber_src = getValidNode(extent);

		if(!chamber_src) return std::nullopt;

		auto& chamber = map[chamber_src.value()];
		chamber.acquire_and_split(*this, extent);
		remain_area_ -= extent.area();
		return chamber_src.value();
	}

	void deallocate(const point_type value) noexcept{
		if(const auto itr = map.find(value); itr != map.end()){
			const auto extent = (itr->second.split - value).area();
			itr->second.mark_idle(*this);
			remain_area_ += extent;
		} else{
			std::println(std::cerr, "try deallocate region not belong to the allocator");
			std::terminate();
		}
	}

	[[nodiscard]] extent_type extent() const noexcept{
		return extent_;
	}

	[[nodiscard]] size_type remain_area() const noexcept{
		return remain_area_;
	}
};

}
