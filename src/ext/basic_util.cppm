module;

#include <cassert>

export module mo_yanxi.basic_util;

import std;

namespace mo_yanxi{
	export
	template <typename T, std::predicate<T&> Pred>
	constexpr void modifiable_erase_if(std::vector<T>& vec, Pred pred) noexcept(
		std::is_nothrow_invocable_v<Pred, T&> && std::is_nothrow_move_assignable_v<T>
		) {
		auto write_pos = vec.begin();
		auto read_pos = vec.begin();
		const auto end = vec.end();
		while(read_pos != end){
			if (!std::invoke(pred, *read_pos)) {
				if (write_pos != read_pos) {
					*write_pos = std::ranges::iter_move(read_pos);
				}
				++write_pos;
			}
			++read_pos;
		}
		vec.erase(write_pos, end);
	}

	export
	template <class Rep, typename Period>
	[[nodiscard]] auto to_absolute_time(const std::chrono::duration<Rep, Period>& rel_time) noexcept {
		constexpr auto zero                 = std::chrono::duration<Rep, Period>::zero();
		const auto cur                      = std::chrono::steady_clock::now();
		decltype(cur + rel_time) abs_time = cur; // return common type
		if (rel_time > zero) {
			constexpr auto forever = (std::chrono::steady_clock::time_point::max)();
			if (abs_time < forever - rel_time) {
				abs_time += rel_time;
			} else {
				abs_time = forever;
			}
		}
		return abs_time;
	}

	export
	template <typename T>
	constexpr auto pass_fn(T& fn) noexcept{
		if constexpr (sizeof(T) > sizeof(void*) * 2){
			return std::ref(fn);
		}else{
			return fn;
		}
	}

	export
	struct dereference{
		template <std::indirectly_readable T>
		constexpr decltype(auto) operator()(T&& val) const noexcept{
			if constexpr (std::is_pointer_v<std::decay_t<T>>){
				assert(val != nullptr);
			}else if constexpr (std::equality_comparable_with<T, std::nullptr_t>){
				assert(val != nullptr);
			}

			return *(std::forward<T>(val));
		}
	};

	namespace ranges::views{
		export constexpr auto deref = std::views::transform(dereference{});
	}
}
