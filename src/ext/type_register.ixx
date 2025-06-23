export module mo_yanxi.type_register;

import std;

namespace mo_yanxi{

	export
	struct type_identity{
	private:
		using Fn = std::type_index() noexcept;
		Fn* fn{};

	public:
		template <typename T>
		[[nodiscard]] consteval explicit(false) type_identity(std::in_place_type_t<T>) : fn{+[] static noexcept {
			return std::type_index(typeid(T));
		}}{

		}

		[[nodiscard]] std::type_index type_idx() const noexcept{
			return fn();
		}
	};

	template <typename T>
	static inline constexpr type_identity idt{std::in_place_type<T>};

	export
	template <typename T>
	[[nodiscard]] constexpr const type_identity* unstable_type_identity_of() noexcept{
		return std::addressof(idt<T>);
	}
}
