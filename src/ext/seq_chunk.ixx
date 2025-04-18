//
// Created by Matrix on 2025/4/16.
//

export module mo_yanxi.seq_chunk;

import mo_yanxi.meta_programming;
import mo_yanxi.concepts;
import std;

namespace mo_yanxi{
	template <typename T>
	struct raw_wrap{
		T val;

		constexpr explicit(false) operator const T&() const{
			return val;
		}

		constexpr explicit(false) operator T&(){
			return val;
		}

		friend constexpr bool operator==(const raw_wrap&, const raw_wrap&) noexcept = default;
	};

	template <typename T, typename Tuple>
	constexpr decltype(auto) try_get(Tuple&& tuple) noexcept {
		if constexpr (tuple_index_v<T, std::remove_cvref_t<Tuple>> == std::tuple_size_v<std::remove_cvref_t<Tuple>>){
			return T{};
		}else{
			return std::forward_like<Tuple>(std::get<T>(tuple));
		}
	}

	export
	template <typename... T>
	struct seq_chunk : private std::conditional_t<std::is_fundamental_v<T>, raw_wrap<T>, T>...{
		using tuple_type = std::tuple<T...>;

		[[nodiscard]] constexpr seq_chunk() noexcept = default;

		template <typename ...Args>
			requires (sizeof...(Args) == sizeof...(T))
		[[nodiscard]] constexpr explicit(false) seq_chunk(Args&& ...args)
		: std::conditional_t<std::is_fundamental_v<T>, raw_wrap<T>, T>{std::forward<Args>(args)} ...{

		}
		template <typename Tuple>
			requires (is_tuple_v<Tuple>)
		[[nodiscard]] constexpr explicit(false) seq_chunk(Tuple&& args)
		: std::conditional_t<std::is_fundamental_v<T>, raw_wrap<T>, T>{mo_yanxi::try_get<T>(args)} ...{

		}

		template <typename ...Args>
			requires (sizeof...(Args) < sizeof...(T))
		[[nodiscard]] constexpr explicit(false) seq_chunk(Args&& ...args)
		: seq_chunk{std::make_tuple(std::forward<Args>(args)...)}{

		}

		template <typename Ty, typename S>
		constexpr decltype(auto) get(this S&& self) noexcept{
			return std::forward_like<S>(
				static_cast<std::conditional_t<std::is_const_v<std::remove_reference_t<S>>, const Ty&, Ty&>>(self));
		}

		template <std::size_t Idx, typename S>
		constexpr decltype(auto) get(this S&& self) noexcept{
			return std::forward<S>(self).template get<std::tuple_element_t<Idx, tuple_type>>();
		}


		template <typename Ty, typename Chunk>
		friend constexpr decltype(auto) get(Chunk&& chunk) noexcept{
			return chunk.template get<Ty>();
		}

		template <std::size_t Idx, typename Chunk>
		friend constexpr decltype(auto) get(Chunk&& chunk) noexcept{
			return chunk.template get<Idx>();
		}
	};

	template <typename T>
	consteval auto ttsc(){
		return []<std::size_t ... I>(std::index_sequence<I...>){
			return std::type_identity<seq_chunk<std::tuple_element_t<I, T> ...>>{};
		}(std::make_index_sequence<std::tuple_size_v<T>>());
	}

	export
	template <typename Tuple>
	using tuple_to_seq_chunk_t = typename decltype(ttsc<Tuple>())::type;



	/*export
	template <std::size_t N, typename Chunk>
		requires (N < std::tuple_size_v<typename Chunk::tuple_type>)
	using seq_chunk_truncate_t = typename decltype([]{
		if constexpr(N == 0){
			std::type_identity<seq_chunk<>>{};
		} else{
			return []<std::size_t ... I>(std::index_sequence<I...>){
				return std::type_identity<seq_chunk<std::tuple_element_t<I, typename Chunk::tuple_type>...>>{};
			}(std::make_index_sequence<N>());
		}
	}())::type;*/

	// export
	// template <std::size_t Idx, typename Chunk>
	// inline constexpr std::size_t seq_chunk_offset_at = tuple_offset_at_v<std::tuple_size_v<typename Chunk::tuple_type> - 1 - Idx, reverse_tuple_t<typename Chunk::tuple_type>>;;
	//
	// export
	// template <typename V, typename Chunk>
	// inline constexpr std::size_t seq_chunk_offset_of = seq_chunk_offset_at<tuple_index_v<V, typename Chunk::tuple_type>, Chunk>;
}


export namespace std{
	template <mo_yanxi::spec_of<mo_yanxi::seq_chunk> T>
	struct tuple_size<T> : std::integral_constant<std::size_t, std::tuple_size_v<typename T::tuple_type>>{};

	template <std::size_t Idx, mo_yanxi::spec_of<mo_yanxi::seq_chunk> T>
	struct tuple_element<Idx, T> : std::type_identity<std::tuple_element_t<Idx, typename T::tuple_type>>{};

	template <typename T, mo_yanxi::decayed_spec_of<mo_yanxi::seq_chunk> Chunk>
	constexpr decltype(auto) get(Chunk&& chunk) noexcept{
		return chunk.template get<T>();
	}

	template <std::size_t Idx, mo_yanxi::decayed_spec_of<mo_yanxi::seq_chunk> Chunk>
	constexpr decltype(auto) get(Chunk&& chunk) noexcept{
		return chunk.template get<Idx>();
	}
}
