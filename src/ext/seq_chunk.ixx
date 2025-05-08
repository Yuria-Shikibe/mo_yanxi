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
	struct seq_chunk final : private std::conditional_t<std::is_fundamental_v<T>, raw_wrap<T>, T>...{

		template <typename Ty>
			requires (contained_within<std::remove_cvref_t<Ty>, T...>)
		static copy_qualifier_t<Ty, seq_chunk>* cast_from(Ty* p) noexcept{
			return static_cast<copy_qualifier_t<Ty, seq_chunk>*>(p);
		}

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


	export
	template <typename Ty, spec_of<seq_chunk> Chunk>
	constexpr copy_qualifier_t<Ty, Chunk>* seq_chunk_cast(Ty* p) noexcept{
		return Chunk::cast_from(p);
	}

	export
	template <typename Tgt, spec_of<seq_chunk> ChunkTy, typename ChunkPartial>
		requires requires{
		requires contained_in<Tgt, typename ChunkTy::tuple_type>;
		requires contained_in<std::remove_cvref_t<ChunkPartial>, typename ChunkTy::tuple_type>;
		}
	constexpr [[nodiscard]] decltype(auto) neighbour_of(ChunkPartial& value) noexcept{
		using CTy = copy_qualifier_t<ChunkPartial, std::remove_cvref_t<ChunkTy>>;
		CTy* chunk = CTy::cast_from(std::addressof(value));
		return chunk->template get<Tgt>();//std::forward_like<ChunkPartial>();
	}

	export
	template <typename Tgt, tuple_spec ChunkTy, typename ChunkPartial>
		requires requires{
		requires contained_in<Tgt, ChunkTy>;
		requires contained_in<std::remove_cvref_t<ChunkPartial>, ChunkTy>;
		}
	constexpr [[nodiscard]] decltype(auto) neighbour_of(ChunkPartial& value) noexcept{
		using CTy = copy_qualifier_t<ChunkPartial, tuple_to_seq_chunk_t<std::remove_cvref_t<ChunkTy>>>;
		CTy* chunk = CTy::cast_from(std::addressof(value));
		return chunk->template get<Tgt>();//std::forward_like<ChunkPartial>();
	}


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


	export
	template <typename T, mo_yanxi::decayed_spec_of<mo_yanxi::seq_chunk> Chunk>
	constexpr decltype(auto) get(Chunk&& chunk) noexcept{
		return chunk.template get<T>();
	}

	export
	template <std::size_t Idx, mo_yanxi::decayed_spec_of<mo_yanxi::seq_chunk> Chunk>
	constexpr decltype(auto) get(Chunk&& chunk) noexcept{
		return chunk.template get<Idx>();
	}
}


namespace std{
	template <typename ...T>
	struct tuple_size<mo_yanxi::seq_chunk<T...>> : std::integral_constant<std::size_t, sizeof...(T)>{};

	template <std::size_t Idx, typename ...T>
	struct tuple_element<Idx, mo_yanxi::seq_chunk<T...>> : std::tuple_element<Idx, std::tuple<T...>>{};
}
