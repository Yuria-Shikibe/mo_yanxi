//
// Created by Matrix on 2024/5/25.
//

export module ext.collection_operation;

import std;

export namespace mo_yanxi::algo{
	enum struct merge_policy : int{
		replace   = 0,
		join      = 0b0000'0001,
		overlaped = 0b0000'0010,
		minus     = 0b0000'0100,
		diff      = 0b0000'1000,

		reserve = 0b0001'0000,
	};

	constexpr std::array AllBoolOp{
			merge_policy::replace,
			merge_policy::join,
			merge_policy::overlaped,
			merge_policy::minus,
			merge_policy::diff,
		};

	constexpr std::array AllNamedBoolOp{
			std::pair<merge_policy, std::string_view>{merge_policy::replace, "blender_icon_select_set"},
			std::pair<merge_policy, std::string_view>{merge_policy::join, "blender_icon_select_extend"},
			std::pair<merge_policy, std::string_view>{merge_policy::overlaped, "blender_icon_select_intersect"},
			std::pair<merge_policy, std::string_view>{merge_policy::minus, "blender_icon_select_subtract"},
			std::pair<merge_policy, std::string_view>{merge_policy::diff, "blender_icon_select_difference"},
		};

	constexpr std::string_view getBoolOpName(merge_policy op){
		op = merge_policy{static_cast<std::underlying_type_t<merge_policy>>(op) & 0x0fu};

		switch(op){
			case merge_policy::replace : return AllNamedBoolOp[0].second;
			case merge_policy::join : return AllNamedBoolOp[1].second;
			case merge_policy::overlaped : return AllNamedBoolOp[2].second;
			case merge_policy::minus : return AllNamedBoolOp[3].second;
			case merge_policy::diff : return AllNamedBoolOp[4].second;

			default : std::unreachable();
		}
	}
}

export mo_yanxi::algo::merge_policy operator|(const mo_yanxi::algo::merge_policy l, const mo_yanxi::algo::merge_policy r){
	return mo_yanxi::algo::merge_policy{std::to_underlying(l) | std::to_underlying(r)};
}

export bool operator&(const mo_yanxi::algo::merge_policy l, const mo_yanxi::algo::merge_policy r){
	return std::to_underlying(l) & std::to_underlying(r);
}

export namespace mo_yanxi::algo{
	template <typename K, typename V, typename Hs, typename Eq, typename Alloc, typename Append>
		requires std::same_as<std::decay_t<Append>, std::unordered_map<K, V, Hs, Eq, Alloc>>
	void bool_merge(
		const merge_policy op,
		std::unordered_map<K, V, Hs, Eq, Alloc>& to,
		Append&& append
	){
		const auto opTy = merge_policy{std::to_underlying(op) & 0x0fu};
		const bool replace = !(op & merge_policy::reserve);

		if(opTy == merge_policy::replace){
			to = std::forward<std::unordered_map<K, V, Hs, Eq, Alloc>>(append);
			return;
		}

		std::unordered_set<K> checked{};
		switch(opTy){
			case merge_policy::join : break;
			case merge_policy::overlaped :{
				checked = to | std::ranges::views::keys | std::ranges::to<std::unordered_set<K>>();
			}
			default : checked.reserve(to.size());
		}

		for(auto&& [key, val] : std::forward<std::unordered_map<K, V, Hs, Eq, Alloc>>(append)){
			switch(opTy){
				case merge_policy::join :{
					if(replace){
						to.insert_or_assign(std::forward<decltype(key)>(key), std::forward<decltype(val)>(val));
					} else{
						to.try_emplace(std::forward<decltype(key)>(key), std::forward<decltype(val)>(val));
					}
					break;
				}

				case merge_policy::overlaped :{
					auto itr = to.find(key);

					if(itr == to.end()) break;

					checked.erase(key);

					if(replace){
						to.insert_or_assign(itr, std::forward<decltype(key)>(key), std::forward<decltype(val)>(val));
					}

					break;
				}

				case merge_policy::diff :{
					auto itr = to.find(key);

					if(itr == to.end()){
						to.try_emplace(std::forward<decltype(key)>(key), std::forward<decltype(val)>(val));
					} else{
						checked.insert(key);
					}

					break;
				}

				case merge_policy::minus :{
					auto itr = to.find(key);

					if(itr != to.end()) checked.insert(key);

					break;
				}

				default : std::unreachable();
			}
		}

		for(const auto& k : checked){
			to.erase(k);
		}
	}


	// using K = int;
	// using Hs = std::hash<K>;
	// using Eq = std::equal_to<K>;
	// using Alloc = std::allocator<K>;
	// using Append = std::unordered_set<K, Hs, Eq, Alloc>;

	template <typename K, typename Hs, typename Eq, typename Alloc, typename Append>
		requires std::same_as<std::decay_t<Append>, std::unordered_set<K, Hs, Eq, Alloc>>
	void bool_merge(const merge_policy op,
		std::unordered_set<K, Hs, Eq, Alloc>& to,
		Append&& append
	){
		const auto opTy = merge_policy{static_cast<std::underlying_type_t<merge_policy>>(op) & 0x0fu};

		if(opTy == merge_policy::replace){
			to = std::forward<std::unordered_set<K, Hs, Eq, Alloc>>(append);
			return;
		}

		std::unordered_set<K> checked{};
		switch(opTy){
			case merge_policy::join : break;
			case merge_policy::overlaped :{
				checked = to;
			}
			default : checked.reserve(to.size());
		}

		for(auto&& key : std::forward<std::unordered_set<K, Hs, Eq, Alloc>>(append)){
			switch(opTy){
				case merge_policy::join :{
					to.insert(std::forward<decltype(key)>(key));
					break;
				}

				case merge_policy::overlaped :{
					auto itr = to.find(key);

					if(itr == to.end()) break;

					checked.erase(key);

					to.insert(itr, std::forward<decltype(key)>(key));

					break;
				}

				case merge_policy::diff :{
					auto itr = to.find(key);

					if(itr == to.end()){
						to.insert(std::forward<decltype(key)>(key));
					} else{
						checked.insert(key);
					}

					break;
				}

				case merge_policy::minus :{
					auto itr = to.find(key);

					if(itr != to.end()) checked.insert(key);

					break;
				}

				default : std::unreachable();
			}
		}

		for(const auto& k : checked){
			to.erase(k);
		}
	}
}
