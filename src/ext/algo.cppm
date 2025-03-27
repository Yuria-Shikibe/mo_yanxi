module;

#include <cassert>
// #include <type_traits>

export module ext.algo;
import mo_yanxi.concepts;
import std;


namespace mo_yanxi::algo{
	export
	template <
		std::input_iterator It,
		std::sentinel_for<It> Se,
		std::indirectly_unary_invocable<It> Proj = std::identity,
		typename Func = std::plus<>,
		typename T = std::indirect_result_t<Proj, It>>
		requires requires(Func func, T v, It it, Proj proj){
			{ std::invoke(func, std::move(v), std::invoke(proj, *it)) } -> std::convertible_to<T>;
		}
	constexpr T accumulate(It it, Se se, T initial = {}, Func func = {}, Proj proj = {}){
		for(; it != se; ++it){
			initial = std::invoke(func, std::move(initial), std::invoke(proj, *it));
		}

		return initial;
	}

	export
	template <
		typename Rng,
		typename Proj = std::identity, typename Func = std::plus<>,
		typename T = std::indirect_result_t<Proj, std::ranges::iterator_t<Rng>>>
	requires (!std::sentinel_for<T, Rng>)
	constexpr T accumulate(Rng&& rng, T initial, Func func = {}, Proj proj = {}){
		return mo_yanxi::algo::accumulate(std::ranges::begin(rng), std::ranges::end(rng), std::move(initial), std::move(func), std::move(proj));
	}

	export
	template <typename Range>
		requires mo_yanxi::range_of<Range>
	auto partBy(Range&& range, std::indirect_unary_predicate<std::ranges::iterator_t<Range>> auto pred){
		return std::make_pair(range | std::ranges::views::filter(pred),
			range | std::ranges::views::filter(std::not_fn(pred)));
	}

	template <typename Itr, typename Sentinel>
	constexpr bool itrSentinelCompariable = requires(Itr itr, Sentinel sentinel){
		{ itr < sentinel } -> std::convertible_to<bool>;
	};

	template <typename Itr, typename Sentinel>
	constexpr bool itrSentinelCheckable = requires(Itr itr, Sentinel sentinel){
		{ itr != sentinel } -> std::convertible_to<bool>;
	};

	template <typename Itr, typename Sentinel>
	constexpr bool itrRangeOrderCheckable = requires(Itr itr, Sentinel sentinel){
		{ itr <= sentinel } -> std::convertible_to<bool>;
	};

	template <typename Itr, typename Sentinel>
	constexpr void itrRangeOrderCheck(const Itr& itr, const Sentinel& sentinel){
		if constexpr(itrRangeOrderCheckable<Itr, Sentinel>){
			assert(itr <= sentinel);
		}
	}

	template <bool replace, typename Src, typename Dst>
	constexpr void swap_or_replace(Src src, Dst dst) noexcept(
		(replace && std::is_nothrow_move_assignable_v<std::iter_value_t<Src>>)
		||
		(!replace && std::is_nothrow_swappable_v<std::iter_value_t<Src>>)
		){
		if constexpr(replace){
			*dst = std::move(*src);
		} else{
			std::iter_swap(src, dst);
		}
	}


	template <
		bool replace,
		std::permutable Itr,
		std::permutable Sentinel,
		std::indirectly_unary_invocable<Itr> Proj = std::identity,
		std::indirect_unary_predicate<std::projected<Itr, Proj>> Pred
	>
	requires requires{
		requires std::bidirectional_iterator<Sentinel>;
		requires std::sentinel_for<Sentinel, Itr>;
	}
	[[nodiscard]] constexpr Itr
		remove_if_unstable_impl(Itr first, Sentinel sentinel, Pred pred, Proj porj = {}){
		algo::itrRangeOrderCheck(first, sentinel);

		first = std::ranges::find_if(first, sentinel, pred, porj);

		while(first != sentinel){
			--sentinel;
			algo::swap_or_replace<replace>(sentinel, first);

			first = std::ranges::find_if(first, sentinel, pred, porj);
		}

		return first;
	}

	template <bool replace, std::permutable Itr, std::permutable Sentinel, typename Ty, typename Proj = std::identity>
		requires requires{
		requires std::bidirectional_iterator<Sentinel>;
		requires std::sentinel_for<Sentinel, Itr>;
	}
	[[nodiscard]] constexpr Itr remove_unstable_impl(Itr first, Sentinel sentinel, const Ty& val, const Proj porj = {}){
		if constexpr(requires{
			algo::remove_if_unstable_impl<replace>(first, sentinel, std::bind_front(std::equal_to<Ty>{}, std::cref(val)), porj);
		}){
			return algo::remove_if_unstable_impl<replace>(first, sentinel, std::bind_front(std::equal_to<Ty>{}, std::cref(val)), porj);
		} else{
			return algo::remove_if_unstable_impl<replace>(first, sentinel, std::bind_front(std::equal_to<>{}, std::cref(val)), porj);
		}
	}

	template <bool replace, std::permutable Itr, std::permutable Sentinel, typename Proj = std::identity,
	          std::indirect_unary_predicate<std::projected<Itr, Proj>> Pred>
	requires requires(Sentinel sentinel){
		requires std::bidirectional_iterator<Sentinel>;
		requires std::sentinel_for<Sentinel, Itr>;
	}
	[[nodiscard]] constexpr Sentinel remove_unique_if_unstable_impl(Itr first, Sentinel sentinel, Pred pred, Proj porj = {}){
		algo::itrRangeOrderCheck(first, sentinel);

		Itr firstFound = std::ranges::find_if(first, sentinel, pred, porj);
		if(firstFound != sentinel){
			sentinel = std::prev(std::move(sentinel));

			algo::swap_or_replace<replace>(sentinel, firstFound);

			return sentinel;
		}

		return sentinel;
	}

	template <bool replace, std::permutable Itr, std::permutable Sentinel, typename Ty, typename Proj = std::identity>
	requires requires(Sentinel sentinel){--sentinel; requires std::sentinel_for<Sentinel, Itr>;}
	[[nodiscard]] constexpr Itr remove_unique_unstable_impl(Itr first, Sentinel sentinel, const Ty& val,
		const Proj porj = {}){

		if constexpr(requires{
			algo::remove_unique_if_unstable_impl<replace>(first, sentinel, std::bind_front(std::equal_to<Ty>{}, std::cref(val)), porj);
		}){
			return algo::remove_unique_if_unstable_impl<replace>(first, sentinel, std::bind_front(std::equal_to<Ty>{}, std::cref(val)), porj);
		} else{
			return algo::remove_unique_if_unstable_impl<replace>(first, sentinel, std::bind_front(std::equal_to<>{}, std::cref(val)), porj);
		}
	}

	export
	template <bool replace = false, std::permutable Itr, std::permutable Sentinel, typename Ty, typename Proj =
	          std::identity>
	[[nodiscard]] constexpr auto remove_unstable(Itr first, const Sentinel sentinel, const Ty& val,
		const Proj porj = {}){
		return std::ranges::subrange<Itr>{
				algo::remove_unstable_impl<replace>(first, sentinel, val, porj), sentinel
			};
	}

	export
	template <bool replace = false, std::ranges::random_access_range Rng, typename Ty, typename Proj = std::identity>
	[[nodiscard]] constexpr auto remove_unstable(Rng& range, const Ty& val, const Proj porj = {}){
		return std::ranges::borrowed_subrange_t<Rng>{
				algo::remove_unstable_impl<replace>(std::ranges::begin(range), std::ranges::end(range), val, porj),
				std::ranges::end(range)
			};
	}

	export
	template <bool replace = false, std::permutable Itr, std::permutable Sentinel, typename Proj = std::identity,
	          std::indirect_unary_predicate<std::projected<Itr, Proj>> Pred>
	[[nodiscard]] constexpr auto remove_if_unstable(Itr first, const Sentinel sentinel, Pred&& pred,
		const Proj porj = {}){
		return std::ranges::subrange<Itr>{
				algo::remove_if_unstable_impl<replace>(first, sentinel, pred, porj), sentinel
			};
	}

	export
	template <bool replace = false, std::ranges::random_access_range Rng, typename Proj = std::identity,
	          std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<Rng>, Proj>> Pred>
	[[nodiscard]] constexpr auto remove_if_unstable(Rng& range, Pred pred, const Proj porj = {}){
		return std::ranges::borrowed_subrange_t<Rng>{
				algo::remove_if_unstable_impl<replace>(std::ranges::begin(range), std::ranges::end(range), pred,
					porj),
				std::ranges::end(range)
			};
	}

	export
	template <bool replace = true, std::ranges::random_access_range Rng, typename Ty, typename Proj = std::identity>
		requires requires(Rng rng){
			requires std::ranges::sized_range<Rng>;
			rng.erase(std::ranges::begin(rng), std::ranges::end(rng)); requires std::ranges::sized_range<Rng>;
		}
	constexpr decltype(auto) erase_unstable(Rng& range, const Ty& val, const Proj porj = {}){
		auto oldSize = range.size();
		range.erase(
			algo::remove_unstable_impl<replace>(std::ranges::begin(range), std::ranges::end(range), val, porj),
			std::ranges::end(range));
		return oldSize - range.size();
	}

	export
	template <bool replace = true, std::ranges::random_access_range Rng, typename Proj = std::identity,
	          std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<Rng>, Proj>> Pred>
		requires requires(Rng rng){
			requires std::ranges::sized_range<Rng>;
			rng.erase(std::ranges::begin(rng), std::ranges::end(rng));
		}
	constexpr decltype(auto) erase_if_unstable(Rng& range, const Pred pred, const Proj porj = {}){
		// auto oldSize = std::ranges::size(range);
		return range.erase(
			algo::remove_if_unstable_impl<replace>(std::ranges::begin(range), std::ranges::end(range), pred, porj),
			std::ranges::end(range));
		// return oldSize - std::ranges::size(range);
	}

	export
	template <bool replace = true, std::ranges::random_access_range Rng, typename Ty, typename Proj = std::identity>
		requires requires(Rng rng){
			requires std::ranges::sized_range<Rng>;

			rng.erase(std::ranges::begin(rng), std::ranges::end(rng)); requires std::ranges::sized_range<Rng>;
		}
	constexpr decltype(auto) erase_unique_unstable(Rng& range, const Ty& val, const Proj porj = {}){
		auto oldSize = range.size();
		range.erase(
			algo::remove_unique_unstable_impl<replace>(std::ranges::begin(range), std::ranges::end(range), val,
				porj),
			std::ranges::end(range));
		return oldSize - range.size();
	}

	export
	template <bool replace = true, std::ranges::random_access_range Rng, typename Proj = std::identity,
	          std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<Rng>, Proj>> Pred>
		requires requires(Rng rng){
			requires std::ranges::sized_range<Rng>;
			rng.erase(std::ranges::begin(rng), std::ranges::end(rng)); requires std::ranges::sized_range<Rng>;
		}
	constexpr decltype(auto) erase_unique_if_unstable(Rng& range, Pred&& pred, const Proj porj = {}){
		auto oldSize = range.size();
		range.erase(
			algo::remove_unique_if_unstable_impl<
				replace>(std::ranges::begin(range), std::ranges::end(range), pred, porj), std::ranges::end(range));
		return oldSize - range.size();
	}

}

namespace mo_yanxi::algo{

}