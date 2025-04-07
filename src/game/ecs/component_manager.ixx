module;

#include <plf_hive.h>
#include <gch/small_vector.hpp>
#include "../src/ext/adapted_attributes.hpp"

export module mo_yanxi.game.ecs.component_manager;
export import mo_yanxi.heterogeneous.open_addr_hash;
export import mo_yanxi.fixed_size_type_storage;
export import mo_yanxi.strided_span;
export import mo_yanxi.concepts;
export import mo_yanxi.meta_programming;
// export import mo_yanxi.small_vector;

import std;

namespace mo_yanxi::game::ecs{
	export struct component_manager;
	export using entity_id = struct entity*;
	export using entity_data_chunk_index = std::vector<int>::size_type;
	export auto invalid_chunk_idx = std::numeric_limits<entity_data_chunk_index>::max();
	export
	template <typename T>
		requires (std::is_default_constructible_v<T>)
	struct component;

	export
	template <typename TupleT>
	struct archetype;

	export
	struct entity{

	private:
		template <typename TupleT>

		friend struct archetype;

		entity_id id_{};
		entity_data_chunk_index chunk_index_{invalid_chunk_idx};
		std::type_index type_{typeid(std::nullptr_t)};

	public:
		[[nodiscard]] entity() = default;

		[[nodiscard]] constexpr explicit(false) entity(std::type_index type)
			noexcept : id_(this), type_(type){
		}

		[[nodiscard]] constexpr entity_id id() const noexcept{
			return id_;
		}

		[[nodiscard]] constexpr bool is_original() const noexcept{
			return id_ == this;
		}

		[[nodiscard]] constexpr std::type_index type() const noexcept{
			return type_;
		}

		[[nodiscard]] constexpr entity_data_chunk_index chunk_index() const noexcept{
			if(is_original())return chunk_index_;

			if(id()) return id()->chunk_index();

			return chunk_index_ != invalid_chunk_idx;
		}

		/**
		 * @brief Only valid when an entity has its data, i.e. an entity has been truly added.
		 */
		explicit operator bool() const noexcept{
			return chunk_index() != invalid_chunk_idx;
		}


		template <typename T>
		component<T>* get(const component_manager& manager) const;
	};

	template <typename T>
		requires (std::is_default_constructible_v<T>)
	struct component{
	private:
		template <typename TupleT>

		friend struct archetype;

		entity_id eid_{};
		T value{};
	public:

		[[nodiscard]] component() = default;

		[[nodiscard]] explicit component(entity_id eid) noexcept
			: eid_(eid){
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
	consteval auto apply_to_comp() noexcept{
		return [] <std::size_t ...Idx>(std::index_sequence<Idx...>){
			return std::tuple<component<std::tuple_element_t<Idx, T>> ...>{};
		}(std::make_index_sequence<std::tuple_size_v<T>>{});
	}

	template <typename T>
	consteval auto apply_to_slice() noexcept{
		return [] <std::size_t ...Idx>(std::index_sequence<Idx...>){
			return std::tuple<strided_span<std::tuple_element_t<Idx, T>> ...>{};
		}(std::make_index_sequence<std::tuple_size_v<T>>{});
	}

	template <template <typename > typename W, typename T>
	consteval auto apply_to_comp_of() noexcept{
		return [] <std::size_t ...Idx>(std::index_sequence<Idx...>){
			return std::tuple<W<component<std::tuple_element_t<Idx, T>>> ...>{};
		}(std::make_index_sequence<std::tuple_size_v<T>>{});
	}


	template <typename T>
	using tuple_to_comp_t = unary_apply_to_tuple_t<component, T>;


	struct archetype_base{
		virtual ~archetype_base() = default;

		virtual std::size_t insert(const entity& entity) = 0;

		virtual void erase(const entity& entity) = 0;
	};

	export
	struct chunk_data_identity{
		std::type_index type_index;
		std::ptrdiff_t offset;

	};

	template <typename TupleT = std::tuple<int>>
	struct archetype : archetype_base{
		using raw_tuple = TupleT;
		using components = tuple_to_comp_t<raw_tuple>;
	private:
		std::vector<components> chunks{};
	public:

		std::size_t insert(const entity& entity) override{
			auto idx = chunks.size();

			if(entity.chunk_index() != invalid_chunk_idx){
				throw std::runtime_error{"Duplicated Insert"};
			}

			[&] <std::size_t ...Idx>(std::index_sequence<Idx...>){
				chunks.emplace_back((Idx, entity.id()) ...);
			}(std::make_index_sequence<std::tuple_size_v<raw_tuple>>{});

			entity.id()->chunk_index_ = idx;
			return idx;
		}

		void erase(const entity& entity) override{
			auto chunk_size = chunks.size();
			auto idx = entity.chunk_index();

			if(idx >= chunk_size){
				throw std::runtime_error{"Invalid Erase"};
			}

			if(idx == chunk_size - 1){
				chunks.pop_back();
			}else{
				chunks[idx] = std::move(chunks.back());
				chunks.pop_back();

				std::get<0>(chunks[idx]).id()->chunk_index_ = idx;
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

		template <typename T>
		[[nodiscard]] strided_span<component<T>> slice() noexcept{
			return strided_span<component<T>>{
				reinterpret_cast<component<T>*>(reinterpret_cast<std::byte*>(chunks.data()) +
					tuple_offset_of_v<component<T>, components>),
				chunks.size(), sizeof(components)
			};
		}

		template <typename T>
		[[nodiscard]] strided_span<const component<T>> slice() const noexcept{
			return strided_span<const component<T>>{
				reinterpret_cast<const component<T>*>(reinterpret_cast<const std::byte*>(chunks.data()) +
					tuple_offset_of_v<component<T>, components>),
				chunks.size(), sizeof(components)
			};
		}
	};

	export
	struct archetype_slice{
		using acquirer_type = std::add_pointer_t<strided_span<std::byte>(void*) noexcept>;
	private:
		void* idt{};
		acquirer_type getter{};

	public:
		[[nodiscard]] archetype_slice(void* identity, acquirer_type getter)
			: idt(identity),
			  getter(getter){
		}

		[[nodiscard]] constexpr void* identity() const noexcept{
			return idt;
		}

		[[nodiscard]] constexpr acquirer_type get_slice_generator() const noexcept{
			return getter;
		}

		// using T = int;
		template <typename T>
		[[nodiscard]] constexpr strided_span<component<T>> slice() const noexcept {
			auto span = getter(idt);
			return strided_span<component<T>>{reinterpret_cast<component<T>*>(span.data()), span.size() / sizeof (component<T>), span.stride()};
		}

		constexpr auto operator<=>(const archetype_slice& o) const noexcept{
			return std::compare_three_way{}(idt, o.idt);
		}

		constexpr bool operator==(const archetype_slice& o) const noexcept{
			return idt == o.idt;
		}
	};

	struct component_manager{
		template <typename T>
		using small_vector_of = gch::small_vector<T>;
		using archetype_map_type = type_fixed_hash_map<std::unique_ptr<archetype_base>>;

	private:
		pointer_hash_map<entity*, int> to_destroy{};

		plf::hive<entity> entities{};
		archetype_map_type archetypes{};

		type_fixed_hash_map<std::vector<archetype_slice>> type_to_archetype{};

		std::vector<entity> deferred_entity_add_request{};
	public:
		/**
		 * @brief deferred add entity, whose archtype must have been created
		 * @return entity handle
		 */
		template <typename Tuple>
			requires (is_tuple_v<Tuple>)
		entity create_entity_deferred(){
			std::type_index idx = typeid(Tuple);
			auto& ent = *entities.emplace(idx);

			deferred_entity_add_request.push_back(ent);

			return ent;
		}


		template <typename Tuple>
			requires (is_tuple_v<Tuple>)
		auto add_archetype(){
			std::type_index idx = typeid(Tuple);

			return archetypes.try_emplace(idx, create_new_archetype_map<Tuple>());
		}
		template <typename ...Tuple>
			requires (sizeof...(Tuple) > 1)
		void add_archetype(){
			(add_archetype<Tuple>(), ...);
		}

		template <typename Tuple>
			requires (is_tuple_v<Tuple>)
		entity create_entity(){
			using ArcheT = archetype<Tuple>;
			std::type_index idx = typeid(Tuple);
			auto& ent = *entities.emplace(idx);

			const auto itr = archetypes.try_emplace(idx, std::make_unique<ArcheT>());
			if(itr.second){
				this->add_new_archetype_map<Tuple>(static_cast<ArcheT&>(*itr.first->second));
			}
			itr.first->second->insert(ent);

			return ent;
		}

		template <typename ...Ts>
			requires (!is_tuple_v<Ts> && ...)
		entity create_entity(){
			return create_entity<std::tuple<Ts ...>>();
		}

		bool erase_entity(const entity& entity){
			return to_destroy.try_emplace(entity.id()).second;
		}

		void do_deferred(){
			do_deferred_destroy();
			do_deferred_add();
		}

		void do_deferred_add(){
			for (const auto& entity_add_request : deferred_entity_add_request){
				archetypes.at(entity_add_request.type())->insert(entity_add_request);
			}
			deferred_entity_add_request.clear();
		}

		void do_deferred_destroy(){
			//TODO destroy expiration notify/check
			for (auto&& destroy : to_destroy | std::views::keys){
				if(auto map = archetypes.find(destroy->type()); map != archetypes.end()){
					map->second->erase(*destroy);
				}

				auto itr = entities.get_iterator(destroy);
				if(itr != entities.end())entities.erase(itr);
			}
			to_destroy.clear();
		}

		template <typename Tuple>
			requires (is_tuple_v<Tuple>)
		auto get_slice_of(){
			static_assert(std::tuple_size_v<Tuple> > 0);

			using chunk_spans = unary_apply_to_tuple_t<strided_span, unary_apply_to_tuple_t<component, Tuple>>;
			using searched_spans = small_vector_of<chunk_spans>;

			searched_spans result{};

			this->slice_and_then<Tuple>([&](chunk_spans spans){
				result.push_back(spans);
			}, [&](std::size_t sz){
				result.reserve(sz);
			});

			return result;
		}

		template <typename Fn>
		FORCE_INLINE void sliced_each(Fn fn){
			using raw_params = remove_mfptr_this_args<Fn>;

			if constexpr (std::same_as<std::tuple_element_t<0, raw_params>, component_manager&>){
				using params = unary_apply_to_tuple_t<std::decay_t, tuple_drop_first_elem_t<raw_params>>;
				using span_tuple = unary_apply_to_tuple_t<strided_span, params>;

				this->slice_and_then<unary_apply_to_tuple_t<unwrap_first_element_t, params>>([this, f = std::move(fn)](const span_tuple& p){
					auto count = std::ranges::size(std::get<0>(p));

					unary_apply_to_tuple_t<strided_span_iterator, params> iterators{};

					[&] <std::size_t ...Idx> (std::index_sequence<Idx...>){
						((std::get<Idx>(iterators) = std::ranges::begin(std::get<Idx>(p))), ...);
					}(std::make_index_sequence<std::tuple_size_v<params>>{});

					for(std::remove_cvref_t<decltype(count)> i = 0; i != count; ++i){
						[&, this] <std::size_t ...Idx> (std::index_sequence<Idx...>){
							std::invoke(f, *this, *(std::get<Idx>(iterators)++) ...);
						}(std::make_index_sequence<std::tuple_size_v<params>>{});
					}
				});
			}else{
				using params = unary_apply_to_tuple_t<std::decay_t, raw_params>;
				using span_tuple = unary_apply_to_tuple_t<strided_span, params>;

				this->slice_and_then<unary_apply_to_tuple_t<unwrap_first_element_t, params>>([f = std::move(fn)](const span_tuple& p){
					auto count = std::ranges::size(std::get<0>(p));

					unary_apply_to_tuple_t<strided_span_iterator, params> iterators{};

					[&] <std::size_t ...Idx> (std::index_sequence<Idx...>){
						((std::get<Idx>(iterators) = std::ranges::begin(std::get<Idx>(p))), ...);
					}(std::make_index_sequence<std::tuple_size_v<params>>{});

					for(std::remove_cvref_t<decltype(count)> i = 0; i != count; ++i){
						[&] <std::size_t ...Idx> (std::index_sequence<Idx...>){
							std::invoke(f, *(std::get<Idx>(iterators)++) ...);
						}(std::make_index_sequence<std::tuple_size_v<params>>{});
					}
				});
			}

		}

		template <typename ...T>
			requires (!is_tuple_v<T> && ...)
		auto get_slice_of(){
			return get_slice_of<std::tuple<T...>>();
		}



		template <typename Tuple = std::tuple<int>>
		tuple_to_comp_t<Tuple>* get_entity_chunk(const entity& entity) const noexcept{
			std::type_index idx = typeid(Tuple);
			if(idx != entity.type())return nullptr;
			//TODO shuold this a must event? so at() directly?
			archetype<Tuple>& aty = static_cast<archetype<Tuple>&>(archetypes.at(idx).operator*());
			return &aty[entity.chunk_index()];

			return nullptr;
		}

		template <typename T>
		component<T>* get_entity_partial_chunk(const entity& entity) const noexcept{
			const std::type_index idx = typeid(T);
			if(auto slices_itr = type_to_archetype.find(idx); slices_itr != type_to_archetype.end()){
				auto* archetype = archetypes.at(entity.type()).get();

				if(
					const auto itr = std::ranges::lower_bound(slices_itr->second, archetype, {}, &archetype_slice::identity);
					itr != slices_itr->second.end()){
					if(archetype == itr->identity()){
						return &itr->slice<T>()[entity.chunk_index()];
					}
				}
			}

			return nullptr;
		}

	private:
		struct subrange{
			using value_type = const archetype_slice;
			using rng = std::span<value_type>;
			using itr = rng::iterator;

		// private:
			itr begin_itr;
			itr end_itr;
		public:

			[[nodiscard]] constexpr subrange() noexcept = default;

			[[nodiscard]] constexpr explicit(false) subrange(rng range) noexcept : begin_itr(range.begin()),
				end_itr(range.end()){
			}

			constexpr bool empty() const noexcept{
				return begin_itr == end_itr;
			}

			value_type& front() const noexcept{
				return *begin_itr;
			}


			itr begin() const noexcept{
				return begin_itr;
			}

			itr end() const noexcept{
				return begin_itr;
			}
		};

	public:
		template <
			typename Tuple = std::tuple<>,
			typename Fn,
			std::invocable<std::size_t>	ReserveFn = std::identity>
			requires (is_tuple_v<Tuple>)
		void slice_and_then(Fn fn, ReserveFn reserve_fn = {}){
			// using Tuple = std::tuple<int>;
			static_assert(std::tuple_size_v<Tuple> > 0);
			using chunk_spans = unary_apply_to_tuple_t<strided_span, unary_apply_to_tuple_t<component, Tuple>>;

			std::array<std::span<const archetype_slice>, std::tuple_size_v<Tuple>> spans{};

			std::size_t maximum_size{};
			{
				bool none = false;
				auto try_find = [&, this](std::type_index idx) -> std::span<const archetype_slice>{
					if(none)return {};

					if(auto itr = type_to_archetype.find(idx); itr != type_to_archetype.end()){
						maximum_size = std::max(maximum_size, itr->second.size());
						return itr->second;
					}

					none = true;
					return {};
				};

				[&] <std::size_t ...Idx> (std::index_sequence<Idx...>){
					((spans[Idx] = try_find(typeid(std::tuple_element_t<Idx, Tuple>))), ...);
				}(std::make_index_sequence<std::tuple_size_v<Tuple>>{});

				if(none)return;
			}

			std::array<subrange, std::tuple_size_v<Tuple>> ranges{};

			static constexpr auto comp = std::less<void>{};

			auto cur_itr_min{spans.front().front().identity()};
			for(std::size_t i = 0; i < std::tuple_size_v<Tuple>; ++i){
				ranges[i] = spans[i];
				cur_itr_min = std::max(cur_itr_min, ranges[i].front().identity(), comp);
			}

			(void)std::invoke(reserve_fn, maximum_size);
			while(true){
				auto last_itr_min = cur_itr_min;
				for (auto&& rng : ranges){
					//TODO will binary search(lower bound) faster?
					if(comp(rng.front().identity(), last_itr_min)){
						++rng.begin_itr;

						if(rng.empty()){
							return;
						}

					}

					cur_itr_min = std::max(cur_itr_min, rng.front().identity(), comp);
				}

				if(last_itr_min == cur_itr_min){
					[&] <std::size_t ...Idx> (std::index_sequence<Idx...>){
						if constexpr (std::invocable<Fn, std::array<archetype_slice, std::tuple_size_v<Tuple>>>){
							std::invoke(fn, std::array<archetype_slice, std::tuple_size_v<Tuple>>{static_cast<subrange&>(ranges[Idx]).front() ...});
						}else if constexpr (std::invocable<Fn, chunk_spans>){
							std::invoke(fn, std::make_tuple(static_cast<subrange&>(ranges[Idx]).front().slice<std::tuple_element_t<Idx, Tuple>>() ...));
						}else{
							std::invoke(fn, static_cast<subrange&>(ranges[Idx]).front().slice<std::tuple_element_t<Idx, Tuple>>() ...);
						}
					}(std::make_index_sequence<std::tuple_size_v<Tuple>>{});

					for (auto&& archetype_slice : ranges){
						++archetype_slice.begin_itr;
						if(archetype_slice.empty()){
							return;
						}
					}

					cur_itr_min = ranges.back().front().identity();
				}
			}
		}

	private:
		template <typename Tuple = std::tuple<>>
		void add_new_archetype_map(archetype<Tuple>& type){
			auto insert = [this](std::type_index idx, archetype_slice&& elem){
				auto& vector = type_to_archetype[idx];
				auto it = std::ranges::lower_bound(vector, elem);
				vector.insert(it, std::move(elem));
			};

			[&] <std::size_t ...Idx> (std::index_sequence<Idx...>){
				(insert(typeid(std::tuple_element_t<Idx, Tuple>), archetype_slice{
					&type,
					+[](void* arg) noexcept -> strided_span<std::byte>{
						return strided_span<std::byte>{static_cast<archetype<Tuple>*>(arg)->template slice<std::tuple_element_t<Idx, Tuple>>()};
					}
				}), ...);
			}(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
		}


		template <typename Tuple>
		std::unique_ptr<archetype_base> create_new_archetype_map(){
			using ArcheT = archetype<Tuple>;
			std::unique_ptr<archetype_base> ptr = std::make_unique<ArcheT>();
			this->add_new_archetype_map<Tuple>(static_cast<ArcheT&>(*ptr));
			return ptr;
		}

		//TODO delete empty archetype map
	};

	export
	template <typename T>
	struct readonly_decay : std::type_identity<std::add_const_t<std::decay_t<T>>>{};

	export
	template <typename T>
	struct readonly_decay<const T&> : readonly_decay<T>{};

	export
	template <typename T>
	struct readonly_decay<const volatile T&> : readonly_decay<T>{};

	template <typename T>
	struct readonly_decay<T&> : std::type_identity<std::decay_t<T>>{};

	template <typename T>
	struct readonly_decay<volatile T&> : std::type_identity<std::decay_t<T>>{};

	export
	template <typename T>
	using readonly_const_decay_t = typename readonly_decay<T>::type;

	template <typename T>
	struct is_component : std::false_type{};

	template <typename T>
	struct is_component<component<T>> : std::true_type{};

	export
	template <typename T>
	constexpr bool is_component_v = is_component<T>::value;

	export
	template <typename Tuple>
	using get_used_components = tuple_drop_front_n_elem_t<tuple_find_first_v<is_component, unary_apply_to_tuple_t<std::decay_t, Tuple>>, Tuple>;

}

module : private;

namespace mo_yanxi::game::ecs{
	template <typename T>
	component<T>* entity::get(const component_manager& manager) const{
		return manager.get_entity_partial_chunk<T>(*this);
	}
}