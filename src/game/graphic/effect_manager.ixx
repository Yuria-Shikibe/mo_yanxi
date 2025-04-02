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
		pool_type pool{};
		mutable std::mutex mtx{};

	public:
		[[nodiscard]] effect_manager() = default;

		[[nodiscard]] effect_manager(
			const std::size_t min_chunk_size,
			const std::size_t max_chunk_size,
			const std::size_t reserve = 1024
		) : pool(plf::hive_limits{min_chunk_size, max_chunk_size}){
			pool.reserve(reserve);
		}

		[[nodiscard]] explicit effect_manager(
			const std::size_t reserve
		){
			pool.reserve(reserve);
		}

		effect_manager(const effect_manager& other) = delete;

		effect_manager(effect_manager&& other) noexcept{
			std::scoped_lock guard(mtx, other.mtx);
			pool = std::move(other.pool);
		}

		effect_manager& operator=(const effect_manager& other) = delete;

		effect_manager& operator=(effect_manager&& other) noexcept{
			if(this == &other) return *this;
			std::scoped_lock guard(mtx, other.mtx);
			pool = std::move(other.pool);
			return *this;
		}

		[[nodiscard]] effect& obtain(){
			effect* efx;

			{
				auto id = effect::acquire_id();
				std::lock_guard guard(mtx);
				efx = std::to_address(pool.emplace(id));
			}

			return *efx;
		}

		pool_type::size_type update(float delta_in_tick) noexcept {
			pool_type::size_type count = 0;

			constexpr static auto updator = [](effect& effect, float delta) static noexcept {
				return effect.update(delta);
			};

			{
				std::lock_guard guard(mtx);
				const auto end = pool.end();
				for(auto current = pool.begin(); current != end; ++current){
					if(updator(*current, delta_in_tick)){
						const pool_type::size_type original_count = ++count;
						auto last = current;

						while(++last != end && updator(*last, delta_in_tick)){
							++count;
						}

						if(count != original_count){
							current = pool.erase(current, last); // optimised range-erase
						} else{
							current = pool.erase(current);
						}

						if(last == end) break;
					}
				}
			}


			return count;
		}

		template <std::invocable<pool_type::reference> Fn>
		void each(Fn fn) noexcept(std::is_nothrow_invocable_v<Fn, pool_type::reference>){
			std::lock_guard guard(mtx);
			for (auto && effect : pool){
				std::invoke(fn, effect);
			}
		}
		template <std::invocable<pool_type::const_reference> Fn>
		void each(Fn fn) const noexcept(std::is_nothrow_invocable_v<Fn, pool_type::const_reference>) {
			std::lock_guard guard(mtx);
			for (auto && effect : pool){
				std::invoke(fn, effect);
			}
		}
	};
}