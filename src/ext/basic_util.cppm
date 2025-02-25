module;

#include <cassert>

export module ext.basic_util;

import std;

namespace mo_yanxi{
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
		if constexpr (sizeof(T) > sizeof(void*)){
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
