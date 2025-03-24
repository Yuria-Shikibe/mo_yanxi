export module mo_yanxi.type_map;

import std;
import mo_yanxi.meta_programming;

export namespace mo_yanxi{
	template <typename V, typename Ts>
		requires (std::is_default_constructible_v<V>)
	struct type_map : type_to_index<Ts>{
	private:
		using base = type_to_index<Ts>;

	public:
		using value_type = V;

		std::array<V, base::arg_size> items{};

		template <std::size_t I>
		[[nodiscard]] constexpr auto& at() const noexcept{
			static_assert(I < base::arg_size, "invalid index");

			return items[I];
		}

		template <std::size_t I>
		[[nodiscard]] constexpr auto& at() noexcept{
			static_assert(I < base::arg_size, "invalid index");

			return items[I];
		}

		template <typename Ty>
		[[nodiscard]] constexpr auto& at() const noexcept{
			return at<base::template index_of<Ty>>();
		}

		template <typename Ty>
		[[nodiscard]] constexpr auto& at() noexcept{
			return at<base::template index_of<Ty>>();
		}
	};

	template <typename V, typename ...Ts>
	using type_map_of = type_map<V, std::tuple<Ts...>>;
}
