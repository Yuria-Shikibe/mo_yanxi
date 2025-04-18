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
	template <typename T>
	struct component;

	export
	struct archetype_base{
		virtual ~archetype_base() = default;

		virtual void reserve(std::size_t sz){

		}

		virtual std::size_t insert(const entity_id entity);

		virtual void erase(const entity_id entity);

		virtual void* get_chunk_ptr(std::type_index type, std::size_t idx) noexcept = 0;

		template <typename T>
		[[nodiscard]] component<T>* try_get_comp(const std::size_t idx) noexcept{
			return static_cast<component<T>*>(get_chunk_ptr(typeid(T), idx));
		}

		template <typename T>
		[[nodiscard]] component<T>* try_get_comp(entity_id id) noexcept;

		virtual void dump_staging() = 0;
	};

	export
	template <typename TupleT>
	struct archetype;

	export
	struct entity_ref;

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

		std::size_t get_ref_count() const noexcept{
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

		constexpr bool is_expired() const noexcept{
			return get_archetype() == nullptr;
		}

		constexpr archetype_base* get_archetype() const noexcept{
			return archetype_;
		}

		template <typename T>
		component<T>* try_get() const noexcept{
			return archetype_->try_get_comp<T>(chunk_index_);
		}

		template <typename Tuple, typename T>
			requires (contained_in<T, Tuple>)
		component<T>& unchecked_get() const;

		template <typename Tuple>
		unary_apply_to_tuple_t<component, Tuple>& unchecked_get() const;

		template <typename Tuple, typename T>
		component<T>* get() const noexcept;

		template <typename Tuple>
		unary_apply_to_tuple_t<component, Tuple>* get() const noexcept;
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

	//TODO component doesn't bring an ID, but as the first chunk item??

	template <typename T>
	struct component : T{
	/*private:
		entity_id eid_{};
		T value{};
	public:

		[[nodiscard]] component() = default;

		[[nodiscard]] explicit component(entity_id eid) noexcept
			: eid_(eid){
		}

		[[nodiscard]] component(entity_id eid, const T& value)
			: eid_(eid),
			  value(value){
		}

		[[nodiscard]] component(entity_id eid, T&& value)
			: eid_(eid),
			  value(std::move(value)){
		}

		[[nodiscard]] constexpr entity_id id() const noexcept{
			return eid_;
		}

		constexpr T* operator->() noexcept{
			return std::addressof(value);
		}

		constexpr const T* operator->() const noexcept{
			return std::addressof(value);
		}

		constexpr T& operator*() noexcept{
			return value;
		}

		constexpr const T& operator*() const noexcept{
			return value;
		}

		constexpr const T& val() const noexcept{
			return value;
		}

		constexpr T& val() noexcept{
			return value;
		}

		constexpr explicit(false) operator T&() noexcept{
			return value;
		}

		constexpr explicit(false) operator const T&() const noexcept{
			return value;
		}*/
	};

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

	template <typename T>
	using tuple_to_comp_t = unary_apply_to_tuple_t<component, T>;

	template <typename TupleT>
	struct archetype : archetype_base{
		using raw_tuple = TupleT;
		using appended_tuple = tuple_cat_t<std::tuple<chunk_meta>, raw_tuple>;
		using comp_tuple = unary_apply_to_tuple_t<component, appended_tuple>;
		using components = tuple_to_seq_chunk_t<comp_tuple>;
		static constexpr std::size_t chunk_size = std::tuple_size_v<appended_tuple>;

		/*
		template <std::size_t Idx>
		static constexpr std::size_t offset_at = []{
			constexpr auto sz = sizeof(tuple_take_n_elem_t<Idx, comp_tuple>);
			constexpr auto align = alignof(std::tuple_element_t<Idx, comp_tuple>);
			return (sz / align + (sz % align != 0)) * align;
		}();

		template <typename T>
		static constexpr std::size_t offset_of = offset_at<tuple_index_v<T, comp_tuple>>;
		*/


		static constexpr std::size_t single_chunk_size = sizeof(components);
	private:
		std::vector<components> chunks{};

		std::mutex staging_mutex_{};
		std::vector<components> staging{};
	protected:

		/*template <typename T, typename Comp>
			requires requires{
				requires std::same_as<std::remove_cvref_t<Comp>, components>;
				requires contained_in<component<T>, components>;
			}
		static decltype(auto) get_comp_of(Comp&& comp) noexcept{
			return std::forward_like<Comp>(std::get<component<T>>(comp));
		}

		template <typename T, typename Comp>
			requires requires{
				requires std::same_as<std::remove_cvref_t<Comp>, components>;
				requires contained_in<component<T>, components>;
			}
		static decltype(auto) get_unwrap_of(Comp&& comp) noexcept{
			return std::forward_like<Comp>(std::get<component<T>>(comp).val());
		}*/

		template <typename ...Ts>
		static decltype(auto) get_unwrap_of(components& comp) noexcept{
			return [&] <std::size_t... I>(std::index_sequence<I...>){
				return std::tuple<decltype(comp.template get<component<Ts>>()) ...>{comp.template get<component<Ts>>() ...};
			}(std::index_sequence_for<Ts ...>{});
		}

		template <typename ...Ts>
		static decltype(auto) get_unwrap_of(const components& comp) noexcept{
			return [&] <std::size_t... I>(std::index_sequence<I...>){
				return std::tuple<decltype(comp.template get<component<Ts>>()) ...>{comp.template get<component<Ts>>() ...};
			}(std::index_sequence_for<Ts ...>{});
		}

		constexpr static entity_id id_of_chunk(const components& comp) noexcept{
			return static_cast<const component<chunk_meta>&>(comp.template get<component<chunk_meta>>()).eid_;
		}

	public:
		void reserve(std::size_t sz) override{
			chunks.reserve(sz);
		}

		std::size_t insert(components&& comp){
			auto idx = chunks.size();
			entity_id eid = this->id_of_chunk(comp);
			eid->chunk_index_ = idx;

			chunks.push_back(std::move(comp));
			archetype_base::insert(eid);
			return idx;
		}

		std::size_t insert(const entity_id e) final {
			if(!e)return 0;

			auto idx = chunks.size();

			if(e->chunk_index() != invalid_chunk_idx){
				throw std::runtime_error{"Duplicated Insert"};
			}

			chunks.emplace_back(e->id());

			e->id()->chunk_index_ = idx;
			archetype_base::insert(e);
			return idx;
		}

		void erase(const entity_id e) final {
			auto chunk_size = chunks.size();
			auto idx = e->chunk_index();

			if(idx >= chunk_size){
				throw std::runtime_error{"Invalid Erase"};
			}

			this->terminate(chunks[idx]);

			if(idx == chunk_size - 1){
				chunks.pop_back();
			}else{
				chunks[idx] = std::move(chunks.back());
				chunks.pop_back();

				this->id_of_chunk(chunks[idx])->chunk_index_ = idx;
			}
			archetype_base::erase(e);
			e->id()->chunk_index_ = invalid_chunk_idx;
		}

		void* get_chunk_ptr(std::type_index type, std::size_t idx) noexcept final {
			if(idx >= chunks.size())return nullptr;
			void* ptr{};
			[&, this] <std::size_t ...Idx>(std::index_sequence<Idx...>){
				([&, this]<std::size_t I>(){
					if(ptr)return;

					using Ty = std::tuple_element_t<I, appended_tuple>;
					if(std::type_index{typeid(Ty)} == type){
						auto chunk = reinterpret_cast<std::byte*>(chunks.data() + idx);
						constexpr auto off = chunk_tuple_offset_at_v<I, comp_tuple>;
						ptr = chunk + off;
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

			using raw_tuple_type = std::tuple<T...>;
			raw_tuple_type ref_tuple{args...};

			using in_tuple = std::tuple<std::decay_t<T>...>;
			constexpr std::size_t in_tuple_size = sizeof...(T);

			e->id()->archetype_ = this;
			components* comps;
			{
				std::lock_guard _{staging_mutex_};
				comps = &[&] <std::size_t ...Idx>(std::index_sequence<Idx...>) -> auto& {
					return staging.emplace_back(e, [&] <std::size_t I> (){
						using cur_type = std::tuple_element_t<I, raw_tuple>;
						constexpr std::size_t mapped_cur_idx = tuple_index_v<cur_type, in_tuple>;

						if constexpr (mapped_cur_idx == in_tuple_size){
							return component<cur_type>{};
						}else{
							using param_type = std::tuple_element_t<mapped_cur_idx, raw_tuple_type>;
							return component<cur_type>{std::forward<param_type>(std::get<mapped_cur_idx>(ref_tuple))};
						}
					}.template operator()<Idx>() ...);
				}(std::make_index_sequence<std::tuple_size_v<raw_tuple>>{});
			}
			this->init(*comps);
		}

		void dump_staging() final {
			auto sz = chunks.size();
			chunks.reserve(sz + staging.size());
			for (auto && [idx, data] : staging | std::views::enumerate){
				this->id_of_chunk(data)->chunk_index_ = sz + idx;
				chunks.push_back(std::move(data));
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
		[[nodiscard]] strided_span<component<T>> slice() noexcept{
			return strided_span<component<T>>{
				reinterpret_cast<component<T>*>(reinterpret_cast<std::byte*>(chunks.data()) +
					chunk_tuple_offset_of_v<component<T>, comp_tuple>),
				chunks.size(), single_chunk_size
			};
		}

		template <typename T>
		[[nodiscard]] strided_span<const component<T>> slice() const noexcept{
			return strided_span<const component<T>>{
				reinterpret_cast<const component<T>*>(reinterpret_cast<const std::byte*>(chunks.data()) +
					chunk_tuple_offset_of_v<component<T>, comp_tuple>),
				chunks.size(), single_chunk_size
			};
		}
	};

	using tuple = std::tuple<void*, double, float>;
	constexpr auto t = sizeof(tuple) - tuple_offset_at_v<1, tuple>;


	std::size_t archetype_base::insert(const entity_id entity){
		entity->id()->archetype_ = this;
		return 0;
	}

	void archetype_base::erase(const entity_id entity){
		entity->id()->archetype_ = nullptr;
	}


	template <typename T>
	component<T>* archetype_base::try_get_comp(const entity_id id) noexcept{
		return try_get_comp<T>(id->chunk_index());
	}

	template <typename Tuple, typename T> requires (contained_in<T, Tuple>)
	component<T>& entity::unchecked_get() const{
		assert(archetype_);
		auto& aty = CHECKED_STATIC_CAST(archetype<Tuple>&)(*archetype_);
		return aty.template slice<T>()[chunk_index_];
	}

	template <typename Tuple>
	unary_apply_to_tuple_t<component, Tuple>& entity::unchecked_get() const{
		assert(archetype_);
		auto& aty = CHECKED_STATIC_CAST(archetype<Tuple>&)(*archetype_);
		return aty[chunk_index_];
	}

	template <typename Tuple, typename T>
	component<T>* entity::get() const noexcept{
		const std::type_index idx = typeid(Tuple);
		if(idx != type())return nullptr;

		return &unchecked_get<Tuple, T>();
	}

	template <typename Tuple>
	unary_apply_to_tuple_t<component, Tuple>* entity::get() const noexcept{
		const std::type_index idx = typeid(Tuple);
		if(idx != type())return nullptr;

		return &unchecked_get<Tuple>();
	}
}

