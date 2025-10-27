module;

#include "adapted_attributes.hpp"

export module mo_yanxi.type_register;

import std;

namespace mo_yanxi{
	// [0x00007ff662428d08 {mo_yanxi.exe!??$idt@ Upower_generator_building_tag@ chamber@ ecs@ game@ mo_yanxi@ @ @ mo_yanxi@
	// 	@ 3Utype_identity_data@ 1@ B::<!mo_yanxi.type_register>} {fn=0x00007ff6616c970e {mo_yanxi.exe!` mo_yanxi::
	// 	type_identity_data::type_identity_data<mo_yanxi::game::ecs::chamber::power_generator_building_tag>(std::
	// 		in_place_type_t<mo_yanxi::game::ecs::chamber::power_generator_building_tag>) __ptr64'::`1'::<lambda_41_>::
	// 	operator()(void)}, ...}


	export
	template <typename T>
	consteval std::string_view name_of() noexcept {
		static constexpr auto loc = std::source_location::current();
		constexpr std::string_view sv{loc.function_name()};

#ifdef __clang__
		constexpr std::string_view prefix{"[T = "};
		auto pos = sv.find(prefix) + prefix.size();
		return sv.substr(pos, sv.size() - pos - 1);
#else
		constexpr auto pos = sv.find(__func__) + 1;

		constexpr auto remain = sv.substr(pos);
		constexpr auto initial_pos = remain.find('<') + 1;

		constexpr auto i = [](std::size_t initial, std::string_view str) static consteval {
			std::size_t count = 1;
			for(; initial < str.size(); ++initial){
				if(str[initial] == '<')count++;
				if(str[initial] == '>')count--;
				if(!count)break;
			}

			return initial;
		}(initial_pos, remain);

		constexpr std::size_t size = i - initial_pos;
		return remain.substr(initial_pos, size);
#endif
	}
	export
	struct type_identity_data;

	export using type_identity_index = const type_identity_data*;

	template <typename T>
	[[nodiscard]] constexpr type_identity_index unstable_type_identity_of_impl() noexcept;

	export
	struct type_identity_data{
	private:
		using Fn = std::type_index() noexcept;
		std::string_view name_;
		Fn* fn{};

	public:
		template <typename T>
		[[nodiscard]] constexpr explicit(false) type_identity_data(std::in_place_type_t<T>) : name_(mo_yanxi::name_of<T>()), fn{+[] static noexcept {
			return std::type_index(typeid(T));
		}}{

		}

		[[nodiscard]] constexpr std::type_index type_idx() const noexcept{
			return fn();
		}
		template <typename T = std::ptrdiff_t>
			requires (sizeof(T) >= sizeof(std::ptrdiff_t))
		[[nodiscard]] constexpr T to() const noexcept{
			return std::bit_cast<T>(this - unstable_type_identity_of_impl<void>());
		}

		[[nodiscard]] constexpr std::string_view name() const noexcept{
			return name_;
		}
	};

	template <typename T>
	inline constexpr type_identity_data idt{std::in_place_type<T>};

	template <typename T>
	[[nodiscard]] constexpr type_identity_index unstable_type_identity_of_impl() noexcept{
		return std::addressof(idt<T>);
	}

	export
	template <typename T>
	[[nodiscard]] constexpr type_identity_index unstable_type_identity_of() noexcept{
		return unstable_type_identity_of_impl<std::remove_cvref_t<T>>();
	}


	// // Constexpr pointer hash for 64-bit systems
	// export struct type_identity_constexpr_hasher {
	// 	// Main hash function - uses Fibonacci hashing for good distribution
	// 	FORCE_INLINE static constexpr std::size_t operator()(type_identity_index ptr) noexcept {
	//
	// 		// Cast pointer to integer
	// 		auto value = reinterpret_cast<std::uintptr_t>(ptr);
	//
	// 		// Fibonacci hashing (golden ratio reciprocal)
	// 		// 64-bit magic number from Knuth's multiplicative method
	// 		constexpr uintptr_t multiplier = 11400714819323198485ull;
	//
	// 		// Shift right to mix higher bits into lower bits
	// 		// This is particularly important for pointers with alignment
	// 		value ^= value >> 33;
	//
	// 		// Multiply by magic constant
	// 		value *= multiplier;
	//
	// 		// Final mix
	// 		value ^= value >> 33;
	// 		value *= multiplier;
	// 		value ^= value >> 33;
	//
	// 		// Return as size_t
	// 		return static_cast<std::size_t>(value);
	// 	}
	//
	// 	// Optional: enable transparent comparison
	// 	using is_transparent = void;
	// };
}

