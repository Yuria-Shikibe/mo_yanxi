module;

#include "plf_hive.h"
#include <cassert>

export module mo_yanxi.game.ui.grid_editor:sub_panels;

import std;
import mo_yanxi.math.vector2;
import mo_yanxi.ui.elem.list;
import mo_yanxi.game.ecs.component.chamber;
import mo_yanxi.open_addr_hash_map;

namespace mo_yanxi::game::meta::chamber{
	export
	struct path{
		std::vector<math::upoint2> seq{};
	};

	struct path_node {
		math::point2 pos;
		float cost; // 从起点到当前节点的实际代价
		float cost_estimate; // 启发式估计（到终点的代价）
		path_node* parent;

		bool is_optimal;

		constexpr float cost_sum() const noexcept{
			return cost + cost_estimate;
		}

		auto operator<=>(const path_node& o) const noexcept{
			return cost_sum() <=> o.cost_sum();
		}
	};

	export
	struct path_finder;

	enum struct path_find_request_state{
		uninitialized,
		running,
		blocked_obstacle,
		finished,
		failed,
	};

	struct fuck_msvc_hasher{
		static std::size_t operator()(math::point2 vec) noexcept{
			return vec.hash_value();
		}
	};

	export
	struct ideal_path_finder_request{
		friend path_finder;
	private:
		struct pri_queue : public std::priority_queue<path_node*, std::vector<path_node*>, std::ranges::greater>{
			using priority_queue::priority_queue;
			using priority_queue::c;
		};

		const grid* target_grid{};
		math::point2 initial{};
		math::point2 dest{};

		unsigned iteration{};
		path_find_request_state state{};
		std::atomic_bool is_collapsed{};

		pri_queue open_list{};
		fixed_open_hash_map<math::point2, path_node, math::vectors::constant2<int>::max_vec2> all_nodes{};

		path ideal_path{};


		void update(path_finder& finder);

	private:
		[[nodiscard]] int heuristic(const math::point2 p) const noexcept{
			return math::dst_safe<int>(p.x, dest.x) + math::dst_safe<int>(p.y, dest.y);
		}

		void set_collapsed(path_finder& finder) noexcept;

		void reset(const grid& grid, const math::upoint2 src, const math::upoint2 dst){
			this->target_grid = &grid;
			this->initial = src.as<int>();
			this->dest = dst.as<int>();

			iteration = 0;
			state = path_find_request_state::running;
			all_nodes.clear();
			open_list.c.clear();
			ideal_path.seq.clear();

			auto& initial_node = all_nodes[src.as<int>()] = path_node{
				this->initial, 0, static_cast<float>(heuristic(this->initial))};
			open_list.push(&initial_node);

			is_collapsed.store(false, std::memory_order_release);
		}

	public:
		[[nodiscard]] ideal_path_finder_request() = default;

		[[nodiscard]] ideal_path_finder_request(const grid& target_grid, const math::upoint2 initial,
		                                  const math::upoint2 dest)
			: target_grid(&target_grid),
			  initial(initial),
			  dest(dest),
			  state(path_find_request_state::running){

			auto& initial_node = all_nodes[initial.as<int>()] = path_node{
				this->initial, 0, static_cast<float>(heuristic(this->initial))};
			open_list.push(&initial_node);
		}

		[[nodiscard]] bool try_wait_done() const noexcept{
			return is_collapsed.load(std::memory_order_acquire) == true;
		}

		void wait_done() const noexcept{
			is_collapsed.wait(false, std::memory_order_acquire);
		}

		[[nodiscard]] std::span<const math::upoint2> get_path() const noexcept{
			wait_done();
			return ideal_path.seq;
		}
	};

	struct semaphore_acq_guard{
		std::binary_semaphore& semaphore;

		[[nodiscard]] explicit(false) semaphore_acq_guard(std::binary_semaphore& semaphore) noexcept
			: semaphore(semaphore){
			semaphore.acquire();
		}

		~semaphore_acq_guard(){
			semaphore.release();
		}
	};

	export
	struct path_finder{
		friend ideal_path_finder_request;

	private:
		std::binary_semaphore requests_lock{1};
		plf::hive<ideal_path_finder_request> requests_{};

		std::binary_semaphore pending_lock{1};
		std::vector<ideal_path_finder_request*> pendings{};

		std::jthread server_thread_{};

		void process() noexcept{
			pending_lock.acquire();
			try{
				for (auto request : pendings){
					request->update(*this);
				}
			}catch(...){}
			pending_lock.release();
		}

		static void work(std::stop_token token, path_finder& self) noexcept{
			while(!token.stop_requested()){
				self.process();
			}
		}

		void try_init_thread(){
			if(!server_thread_.joinable()){
				server_thread_ = std::jthread(work, std::ref(*this));
			}
		}

	public:
		[[nodiscard]] path_finder() = default;

		ideal_path_finder_request* acquire_path(const grid& target_grid, math::upoint2 initial, math::upoint2 dest);
		bool reacquire_path(ideal_path_finder_request& last_request, const grid& target_grid, math::upoint2 initial,
		                    math::upoint2 dest);

		bool release_request(ideal_path_finder_request& request) noexcept {
			if(!request.is_collapsed.load(std::memory_order_acquire)){
				semaphore_acq_guard acq_guard{requests_lock};
				std::erase(pendings, &request);
			}

			semaphore_acq_guard guard{requests_lock};
			if(const auto itr = requests_.get_iterator(std::addressof(request)); itr != requests_.end()){
				requests_.erase(itr);
				return true;
			}
			return false;
		}

		void clear() noexcept{
			{
				semaphore_acq_guard acq_guard{requests_lock};
				pendings.clear();
			}

			{
				semaphore_acq_guard acq_guard{requests_lock};
				requests_.clear();
			}
		}

		path_finder(const path_finder& other) = delete;
		path_finder(path_finder&& other) noexcept = delete;
		path_finder& operator=(const path_finder& other) = delete;
		path_finder& operator=(path_finder&& other) noexcept = delete;
	};

	struct tile_state{
		unsigned captures{};
	};

	export
	struct grid_obstacle_state{
	private:
		const grid* grid{};
		math::urect corridor_region{};
		std::vector<tile_state> states_{};

		auto&& tile_at(this auto&& self, math::upoint2 coord) noexcept{
			assert(self.corridor_region.contains(coord));
			auto off = coord - self.corridor_region.src;
			return self.states_[off.y * self.corridor_region.width() + off.x];
		}

	public:
		[[nodiscard]] explicit(false) grid_obstacle_state(const chamber::grid& grid, bool region_shrink_to_fit)
			: grid(std::addressof(grid)),
			  corridor_region(region_shrink_to_fit ? get_grid_corridor_region(grid) : math::urect{grid.get_extent()}), states_(corridor_region.area()){

		}

		void move(math::upoint2 src, math::upoint2 dst) noexcept{
			auto& st = tile_at(src);
			auto& dt = tile_at(dst);
			assert(st.captures != 0);
			--st.captures;
			++dt.captures;
		}

		void insert(math::upoint2 src) noexcept{
			++tile_at(src).captures;
		}

		void erase(math::upoint2 dst) noexcept{
			auto& dt = tile_at(dst);
			assert(dt.captures != 0);
			--dt.captures;
		}

		const tile_state& operator[](math::upoint2 indexed_corridor_pos) noexcept{
			return tile_at(indexed_corridor_pos);
		}
	};

}

namespace mo_yanxi::game::ui{
	using namespace mo_yanxi::ui;
	export struct grid_detail_pane;
	export struct grid_editor_viewport;


	template <typename T, std::size_t depth = 1>
		requires (depth > 0)
	struct nested_panel_base{
	protected:
		T& get_main_panel(this const auto& self) noexcept{
			auto p = static_cast<const elem&>(self).get_parent();
			for(auto i = 1; i < depth; ++i){
				p = p->get_parent();
			}

#if DEBUG_CHECK
			return dynamic_cast<T&>(*p);
#else
			return static_cast<T&>(*p);
#endif
		}
	};


	enum struct refresh_channel{
		all,
		indirect,
	};

	struct grid_editor_panel_base{
	protected:
		friend grid_detail_pane;
		~grid_editor_panel_base() = default;

		[[nodiscard]] grid_editor_viewport& get_viewport(this const auto& self) noexcept{
			return get(static_cast<const elem&>(self));
		}

		virtual void refresh(refresh_channel channel){

		}

	private:
		static grid_editor_viewport& get(const elem& self) noexcept;
	};

	struct grid_structural_panel : ui::list, grid_editor_panel_base{
	private:
		meta::chamber::grid_structural_statistic statistic{};

	public:
		struct entry : ui::table, nested_panel_base<grid_structural_panel, 3>{
			const meta::chamber::grid_building* identity{};

			[[nodiscard]] entry(scene* scene, group* group, const meta::chamber::grid_building* identity)
				: table(scene, group),
				  identity(identity){
				interactivity = interactivity::enabled;
				build();
			}

			void on_focus_changed(bool is_focused) override;

		private:
			void build();
		};


		[[nodiscard]] grid_structural_panel(scene* scene, group* parent)
			: list(scene, parent){
			build();
		}

		void refresh(refresh_channel channel) override;

	private:
		list& get_entries_list() const noexcept;
		void build();
	};

	struct power_state_panel : ui::list, grid_editor_panel_base{

	private:
		struct bar_drawer : ui::elem, nested_panel_base<power_state_panel>{
			using elem::elem;

			void draw_content(const rect clipSpace) const override;

		};

	public:
		[[nodiscard]] power_state_panel(scene* scene, group* parent)
			: list(scene, parent){
			build();
		}

		void refresh(refresh_channel channel) override;

	private:
		void build();

	};

	struct maneuvering_panel : ui::list, grid_editor_panel_base{
	private:
		ecs::chamber::maneuver_subsystem subsystem{};

	public:
		[[nodiscard]] maneuvering_panel(scene* scene, group* parent)
			: list(scene, parent){
			build();
		}

		void refresh(refresh_channel channel) override;

	private:

	private:
		void build();
	};


}