module;

#include <plf_hive.h>

export module mo_yanxi.game.graphic.effect.manager;


export import mo_yanxi.game.graphic.effect;
import std;

namespace mo_yanxi::game::fx{
	export
	struct effect_manager{
		using pool_type = plf::hive<effect>;
	private:
		pool_type staging_pool{};
		pool_type active_pool{};
		mutable std::mutex mtx{};

	public:
		[[nodiscard]] effect_manager() = default;

		[[nodiscard]] effect_manager(
			const std::size_t min_chunk_size,
			const std::size_t max_chunk_size,
			const std::size_t reserve = 1024
		) : active_pool(plf::hive_limits{min_chunk_size, max_chunk_size}){
			active_pool.reserve(reserve);
		}

		[[nodiscard]] explicit effect_manager(
			const std::size_t reserve
		){
			active_pool.reserve(reserve);
		}

		effect_manager(const effect_manager& other) = delete;

		effect_manager(effect_manager&& other) noexcept{
			std::scoped_lock guard(mtx, other.mtx);
			staging_pool = std::move(other.staging_pool);
			active_pool = std::move(other.active_pool);
		}

		effect_manager& operator=(const effect_manager& other) = delete;

		effect_manager& operator=(effect_manager&& other) noexcept{
			if(this == &other) return *this;
			std::scoped_lock guard(mtx, other.mtx);
			staging_pool = std::move(other.staging_pool);
			active_pool = std::move(other.active_pool);
			return *this;
		}

		[[nodiscard]] effect& obtain(){
			effect* efx;

			{
				auto id = effect::acquire_id();
				std::lock_guard guard(mtx);
				efx = std::to_address(staging_pool.emplace(id));
			}

			return *efx;
		}

		pool_type::size_type update(float delta_in_tick) noexcept {
			if(!staging_pool.empty()){
				std::scoped_lock guard(mtx);

				if(staging_pool.size() < 8){
					pool_type::size_type count = 0;

					const auto end = staging_pool.end();
					for(auto current = staging_pool.begin(); current != end; ++current){
						if(!current->is_referenced()){
							const pool_type::size_type original_count = ++count;
							auto last = current;

							while(++last != end && !last->is_referenced()){
								++count;
							}

							if(count != original_count){
								active_pool.insert_range(std::ranges::subrange{current, last} | std::views::as_rvalue);
								current = staging_pool.erase(current, last);
							} else{
								active_pool.insert(std::ranges::iter_move(current));
								current = staging_pool.erase(current);
							}

							if(last == end) break;
						}
					}
				}


				active_pool.splice(staging_pool);
			}

			pool_type::size_type count = 0;

			constexpr static auto updator = [](effect& effect, float delta) static noexcept {
				return effect.update(delta);
			};

			const auto end = active_pool.end();
			for(auto current = active_pool.begin(); current != end; ++current){
				if(updator(*current, delta_in_tick)){
					const pool_type::size_type original_count = ++count;
					auto last = current;

					while(++last != end && updator(*last, delta_in_tick)){
						++count;
					}

					if(count != original_count){
						current = active_pool.erase(current, last); // optimised range-erase
					} else{
						current = active_pool.erase(current);
					}

					if(last == end) break;
				}
			}

			if(count){
				active_pool.shrink_to_fit();
			}

			return count;
		}

		template <std::invocable<pool_type::reference> Fn>
		void each(Fn fn) noexcept(std::is_nothrow_invocable_v<Fn, pool_type::reference>){
			for (auto && effect : active_pool){
				std::invoke(fn, effect);
			}
		}
		template <std::invocable<pool_type::const_reference> Fn>
		void each(Fn fn) const noexcept(std::is_nothrow_invocable_v<Fn, pool_type::const_reference>) {
			for (auto && effect : active_pool){
				std::invoke(fn, effect);
			}
		}
	};
}