module;

#include "../adapted_attributes.hpp"

export module mo_yanxi.heterogeneous.open_addr_hash;

export import mo_yanxi.heterogeneous;
export import mo_yanxi.open_addr_hash_map;

import std;

namespace mo_yanxi{

	struct type_index_equal_to__not_null{
		using is_transparent = void;
		bool operator()(std::nullptr_t, const std::type_index b) const noexcept {
			return b == typeid(std::nullptr_t);
		}

		bool operator()(const std::type_index b, std::nullptr_t) const noexcept {
			return b == typeid(std::nullptr_t);
		}

		bool operator()(const std::type_index b, const std::type_index a) const noexcept {
			return a == b;
		}

		constexpr bool operator()(std::nullptr_t, std::nullptr_t) const noexcept {
			return true;
		}
	};

	struct type_index_hasher{
		using is_transparent = void;
		static constexpr std::hash<std::type_index> hasher{};

		std::size_t operator()(const std::type_index val) const noexcept {
			return hasher(val);
		}

		std::size_t operator()(std::nullptr_t) const noexcept {
			return hasher(typeid(std::nullptr_t));
		}
	};

	struct Proj{
		std::type_index operator ()(std::nullptr_t) const noexcept{
			return typeid(std::nullptr_t);
		}
	};

	export
	template <typename T>
	using type_fixed_hash_map = fixed_open_hash_map<
			std::type_index,
			T,
			nullptr,
			type_index_hasher, type_index_equal_to__not_null, Proj>;

	export
	template <typename T>
	using type_unordered_map = std::unordered_map<
			std::type_index,
			T,
			type_index_hasher, type_index_equal_to__not_null>;

	struct Prov{
		std::string operator ()(std::nullptr_t) const noexcept{
			return std::string();
		}
	};

	struct open_addr_equal : transparent::string_equal_to{
		using is_direct = void;

		using is_transparent = void;
		using string_equal_to::operator();

		FORCE_INLINE
		constexpr bool operator ()(const std::string_view sv) const noexcept{
			return sv.empty();
		}
	};

	export
	template <typename V>
	class string_open_addr_hash_map : public
		fixed_open_addr_hash_map<std::string, V, std::string_view, std::in_place, transparent::string_hasher, open_addr_equal/*, Prov*/>{
	private:
		using self_type = fixed_open_addr_hash_map<std::string, V, std::string_view, std::in_place, transparent::string_hasher, open_addr_equal/*, Prov*/>;

	public:
		using self_type::fixed_open_addr_hash_map;

		V& at(const std::string_view key){
			return this->find(key)->second;
		}

		const V& at(const std::string_view key) const {
			return this->find(key)->second;
		}

		V at(const std::string_view key, const V& def) const requires std::is_copy_assignable_v<V>{
			if(const auto itr = this->find(key); itr != this->end()){
				return itr->second;
			}
			return def;
		}

		V* try_find(const std::string_view key){
			if(const auto itr = this->find(key); itr != this->end()){
				return &itr->second;
			}
			return nullptr;
		}

		const V* try_find(const std::string_view key) const {
			if(const auto itr = this->find(key); itr != this->end()){
				return &itr->second;
			}
			return nullptr;
		}

		using self_type::insert_or_assign;

		template <class Arg>
		std::pair<typename self_type::iterator, bool> insert_or_assign(const std::string_view key, Arg&& val) {
			return this->insert_or_assign(static_cast<std::string>(key), std::forward<Arg>(val));
		}

		using self_type::operator[];

		V& operator[](const std::string_view key) {
			if(auto itr = this->find(key); itr != this->end()){
				return itr->second;
			}

			return this->emplace_impl(std::string(key)).first->second;
		}

		V& operator[](const char* key) {
			return operator[](std::string_view(key));
		}
	};
}
