export module mo_yanxi.flat_seq_map;

import std;

namespace mo_yanxi{
	export
	template <typename K, typename V>
	struct flat_seq_map_storage{
		using key_type = K;
		using mapped_type = V;

		key_type key;
		mapped_type value;
	};

	export
	template <typename K, typename V, typename Eq = std::ranges::less, typename Cont = std::vector<flat_seq_map_storage<K, V>>>
	struct flat_seq_map{
		using key_type = K;
		using mapped_type = V;

	private:
		using comp = Eq;

	public:
		using value_type = flat_seq_map_storage<key_type, mapped_type>;
		static_assert(std::same_as<std::ranges::range_value_t<Cont>, value_type>);

	private:
		Cont values{};

		static constexpr decltype(auto) try_to_underlying(const key_type& key) noexcept{
			if constexpr(std::is_enum_v<key_type>){
				return std::to_underlying(key);
			} else{
				return key;
			}
		}

		[[nodiscard]] static constexpr decltype(auto) get_key_proj(const value_type& value) noexcept{
			return flat_seq_map::try_to_underlying(value.key);
		}

	public:
		template <std::ranges::input_range Rng = std::initializer_list<mapped_type>>
			requires std::convertible_to<std::ranges::range_reference_t<Rng>, mapped_type>
		auto insert_chunk_front(const key_type& key, Rng&& rng){
			auto itr = std::ranges::lower_bound(values, flat_seq_map::try_to_underlying(key), comp{},
			                                    flat_seq_map::get_key_proj);
			return values.insert_range(itr, rng | std::views::transform([&](auto&& rng_value){
				return value_type{key, std::forward_like<Rng>(rng_value)};
			}));
		}

		template <std::ranges::input_range Rng = std::initializer_list<mapped_type>>
			requires std::convertible_to<std::ranges::range_reference_t<Rng>, mapped_type>
		auto insert_chunk_back(const key_type& key, Rng&& rng){
			auto itr = std::ranges::upper_bound(values, flat_seq_map::try_to_underlying(key), comp{},
			                                    flat_seq_map::get_key_proj);
			return values.insert_range(itr, rng | std::views::transform([&](auto&& rng_value){
				return value_type{key, std::forward_like<Rng>(rng_value)};
			}));
		}

		template <typename Key = const key_type&, typename... Args>
			requires std::same_as<key_type, std::remove_cvref_t<Key>> && std::constructible_from<mapped_type, Args...>
		auto insert_chunk_front(Key&& key, Args&&... value){
			auto itr = std::ranges::lower_bound(values, flat_seq_map::try_to_underlying(key), comp{},
			                                    flat_seq_map::get_key_proj);
			return values.insert(itr, flat_seq_map_storage{std::forward<Key>(key), mapped_type{std::forward<Args>(value)...}});
		}

		template <typename Key = const key_type&, typename... Args>
			requires std::same_as<key_type, std::remove_cvref_t<Key>> && std::constructible_from<mapped_type, Args...>
		auto insert_chunk_back(Key&& key, Args&&... value){
			auto itr = std::ranges::upper_bound(values, flat_seq_map::try_to_underlying(key), comp{},
			                                    flat_seq_map::get_key_proj);
			return values.insert(itr, flat_seq_map_storage{std::forward<Key>(key), mapped_type{std::forward<Args>(value)...}});
		}

		template <typename S>
		auto find(this S&& self, const key_type& key) noexcept{
			return std::ranges::equal_range(std::forward_like<S>(self.values),
			                                flat_seq_map::try_to_underlying(std::as_const(key)), comp{},
			                                flat_seq_map::get_key_proj) | std::views::transform(&value_type::value);
		}

		template <typename S>
		auto find_front(this S&& self, const key_type& key) noexcept{
			auto itr_begin = std::ranges::find_if_not(self.values, [&](const key_type& k){
				return comp{}(k, flat_seq_map::try_to_underlying(key));
			}, flat_seq_map::get_key_proj);

			auto itr_end = std::ranges::find_if(itr_begin, self.values.end(), [&](const key_type& k){
				return comp{}(flat_seq_map::try_to_underlying(key), k);
			}, flat_seq_map::get_key_proj);

			return std::ranges::borrowed_subrange_t<decltype(std::forward_like<S>(self.values))>{itr_begin, itr_end} |
				std::views::transform(&value_type::value);
		}

		template <typename S>
		auto find_back(this S&& self, const key_type& key) noexcept{
			auto itr_begin = std::ranges::find_if_not(self.values.rbegin(), self.values.rend(), [&](const key_type& k){
				return comp{}(flat_seq_map::try_to_underlying(key), k);
			}, flat_seq_map::get_key_proj);

			auto itr_end = std::ranges::find_if(itr_begin, self.values.rend(), [&](const key_type& k){
				return comp{}(k, flat_seq_map::try_to_underlying(key));
			}, flat_seq_map::get_key_proj);

			return std::ranges::borrowed_subrange_t<decltype(std::forward_like<S>(self.values))>{
					itr_end.base(), itr_begin.base()
				} | std::views::transform(&value_type::value);
		}
	};
}
