module;

#include <cassert>

#if DEBUG_CHECK
#define CHECKED_STATIC_CAST(type) dynamic_cast<type>
#else
#define CHECKED_STATIC_CAST(type) static_cast<type>
#endif

export module mo_yanxi.game.ecs.entity;

export import mo_yanxi.strided_span;
export import mo_yanxi.meta_programming;
export import mo_yanxi.seq_chunk;
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

		virtual void* get_chunk_ptr(std::type_index type, std::size_t idx) noexcept = 0;

		[[nodiscard]] virtual bool has_type(std::type_index type) const = 0;

		virtual void set_expired(entity_id entity_id);

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
			return static_cast<T*>(get_chunk_ptr(typeid(T), idx));
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
	using tuple_to_comp_t = tuple_to_seq_chunk_t<tuple_cat_t<std::tuple<chunk_meta>, Tuple>>;
	
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
			return chunk_index() != invalid_chunk_idx;
		}

		[[nodiscard]] constexpr bool is_expired() const noexcept{
			return get_archetype() == nullptr;
		}

		[[nodiscard]] constexpr archetype_base* get_archetype() const noexcept{
			return archetype_;
		}

		void set_expired() const noexcept{
			assert(!is_expired());

		}

		template <typename T>
		[[nodiscard]] T* try_get() const noexcept{
			return archetype_->try_get_comp<T>(chunk_index_);
		}

		template <typename T>
		[[nodiscard]] T& at() const noexcept{
			if(!archetype_){
				const auto str = std::format("Illegal Access To Chunk<{}> on entity<{}> on empty archetype", typeid(T).name(), type().name());
				throw bad_chunk_access{str.c_str()};
			}

			T* ptr = archetype_->try_get_comp<T>(chunk_index_);
			if(!ptr){
				T* ptr = archetype_->try_get_comp<T>(chunk_index_);

				const auto str = std::format("Illegal Access To Chunk<{}> on entity<{}> at[{}]", typeid(T).name(), type().name(), chunk_index());
				throw bad_chunk_access{str.c_str()};
			}
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
	};


	struct entity_ref{
	private:
		entity_id id_{};

		void try_incr_ref() const noexcept{
			if(id_ && *id_){
				id_->referenced_count.fetch_add(1, std::memory_order_relaxed);
			}
		}

		void try_drop_ref() const noexcept{
			if(id_ && *id_){
				id_->referenced_count.fetch_sub(1, std::memory_order_relaxed);
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

		void reset(const entity_id entity_id) noexcept{
			this->operator=(entity_id);
		}

		void reset(entity& entity) noexcept{
			this->operator=(entity);
		}

		explicit constexpr operator bool() const noexcept{
			return id_;
		}

		[[nodiscard]] entity_id id() const noexcept{
			return id_;
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
		constexpr void drop_if_expired() noexcept{
			if(id_ && id_->is_expired()){
				this->operator=({});
			}
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
	struct unwrap_component{
		template <typename T>
			requires requires(T&& t){
				*t;
			}
		constexpr static auto&& operator()(T&& comp) noexcept{
			return std::forward_like<T>(*comp);
		}
	};

	export
	template <typename T>
	struct component_custom_behavior_base{
		using value_type = T;

		static void on_init(const chunk_meta& meta, value_type& comp) = delete;
		static void on_terminate(const chunk_meta& meta, value_type& comp) = delete;
		static void on_relocate(const chunk_meta& meta, value_type& comp) = delete;
	};

	export
	template <typename T>
	struct component_custom_behavior : component_custom_behavior_base<T>{};

	//TODO derive check
	// template <typename T, typename Tpl>
	// constexpr inline bool all_derived_from = []() constexpr {
	// 	bool
	// }();

	template <typename T>
	struct component_trait{
		// using trait = component_custom_behavior<T>;
		using value_type = typename component_custom_behavior<T>::value_type;

		using base_types = typename decltype([]{
			if constexpr (requires{
				typename component_custom_behavior<T>::base_types;
			}){
				if constexpr (is_tuple_v<typename component_custom_behavior<T>::base_types>){
					return std::type_identity<tuple_cat_t<typename component_custom_behavior<T>::base_types, std::tuple<value_type>>>{};
				}else{
					return std::type_identity<std::tuple<typename component_custom_behavior<T>::base_types, value_type>>{};
				}

			}else{
				return std::type_identity<std::tuple<value_type>>{};
			}
		}())::type;

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

	template <typename L, typename R>
	struct type_pair{
		using first_type = L;
		using second_type = R;
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

	// using Base = derive_map_of_trait<C>;

	// constexpr auto a = tuple_match_first_v<find_if_first_equal, Base, C>;

	template <typename TupleT>
	struct archetype : archetype_base{
		using raw_tuple = TupleT;
		// using raw_tuple = std::tuple<int>;
		using appended_tuple = tuple_cat_t<std::tuple<chunk_meta>, raw_tuple>;
		using components = tuple_to_seq_chunk_t<appended_tuple>;
		static constexpr std::size_t chunk_size = std::tuple_size_v<appended_tuple>;

		static constexpr std::size_t single_chunk_size = sizeof(components);

		using derive_map = all_apply_to<tuple_cat_t, unary_apply_to_tuple_t<derive_map_of_trait, raw_tuple>>;
	private:
		std::vector<components> chunks{};

		std::mutex staging_mutex_{};
		std::vector<components> staging{};
		std::mutex staging_expire_mutex_{};
		std::vector<entity_id> staging_expires{};
	protected:

		template <typename ...Ts>
		static auto get_unwrap_of(components& comp) noexcept{
			return [&] <std::size_t... I>(std::index_sequence<I...>){
				return std::tie(get<Ts>(comp) ...);
			}(std::index_sequence_for<Ts ...>{});
		}

		template <typename ...Ts>
		static auto get_unwrap_of(const components& comp) noexcept{
			return [&] <std::size_t... I>(std::index_sequence<I...>){
				return std::tie(get<Ts>(comp) ...);
			}(std::index_sequence_for<Ts ...>{});
		}

		constexpr static entity_id id_of_chunk(const components& comp) noexcept{
			return static_cast<const chunk_meta&>(get<chunk_meta>(comp)).eid_;
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
			}


			archetype_base::erase(e);
			e->id()->chunk_index_ = invalid_chunk_idx;
		}

		bool has_type(std::type_index type) const override{
			bool rst{};
			[&, this] <std::size_t ...Idx>(std::index_sequence<Idx...>){
				([&, this]<std::size_t I>(){
					if(rst)return;

					using Ty = std::tuple_element_t<I, appended_tuple>;
					if(std::type_index{typeid(Ty)} == type){
						rst = true;
					}
				}.template operator()<Idx>(), ...);
			}(std::make_index_sequence<std::tuple_size_v<appended_tuple>>{});
			return rst;
		}

		void* get_chunk_ptr(std::type_index type, std::size_t idx) noexcept final {
			if(idx >= chunks.size())return nullptr;
			void* ptr{};
			[&, this] <std::size_t ...Idx>(std::index_sequence<Idx...>){
				([&, this]<std::size_t I>(){
					if(ptr)return;

					using Ty = std::tuple_element_t<I, appended_tuple>;
					if(std::type_index{typeid(Ty)} == type){
						// auto chunk = reinterpret_cast<std::byte*>(chunks.data() + idx);
						// constexpr auto off = chunk_tuple_offset_at_v<I, appended_tuple>;
						ptr = &get<Ty>(chunks[idx]);
					}
				}.template operator()<Idx>(), ...);
			}(std::make_index_sequence<std::tuple_size_v<appended_tuple>>{});
			return ptr;
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
			[&] <std::size_t ...Idx>(std::index_sequence<Idx...>) -> auto& {
				return staging.emplace_back(e, [&] <std::size_t I> (){
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

			/*
			* components comp = [&] <std::size_t ...Idx>(std::index_sequence<Idx...>) -> auto {
				return components{e, [&] <std::size_t I> (){
					using cur_type = std::tuple_element_t<I, raw_tuple>;
					constexpr std::size_t mapped_cur_idx = tuple_index_v<cur_type, in_tuple>;

					if constexpr (mapped_cur_idx == in_tuple_size){
						return cur_type{};
					}else{
						using param_type = std::tuple_element_t<mapped_cur_idx, raw_tuple_type>;
						return cur_type{std::forward<param_type>(std::get<mapped_cur_idx>(ref_tuple))};
					}
				}.template operator()<Idx>() ...};
			}(std::make_index_sequence<std::tuple_size_v<raw_tuple>>{});

			std::lock_guard _{staging_mutex_};
			staging.push_back(std::move(comp));
			 */
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
			constexpr auto idx = tuple_match_first_v<find_if_first_equal, derive_map, T>;
			if constexpr (idx != std::tuple_size_v<derive_map>){
				if(chunks.empty()){
					return {nullptr, 0, single_chunk_size};
				}else{
					return strided_span<T>{
						static_cast<T*>(&get<typename std::tuple_element_t<idx, derive_map>::second_type>(chunks.front())),
						chunks.size(), single_chunk_size
					};
				}
			}else if constexpr (contained_in<T, appended_tuple>){
				return strided_span<T>{
					reinterpret_cast<T*>(reinterpret_cast<std::byte*>(chunks.data()) +
						chunk_tuple_offset_of_v<T, appended_tuple>),
					chunks.size(), single_chunk_size
				};
			}else{
				static_assert(false, "Invalid Type!");
			}
		}

		template <typename T>
		[[nodiscard]] strided_span<const T> slice() const noexcept{
			constexpr auto idx = tuple_match_first_v<find_if_first_equal, derive_map, T>;
			if constexpr (idx != std::tuple_size_v<derive_map>){
				if(chunks.empty()){
					return {nullptr, 0, single_chunk_size};
				}else{
					return strided_span<T>{
						static_cast<const T*>(&get<typename std::tuple_element_t<idx, derive_map>::second_type>(chunks.front())),
						chunks.size(), single_chunk_size
					};
				}
			}else if constexpr (contained_in<T, appended_tuple>){
				return strided_span<const T>{
					reinterpret_cast<const T*>(reinterpret_cast<const std::byte*>(chunks.data()) +
						chunk_tuple_offset_of_v<const T, appended_tuple>),
					chunks.size(), single_chunk_size
				};
			}else{
				static_assert(false, "Invalid Type!");
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
		entity->archetype_ = nullptr;
	}

	void archetype_base::set_expired(entity_id entity_id){
		entity_id->archetype_ = nullptr;
	}


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

