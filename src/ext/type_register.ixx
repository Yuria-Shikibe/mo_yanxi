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

	template <typename T>
	consteval std::string_view name() noexcept{
		return __func__;
	}

	export
	struct type_identity_data{
	private:
		using Fn = std::type_index() noexcept;
		std::string_view name;
		Fn* fn{};

	public:
		template <typename T>
		[[nodiscard]] constexpr explicit(false) type_identity_data(std::in_place_type_t<T>) : name(mo_yanxi::name<T>()), fn{+[] static noexcept {
			return std::type_index(typeid(T));
		}}{

		}

		[[nodiscard]] std::type_index type_idx() const noexcept{
			return fn();
		}
	};

	template <typename T>
	inline constexpr type_identity_data idt{std::in_place_type<T>};

	export using type_identity_index = const type_identity_data*;


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

