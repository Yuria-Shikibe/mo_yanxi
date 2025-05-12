module;

#include <cassert>

#include "ext/adapted_attributes.hpp"

#if DEBUG_CHECK
#define COMP_AT_CHECK
#endif

#if DEBUG_CHECK
#define CHECKED_STATIC_CAST(type) dynamic_cast<type>
#else
#define CHECKED_STATIC_CAST(type) static_cast<type>
#endif

// NOLINTBEGIN(*-misplaced-const)

export module mo_yanxi.game.ecs.entity;

export import mo_yanxi.strided_span;
export import mo_yanxi.meta_programming;
export import mo_yanxi.seq_chunk;

import mo_yanxi.algo.hash;
import std;

namespace mo_yanxi::game::ecs{
	export struct bad_chunk_access : std::exception{
		[[nodiscard]] bad_chunk_access() = default;

		[[nodiscard]] explicit bad_chunk_access(char const* Message)
			: exception(Message){
		}
	};


	template <typename T>
	struct alignas(T) type_replacement{
		std::byte _[sizeof(T)];
	};

	template <std::size_t I, typename Tuple>
	consteval std::size_t element_offset() noexcept {
		using Tpl = tuple_to_seq_chunk_t<unary_apply_to_tuple_t<type_replacement, Tuple>>;
		union {
			char a[sizeof(Tpl)];
			Tpl t{};
		};
		auto* p = std::addressof(t.template get<I>());
		t.~Tpl();
		for (std::size_t i = 0;; ++i) {
			if (static_cast<void*>(a + i) == p) return i;
		}
	}

	template <std::size_t Index, typename Tuple>
	inline constexpr std::size_t chunk_tuple_offset_at_v = element_offset<Index, Tuple>();

	template <typename T, typename Tuple>
	inline constexpr std::size_t chunk_tuple_offset_of_v = element_offset<tuple_index_v<T, Tuple>, Tuple>();
	
	export using entity_id = struct entity*;
	export using entity_data_chunk_index = std::vector<int>::size_type;
	export auto invalid_chunk_idx = std::numeric_limits<entity_data_chunk_index>::max();

	export
	template <typename TupleT>
	struct archetype;

	export
	struct chunk_meta{

		template <typename TupleT>
		friend struct archetype;

	private:
		entity_id eid_{};

	public:
		[[nodiscard]] constexpr chunk_meta() = default;

		[[nodiscard]] constexpr explicit(false) chunk_meta(ecs::entity_id entity_id)
			noexcept : eid_(entity_id){
		}

		[[nodiscard]] constexpr entity_id id() const noexcept{
			return eid_;
		}
	};

	/*
	export
	template <typename T>
	using component = T;//: T{};
	*/

	export
	struct archetype_base{
	protected:
		template <typename T>
		[[nodiscard]] static constexpr chunk_meta& meta_of(T& comp) noexcept{
			return comp.template get<chunk_meta>();
		}

		template <typename T>
		[[nodiscard]] static constexpr const chunk_meta& meta_of(const T& comp) noexcept{
			return comp.template get<chunk_meta>();
		}

	public:
		virtual ~archetype_base() = default;

		virtual void reserve(std::size_t sz){

		}

		virtual std::size_t insert(const entity_id entity);

		virtual void erase(const entity_id entity);

		virtual void* get_chunk_partial_ptr(std::type_index type, std::size_t idx) noexcept = 0;

		[[nodiscard]] virtual bool has_type(std::type_index type) const = 0;

		// virtual void set_expired(entity_id entity_id);

		template <typename T>
		[[nodiscard]] bool has_type() const noexcept{
			if constexpr (std::same_as<T, chunk_meta>){
				return true;
			}else{
				return has_type(typeid(T));
			}
		}

		template <typename T>
		[[nodiscard]] T* try_get_comp(const std::size_t idx) noexcept{
			return static_cast<T*>(get_chunk_partial_ptr(typeid(T), idx));
		}

		template <typename T>
		[[nodiscard]] T* try_get_comp(entity_id id) noexcept;

		virtual void dump_staging() = 0;
	};

	export
	struct entity_ref;


	export
	template <typename Tuple>
		requires (is_tuple_v<Tuple>)
	using tuple_to_comp_t = std::conditional_t<
		std::same_as<std::tuple_element_t<0, Tuple>, chunk_meta>,
		tuple_to_seq_chunk_t<tuple_cat_t<Tuple>>,
		tuple_to_seq_chunk_t<tuple_cat_t<std::tuple<chunk_meta>, Tuple>>
	>;
	
	export
	struct entity{

	private:
		template <typename TupleT>
		friend struct archetype;
		friend struct archetype_base;
		friend struct entity_ref;

		// entity_id id_{};
		std::atomic_size_t referenced_count{};
		entity_data_chunk_index chunk_index_{invalid_chunk_idx};

		archetype_base* archetype_{};
		std::type_index type_{typeid(std::nullptr_t)};

	public:
		[[nodiscard]] entity() = default;

		[[nodiscard]] constexpr explicit(false) entity(std::type_index type)
			noexcept : type_(type){
		}

		[[nodiscard]] std::size_t get_ref_count() const noexcept{
			return referenced_count.load(std::memory_order_relaxed);
		}

		[[nodiscard]] constexpr entity_id id() noexcept{
			return this;
		}

		[[nodiscard]] constexpr std::type_index type() const noexcept{
			return type_;
		}

		[[nodiscard]] constexpr entity_data_chunk_index chunk_index() const noexcept{
			return chunk_index_;
		}



		/**
		 * @brief Only valid when an entity has its data, i.e. an entity has been truly added.
		 */
		explicit operator bool() const noexcept{
			return is_valid();
		}

		[[nodiscard]] constexpr bool is_expired() const noexcept{
			return get_archetype() == nullptr;
		}

		[[nodiscard]] constexpr bool is_valid() const noexcept{
			return chunk_index() != invalid_chunk_idx;
		}

		[[nodiscard]] constexpr archetype_base* get_archetype() const noexcept{
			return archetype_;
		}

		// void set_expired() const noexcept{
		// 	assert(!is_expired());
		//
		// }

		template <typename T>
		[[nodiscard]] T* try_get() const noexcept{
			assert(archetype_);
			return archetype_->try_get_comp<T>(chunk_index_);
		}

		template <typename T>
		[[nodiscard]] T& at() const noexcept{
#ifdef COMP_AT_CHECK
			if(!archetype_){
				std::println(std::cerr, "[FATAL ERROR] Illegal Access To Chunk<{}> on entity<{}> on empty archetype", typeid(T).name(), type().name());
				std::terminate();
			}
#endif

			T* ptr = archetype_->try_get_comp<T>(chunk_index_);

#ifdef COMP_AT_CHECK
			if(!ptr){
				std::println(std::cerr, "[FATAL ERROR] Illegal Access To Chunk<{}> on entity<{}> at[{}]", typeid(T).name(), type().name(), chunk_index());
				std::terminate();
			}
#endif

			return *ptr;
		}

		template <typename Tuple, typename T>
			requires (contained_in<T, Tuple>)
		[[nodiscard]] T& unchecked_get() const;

		template <typename Tuple>
		[[nodiscard]] tuple_to_comp_t<Tuple>& unchecked_get() const;

		template <typename Tuple, typename T>
		[[nodiscard]] T* get() const noexcept;

		template <typename Tuple>
		[[nodiscard]] tuple_to_comp_t<Tuple>* get() const noexcept;

		template <typename C, typename T>
		FORCE_INLINE constexpr T& operator->*(T C::* mptr) const noexcept{
			return at<C>().*mptr;
		}

		template <typename T>
			requires std::is_member_function_pointer_v<T>
		FORCE_INLINE constexpr auto operator->*(T mfptr) const noexcept{
			using trait = mptr_info<T>;
			using class_type = typename trait::class_type;

			return [&, this]<std::size_t ...Idx>(std::index_sequence<Idx...>){
				return [mfptr, &object = this->at<class_type>()](std::tuple_element_t<Idx, typename trait::func_args> ...args) -> decltype(auto) {
					return std::invoke(mfptr, object, std::forward<decltype(args)>(args)...);
				};
			}(std::make_index_sequence<std::tuple_size_v<typename trait::func_args>>{});
		}
	};


	struct entity_ref{
	private:
		entity_id id_{};

		void try_incr_ref() const noexcept{
			if(id_ && !id_->is_expired()){
				id_->referenced_count.fetch_add(1, std::memory_order_relaxed);
			}
		}

		void try_drop_ref() const noexcept{
			if(id_){
				auto rst = id_->referenced_count.fetch_sub(1, std::memory_order_relaxed);
				assert(rst != 0);
			}
		}
	public:
		[[nodiscard]] constexpr entity_ref() noexcept = default;

		[[nodiscard]] explicit(false) entity_ref(entity_id entity_id) noexcept :
			id_(entity_id){
			try_incr_ref();
		}

		[[nodiscard]] explicit(false) entity_ref(entity& entity) noexcept :
			id_(entity.id()){
			if(entity){
				id_->referenced_count.fetch_add(1, std::memory_order_relaxed);
			}
		}

		entity_ref(const entity_ref& other) noexcept
			: id_{other.id_}{
			try_incr_ref();
		}

		entity_ref(entity_ref&& other) noexcept
			: id_{std::exchange(other.id_, {})}{
		}

		entity_ref& operator=(const entity_ref& other) noexcept{
			if(this == &other) return *this;
			try_drop_ref();
			id_ = other.id_;
			try_incr_ref();
			return *this;
		}

		entity_ref& operator=(entity_id other) noexcept{
			if(id_ == other) return *this;
			try_drop_ref();
			id_ = other;
			try_incr_ref();
			return *this;
		}

		entity_ref& operator=(std::nullptr_t other) noexcept{
			try_drop_ref();
			id_ = nullptr;
			return *this;
		}

		entity_ref& operator=(entity_ref&& other) noexcept{
			if(this == &other) return *this;
			try_drop_ref();
			id_ = std::exchange(other.id_, {});
			return *this;
		}

		~entity_ref(){
			try_drop_ref();
		}

		constexpr entity* operator->() const noexcept{
			return id_;
		}

		constexpr entity& operator*() const noexcept{
			assert(id_);
			return *id_;
		}

		void reset(const entity_id entity_id = nullptr) noexcept{
			this->operator=(entity_id);
		}

		void reset(entity& entity) noexcept{
			this->operator=(entity);
		}

		explicit constexpr operator bool() const noexcept{
			return id_ && *id_;
		}

		[[nodiscard]] entity_id id() const noexcept{
			return id_;
		}

		template <typename C, typename T>
		FORCE_INLINE constexpr T& operator->*(T C::* mptr) const noexcept{
			assert(id_);
			return id_->operator->*<C, T>(mptr);
		}


		template <typename T>
			requires (std::is_member_function_pointer_v<T>)
		FORCE_INLINE constexpr decltype(auto) operator->*(T mfptr) const noexcept{
			assert(id_);
			return id_->operator->*(mfptr);
		}

		constexpr explicit(false) operator entity&() const noexcept{
			assert(id_);
			return *id_;
		}

		constexpr explicit(false) operator entity_id() const noexcept{
			return id_;
		}

		/**
		 * @brief drop the referenced entity if it is already erased
		 */
		constexpr bool drop_if_expired() noexcept{
			if(id_ && id_->is_expired()){
				this->operator=(nullptr);
				return true;
			}
			return false;
		}

		[[nodiscard]] constexpr bool is_expired() const noexcept{
			return id_ && id_->is_expired();
		}

		constexpr friend bool operator==(const entity_ref& lhs, const entity_ref& rhs) noexcept = default;
		constexpr friend bool operator==(const entity_ref& lhs, const entity_id eid) noexcept{
			return lhs.id_ == eid;
		}
		constexpr friend bool operator==(const entity_id eid, const entity_ref& rhs) noexcept{
			return eid == rhs.id_;
		}
	};



	export
	template <typename Tgt, typename EntityChunkDesc, typename ChunkPartial>
		requires requires{
		requires is_tuple_v<EntityChunkDesc>;
		requires contained_in<Tgt, EntityChunkDesc>;
		requires contained_in<std::remove_cvref_t<ChunkPartial>, EntityChunkDesc>;
		}
	constexpr [[nodiscard]] decltype(auto) chunk_neighbour_of(ChunkPartial& value) noexcept{


		using Tup = tuple_cat_t<std::tuple<chunk_meta>, EntityChunkDesc>;
		decltype(auto) rst = mo_yanxi::neighbour_of<Tgt, Tup, ChunkPartial>(value);
#if DEBUG_CHECK
		auto get_meta = [&]<typename C>() -> const chunk_meta& {
			return mo_yanxi::neighbour_of<chunk_meta, C, ChunkPartial>(value);
		};

		const entity_id eid = get_meta.template operator()<Tup>().id();
		assert(std::addressof(eid->at<Tgt>()) == std::addressof(rst));
#endif
		return rst;
	}

	export
	template <typename EntityChunkDesc, typename ChunkPartial>
		requires requires{
		requires is_tuple_v<EntityChunkDesc>;
		requires contained_in<std::remove_cvref_t<ChunkPartial>, EntityChunkDesc>;
		}
	constexpr [[nodiscard]] decltype(auto) chunk_of(ChunkPartial& value) noexcept{
		using Tup = tuple_to_comp_t<tuple_cat_t<std::tuple<chunk_meta>, EntityChunkDesc>>;
		decltype(auto) rst = mo_yanxi::seq_chunk_cast<Tup>(&value);
#if DEBUG_CHECK
		auto get_meta = [&]<typename C>() -> const chunk_meta& {
			return mo_yanxi::neighbour_of<chunk_meta, C, ChunkPartial>(value);
		};

		const entity_id eid = get_meta.template operator()<Tup>().id();
		assert(eid->get<EntityChunkDesc>() == rst);
#endif
		return *rst;
	}

	export
	template <typename T>
	struct component_custom_behavior_base{
		using value_type = T;

		static void on_init(const chunk_meta& meta, value_type& comp) = delete;
		static void on_terminate(const chunk_meta& meta, value_type& comp) = delete;
		static void on_relocate(const chunk_meta& meta, value_type& comp) = delete;

		static void on_init(const chunk_meta& meta, const value_type& comp) = delete;
		static void on_terminate(const chunk_meta& meta, const value_type& comp) = delete;
		static void on_relocate(const chunk_meta& meta, const value_type& comp) = delete;
	};

	export
	template <typename TypeDesc>
	struct archetype_custom_behavior_base{
		using value_type = tuple_to_comp_t<TypeDesc>;


		template <typename ...Ts>
		static auto get_unwrap_of(value_type& comp) noexcept{
			return [&] <std::size_t... I>(std::index_sequence<I...>){
				return std::tie(get<Ts>(comp) ...);
			}(std::index_sequence_for<Ts ...>{});
		}

		template <typename ...Ts>
		static auto get_unwrap_of(const value_type& comp) noexcept{
			return [&] <std::size_t... I>(std::index_sequence<I...>){
				return std::tie(get<Ts>(comp) ...);
			}(std::index_sequence_for<Ts ...>{});
		}

		constexpr static entity_id id_of_chunk(const value_type& comp) noexcept{
			return static_cast<const chunk_meta&>(get<chunk_meta>(comp)).id();
		}

		static void on_init(value_type& comp) = delete;
		static void on_terminate(value_type& comp) = delete;
		static void on_relocate(value_type& comp) = delete;
	};

	export
	template <typename T>
	struct archetype_custom_behavior : archetype_custom_behavior_base<T>{};

	export
	template <typename T>
	struct component_custom_behavior : component_custom_behavior_base<T>{};

	template <typename T>
	using unwrap_type = typename T::type;

	template <typename T>
	struct get_base_types{
		using direct_parent = typename decltype([]{
			if constexpr (requires{
				typename component_custom_behavior<T>::base_types;
			}){
				if constexpr (is_tuple_v<typename component_custom_behavior<T>::base_types>){
					return std::type_identity<typename component_custom_behavior<T>::base_types>{};
				}else{
					return std::type_identity<std::tuple<typename component_custom_behavior<T>::base_types>>{};
				}
			}else{
				return std::type_identity<std::tuple<>>{};
			}
		}())::type;

		using type = tuple_cat_t<
			std::tuple<T>,
			all_apply_to<tuple_cat_t,
				unary_apply_to_tuple_t<unwrap_type,
					unary_apply_to_tuple_t<get_base_types, direct_parent>>>>;
	};

	template <typename T>
	struct component_trait{
		using value_type = typename component_custom_behavior<T>::value_type;
		static_assert(std::same_as<T, value_type>);

		using base_types = typename get_base_types<value_type>::type;

		static constexpr bool is_polymorphic = !requires{
			typename component_custom_behavior<T>::base_types;
		};

		static void on_init(const chunk_meta& meta, T& comp) {
			if constexpr (requires{
				component_custom_behavior<T>::on_init(meta, comp);
			}){
				component_custom_behavior<T>::on_init(meta, comp);
			}
		}

		static void on_terminate(const chunk_meta& meta, T& comp) {
			if constexpr (requires{
				component_custom_behavior<T>::on_terminate(meta, comp);
			}){
				component_custom_behavior<T>::on_terminate(meta, comp);
			}
		}

		static void on_relocate(const chunk_meta& meta, T& comp) {
			if constexpr (requires{
				component_custom_behavior<T>::on_relocate(meta, comp);
			}){
				component_custom_behavior<T>::on_relocate(meta, comp);
			}
		}
	};

	template <typename T>
	struct archetype_trait{
		using value_type = typename archetype_custom_behavior<T>::value_type;
		static_assert(std::same_as<tuple_to_comp_t<T>, value_type>);

		static void on_init(value_type& chunk) {
			if constexpr (requires{
				archetype_custom_behavior<T>::on_init(chunk);
			}){
				archetype_custom_behavior<T>::on_init(chunk);
			}
		}

		static void on_terminate(value_type& chunk) {
			if constexpr (requires{
				archetype_custom_behavior<T>::on_terminate(chunk);
			}){
				archetype_custom_behavior<T>::on_terminate(chunk);
			}
		}

		static void on_relocate(value_type& chunk) {
			if constexpr (requires{
				archetype_custom_behavior<T>::on_relocate(chunk);
			}){
				archetype_custom_behavior<T>::on_relocate(chunk);
			}
		}
	};

	template <typename L, typename R>
	struct type_pair{
		using first_type = L;
		using second_type = R;
	};

	struct type_comp_convert{
		using converter = std::add_pointer_t<void*(void*) noexcept>;
		converter comp_converter;

		[[nodiscard]] constexpr type_comp_convert() = default;

		template <typename Ty, typename MostDerivedTy, typename ChunkSeqTy>
		[[nodiscard]] constexpr type_comp_convert(std::in_place_type_t<Ty>, std::in_place_type_t<MostDerivedTy>, std::in_place_type_t<ChunkSeqTy>)
			noexcept :  comp_converter(+[](void* c) noexcept -> void*{
				auto& chunk = *static_cast<ChunkSeqTy*>(c);
				auto& most_derived_comp = chunk.template get<MostDerivedTy>();
				return static_cast<void*>(static_cast<Ty*>(std::addressof(most_derived_comp)));
			})  {}

		constexpr explicit operator bool() const noexcept{
			return comp_converter != nullptr;
		}


		template <typename T>
		[[nodiscard]] void* get(T& chunk) const noexcept{
			return comp_converter(std::addressof(chunk));
		}
	};


	struct empty_state{};

	struct type_index_to_convertor{
		std::type_index type{typeid(empty_state)};
		type_comp_convert convertor{};
	};

	template <typename T>
	using derive_map_of_trait = typename decltype([]{
		// if constexpr (!component_trait<T>::is_polymorphic){
			using bases = typename component_trait<T>::base_types;
			return []<std::size_t ...Idx>(std::index_sequence<Idx...>){
				return std::type_identity<std::tuple<
					type_pair<std::tuple_element_t<Idx, bases>, T> ...
				>>{};
			}(std::make_index_sequence<std::tuple_size_v<bases>>{});
		// }else{
		// 	return std::type_identity<std::tuple<>>{};
		// }
	}())::type;

	export
	template <typename Pair, typename Ty>
	struct find_if_first_equal : std::bool_constant<std::same_as<typename Pair::first_type, Ty>>{};

	template <typename TupleT>
	struct archetype : archetype_base{
		using raw_tuple = TupleT;
		// using raw_tuple = std::tuple<int>;
		using appended_tuple = tuple_cat_t<std::tuple<chunk_meta>, raw_tuple>;
		using components = tuple_to_seq_chunk_t<appended_tuple>;
		static constexpr std::size_t chunk_comp_count = std::tuple_size_v<appended_tuple>;

		static constexpr std::size_t single_chunk_size = sizeof(components);

		using base_to_derive_map = all_apply_to<tuple_cat_t, unary_apply_to_tuple_t<derive_map_of_trait, appended_tuple>>;
		static constexpr std::size_t type_hash_map_size = std::tuple_size_v<base_to_derive_map>;


		//believe that this is faster than static
		const std::array<type_index_to_convertor, type_hash_map_size * 2> type_convertor_hash_map = []{
			std::array<type_index_to_convertor, type_hash_map_size * 2> hash_array{};

			std::array<type_index_to_convertor, type_hash_map_size> temp{};
			[&] <std::size_t ...Idx>(std::index_sequence<Idx...>){
				((temp[Idx] = type_index_to_convertor{
					typeid(typename std::tuple_element_t<Idx, base_to_derive_map>::first_type),
					type_comp_convert{
					std::in_place_type<typename std::tuple_element_t<Idx, base_to_derive_map>::first_type>,
					std::in_place_type<typename std::tuple_element_t<Idx, base_to_derive_map>::second_type>,
					std::in_place_type<components>
				}}), ...);
			}(std::make_index_sequence<type_hash_map_size>{});

			algo::make_hash(hash_array, temp, &type_index_to_convertor::type);

			return hash_array;
		}();

	private:
		std::vector<components> chunks{};

		std::mutex staging_mutex_{};
		std::vector<components> staging{};
		std::mutex staging_expire_mutex_{};
		std::vector<entity_id> staging_expires{};
	protected:

		template <typename ...Ts>
		static auto get_unwrap_of(components& comp) noexcept{
			return archetype_custom_behavior<raw_tuple>::template get_unwrap_of<Ts ...>(comp);
		}

		template <typename ...Ts>
		static auto get_unwrap_of(const components& comp) noexcept{
			return archetype_custom_behavior<raw_tuple>::template get_unwrap_of<Ts ...>(comp);
		}

		constexpr static entity_id id_of_chunk(const components& comp) noexcept{
			return archetype_custom_behavior<raw_tuple>::id_of_chunk(comp);
		}

	public:
		void reserve(std::size_t sz) override{
			chunks.reserve(sz);
		}

	private:
		template <bool set_archetype = true>
		std::size_t insert(components&& comp){
			auto idx = chunks.size();
			entity_id eid = this->id_of_chunk(comp);

			if(eid->chunk_index() != invalid_chunk_idx || (set_archetype && eid->get_archetype() != nullptr)){
				throw std::runtime_error{"Duplicated Insert"};
			}

			eid->chunk_index_ = idx;
			components& added_comp = chunks.emplace_back(std::move(comp));
			if constexpr (set_archetype)archetype_base::insert(eid);

			[&] <std::size_t... I>(std::index_sequence<I...>){
				(component_trait<std::tuple_element_t<I, raw_tuple>>::on_init(get<0>(added_comp), get<I + 1>(added_comp)), ...);
			}(std::make_index_sequence<std::tuple_size_v<raw_tuple>>());

			archetype_trait<raw_tuple>::on_init(added_comp);
			this->init(added_comp);

			return idx;
		}
	public:

		std::size_t insert(const entity_id eid) final {
			if(!eid)return 0;

			auto idx = chunks.size();

			if(eid->chunk_index() != invalid_chunk_idx || eid->get_archetype() != nullptr){
				throw std::runtime_error{"Duplicated Insert"};
			}

			eid->chunk_index_ = idx;
			components& added_comp = chunks.emplace_back(eid->id());
			archetype_base::insert(eid);

			[&] <std::size_t... I>(std::index_sequence<I...>){
				(component_trait<std::tuple_element_t<I, raw_tuple>>::on_init(get<0>(added_comp), get<I + 1>(added_comp)), ...);
			}(std::make_index_sequence<std::tuple_size_v<raw_tuple>>());

			archetype_trait<raw_tuple>::on_init(added_comp);
			this->init(added_comp);

			return idx;
		}

		void erase(const entity_id e) final {
			auto chunk_size = chunks.size();
			auto idx = e->chunk_index();

			if(idx >= chunk_size){
				throw std::runtime_error{"Invalid Erase"};
			}

			components& chunk = chunks[idx];

			this->terminate(chunk);
			archetype_trait<raw_tuple>::on_terminate(chunk);

			[&] <std::size_t... I>(std::index_sequence<I...>){
				(component_trait<std::tuple_element_t<I, raw_tuple>>::on_terminate(get<0>(chunk), get<I + 1>(chunk)), ...);
			}(std::make_index_sequence<std::tuple_size_v<raw_tuple>>());

			if(idx == chunk_size - 1){
				chunks.pop_back();
			}else{
				chunk = std::move(chunks.back());
				chunks.pop_back();

				this->id_of_chunk(chunk)->chunk_index_ = idx;

				[&] <std::size_t... I>(std::index_sequence<I...>){
					(component_trait<std::tuple_element_t<I, raw_tuple>>::on_relocate(get<0>(chunk), get<I + 1>(chunk)), ...);
				}(std::make_index_sequence<std::tuple_size_v<raw_tuple>>());

				archetype_trait<raw_tuple>::on_relocate(chunk);
			}


			archetype_base::erase(e);
		}

		[[nodiscard]] bool has_type(std::type_index type) const final{
			return algo::access_hash(type_convertor_hash_map, type, &type_index_to_convertor::type, {}, [](const type_index_to_convertor& conv){
				return !conv.convertor;
			}) != type_convertor_hash_map.end();
		}

		void* get_chunk_partial_ptr(std::type_index type, std::size_t idx) noexcept final {
			if(auto itr = algo::access_hash(type_convertor_hash_map, type, &type_index_to_convertor::type, {}, [](const type_index_to_convertor& conv){
				return !conv.convertor;
			}); itr != type_convertor_hash_map.end()){
				return itr->convertor.get(chunks[idx]);
			}else{
				return nullptr;
			}
		}

		[[nodiscard]] auto& at(entity_data_chunk_index idx) const noexcept{
			return chunks.at(idx);
		}

		[[nodiscard]] auto& at(entity_data_chunk_index idx) noexcept{
			return chunks.at(idx);
		}

		[[nodiscard]] auto& operator[](entity_data_chunk_index idx) const noexcept{
			return chunks[idx];
		}

		[[nodiscard]] auto& operator[](entity_data_chunk_index idx) noexcept{
			return chunks[idx];
		}

		template <typename ...T>
			requires (contained_in<std::decay_t<T>, raw_tuple> && ...)
		void push_staging(const entity_id e, T&& ...args){
			assert(e);

			using raw_tuple_type = std::tuple<T&& ...>;
			raw_tuple_type ref_tuple{std::forward<T>(args)...};

			using in_tuple = std::tuple<std::decay_t<T>...>;
			constexpr std::size_t in_tuple_size = sizeof...(T);

			e->id()->archetype_ = this;

			std::lock_guard _{staging_mutex_};
			[&] <std::size_t ...Idx>(std::index_sequence<Idx...>){
				staging.emplace_back(e, [&] <std::size_t I> (){
					using cur_type = std::tuple_element_t<I, raw_tuple>;
					constexpr std::size_t mapped_cur_idx = tuple_index_v<cur_type, in_tuple>;

					if constexpr (mapped_cur_idx == in_tuple_size){
						return cur_type{};
					}else{
						using param_type = std::tuple_element_t<mapped_cur_idx, raw_tuple_type>;
						return cur_type{std::forward<param_type>(std::get<mapped_cur_idx>(ref_tuple))};
					}
				}.template operator()<Idx>() ...);
			}(std::make_index_sequence<std::tuple_size_v<raw_tuple>>{});
		}

		void push_staging(const entity_id e, components&& comp) noexcept {
			assert(e);
			e->id()->archetype_ = this;
			chunk_meta& meta = get<chunk_meta>(comp);
			meta.eid_ = e;

			std::lock_guard _{staging_mutex_};
			staging.push_back(std::move(comp));
		}

		void dump_staging() final {
			auto sz = chunks.size();
			chunks.reserve(sz + staging.size());
			for (auto && data : staging){
				this->insert<false>(std::move(data));
			}

			if(staging.size() > 128){
				staging = decltype(staging){};
				staging.reserve(32);
			}else{
				staging.clear();
			}
		}

	protected:
		virtual void terminate(components& comps){

		}

		virtual void init(components& comps){

		}

	public:
		template <typename T>
		[[nodiscard]] strided_span<T> slice() noexcept{
			constexpr auto idx = tuple_match_first_v<find_if_first_equal, base_to_derive_map, T>;
			if constexpr (idx != std::tuple_size_v<base_to_derive_map>){
				if(chunks.empty()){
					return {nullptr, 0, single_chunk_size};
				}else{
					return strided_span<T>{
						static_cast<T*>(std::addressof(get<typename std::tuple_element_t<idx, base_to_derive_map>::second_type>(chunks.front()))),
						chunks.size(), single_chunk_size
					};
				}
			}else{
				static_assert(false, "Invalid Type! Try Register Base Type?");
			}
		}

		template <typename T>
		[[nodiscard]] strided_span<const T> slice() const noexcept{
			constexpr auto idx = tuple_match_first_v<find_if_first_equal, base_to_derive_map, T>;
			if constexpr (idx != std::tuple_size_v<base_to_derive_map>){
				if(chunks.empty()){
					return {nullptr, 0, single_chunk_size};
				}else{
					return strided_span<const T>{
						static_cast<const T*>(std::addressof(get<typename std::tuple_element_t<idx, base_to_derive_map>::second_type>(chunks.front()))),
						chunks.size(), single_chunk_size
					};
				}
			}else{
				static_assert(false, "Invalid Type! Try Register Base Type?");
			}
		}
	};

	using tuple = std::tuple<void*, double, float>;
	constexpr auto t = sizeof(tuple) - tuple_offset_at_v<1, tuple>;


	std::size_t archetype_base::insert(const entity_id entity){
		entity->id()->archetype_ = this;
		return 0;
	}

	void archetype_base::erase(const entity_id entity){
		entity->chunk_index_ = invalid_chunk_idx;
		entity->archetype_ = nullptr;
	}

	// void archetype_base::set_expired(entity_id entity_id){
	// 	entity_id->archetype_ = nullptr;
	// }


	template <typename T>
	T* archetype_base::try_get_comp(const entity_id id) noexcept{
		return try_get_comp<T>(id->chunk_index());
	}

	template <typename Tuple, typename T> requires (contained_in<T, Tuple>)
	T& entity::unchecked_get() const{
		assert(archetype_);
		auto& aty = CHECKED_STATIC_CAST(archetype<Tuple>&)(*archetype_);
		return aty.template slice<T>()[chunk_index_];
	}

	template <typename Tuple>
	tuple_to_comp_t<Tuple>& entity::unchecked_get() const{
		assert(archetype_);
		auto& aty = CHECKED_STATIC_CAST(archetype<Tuple>&)(*archetype_);
		return aty[chunk_index_];
	}

	template <typename Tuple, typename T>
	T* entity::get() const noexcept{
		const std::type_index idx = typeid(Tuple);
		if(idx != type())return nullptr;

		return &unchecked_get<Tuple, T>();
	}

	template <typename Tuple>
	tuple_to_comp_t<Tuple>* entity::get() const noexcept{
		const std::type_index idx = typeid(Tuple);
		if(idx != type())return nullptr;

		return &unchecked_get<Tuple>();
	}
}

// NOLINTEND(*-misplaced-const)

template <>
struct std::hash<mo_yanxi::game::ecs::entity_ref>{
	static std::size_t operator()(const mo_yanxi::game::ecs::entity_ref& ref) noexcept{
		static constexpr std::hash<const void*> hasher{};
		return hasher(ref.id());
	}
};