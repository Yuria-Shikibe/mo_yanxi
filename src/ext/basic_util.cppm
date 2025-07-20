module;

#include <cassert>
#include "adapted_attributes.hpp"

export module mo_yanxi.basic_util;

import std;

namespace mo_yanxi{


	template <class Ty>
	concept Boolean_testable_impl = std::convertible_to<Ty, bool>;

	export
	template <class Ty>
	concept boolean_testable = Boolean_testable_impl<Ty> && requires(Ty&& t) {
		{ !static_cast<Ty&&>(t) } -> Boolean_testable_impl;
	};

	export
	template <typename T>
		requires requires(T& t){
			{ t.try_lock() } noexcept -> boolean_testable;
			{ t.unlock() } noexcept;
		}
	FORCE_INLINE constexpr void assert_unlocked(T& mutex) noexcept{
#if DEBUG_CHECK
		if(mutex.try_lock()){
			mutex.unlock();
			return;
		}
		std::println(std::cerr, "Unlocked mutex assertion failed");
		std::terminate();
#endif
	}

	export
	template <typename T>
		requires requires(T& t){
			{ t.try_lock() } noexcept -> boolean_testable;
		}
	FORCE_INLINE constexpr void assert_locked(T& mutex) noexcept{
#if DEBUG_CHECK
		if(mutex.try_lock()){
			std::println(std::cerr, "Unlocked mutex assertion failed");
			std::terminate();
		}
#endif
	}

	export
	template<typename... Fs>
	struct overload : private Fs... {
		template <typename ...Args>
		constexpr explicit(false) overload(Args&&... fs) : Fs{std::forward<Args>(fs)}... {}

		using Fs::operator()...;
	};

	export
	template<typename... Fs>
	overload(Fs&&...) -> overload<std::decay_t<Fs>...>;

	export
	template<typename Ret, typename... Fs>
		requires (std::is_void_v<Ret> || std::is_default_constructible_v<Ret>)
	struct overload_def_noop : private Fs... {
		template <typename V, typename ...Args>
		constexpr explicit(false) overload_def_noop(std::in_place_type_t<V>, Args&&... fs) : Fs{std::forward<Args>(fs)}... {}

		using Fs::operator()...;

		template <typename ...T>
			requires (!std::invocable<Fs, T...> && ...)
		static Ret operator()(T&& ...) noexcept{
			if constexpr (std::is_void_v<Ret>){
				return ;
			}else{
				return Ret{};
			}
		}
	};

	export
	template<typename V, typename... Fs>
	overload_def_noop(std::in_place_type_t<V>, Fs&&...) -> overload_def_noop<V, std::decay_t<Fs>...>;


	export
	template <typename T, std::predicate<std::ranges::range_reference_t<T>> Pred>
	constexpr void modifiable_erase_if(T& vec, Pred pred) noexcept(
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
		if constexpr (sizeof(T) > sizeof(void*) * 2 || !std::constructible_from<T, const T&>){
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
