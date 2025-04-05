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
	export using entity_id = struct entity*;
	export using entity_data_chunk_index = std::vector<int>::size_type;
	export auto invalid_chunk_idx = std::numeric_limits<entity_data_chunk_index>::max();

	export
	template <typename TupleT>
		requires (std::tuple_size_v<TupleT> > 0)
	struct archetype;

	export
	struct entity{

	private:
		template <typename TupleT>
			requires (std::tuple_size_v<TupleT> > 0)
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
			return chunk_index_;
		}
	};

	export
	template <typename T>
		requires (std::is_default_constructible_v<T>)
	struct component{
	private:
		template <typename TupleT>
			requires (std::tuple_size_v<TupleT> > 0)
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

	template <template <typename > typename W, typename T>
	consteval auto apply_to() noexcept{
		return [] <std::size_t ...Idx>(std::index_sequence<Idx...>){
			return std::tuple<W<std::tuple_element_t<Idx, T>> ...>{};
		}(std::make_index_sequence<std::tuple_size_v<T>>{});
	}

	template <typename T>
	using tuple_to_comp_t = decltype(apply_to_comp<T>());

	template <template <typename > typename W, typename T>
	using tuple_to_comp_container_t = decltype(apply_to_comp_of<W, T>());

	template <typename T>
	using tuple_to_slice_t = decltype(apply_to_slice<T>());

	template <template <typename > typename W, typename T>
	using wrap_tuple_t = decltype(apply_to<W, T>());


	struct archetype_base{
		virtual ~archetype_base() = default;

		virtual std::size_t insert(entity& entity) = 0;

		virtual void erase(entity& entity) = 0;
	};


	template <typename TupleT = std::tuple<int>>
		requires (std::tuple_size_v<TupleT> > 0)
	struct archetype : archetype_base{
		using raw_tuple = TupleT;
		using components = tuple_to_comp_t<raw_tuple>;
	private:
		std::vector<components> chunks{};
	public:

		std::size_t insert(entity& entity) override{
			auto idx = chunks.size();

			if(entity.chunk_index_ != invalid_chunk_idx){
				throw std::runtime_error{"Duplicated Insert"};
			}

			auto fake_get = [&] <std::size_t idx> (std::integral_constant<std::size_t, idx>) -> entity_id{
				return entity.id();
			};

			[&] <std::size_t ...Idx>(std::index_sequence<Idx...>){
				chunks.emplace_back(fake_get(std::integral_constant<std::size_t, Idx>{}) ...);
			}(std::make_index_sequence<std::tuple_size_v<raw_tuple>>{});

			entity.chunk_index_ = idx;
			return idx;
		}

		void erase(entity& entity) override{
			auto chunk_size = chunks.size();
			if(entity.chunk_index_ >= chunk_size){
				throw std::runtime_error{"Invalid Erase"};
			}

			if(entity.chunk_index_ == chunk_size - 1){
				chunks.pop_back();
			}else{
				chunks[entity.chunk_index_] = std::move(chunks.back());
				chunks.pop_back();

				std::get<0>(chunks[entity.chunk_index_]).id()->chunk_index_ = entity.chunk_index_;
			}

			entity.chunk_index_ = invalid_chunk_idx;
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


	using void_hive = archetype<std::tuple<void*>>;

	export
	struct component_manager{
		template <typename T>
		using small_vector_of = gch::small_vector<T>;
		// using archetype_wrapper_type = fixed_size_type_wrapper<alignof(void_hive), sizeof(void_hive)>;
		using archetype_map_type = type_fixed_hash_map<std::unique_ptr<archetype_base>>;

	private:
		std::vector<entity*> to_destroy{};

		plf::hive<entity> entities{};
		archetype_map_type archetypes;

		type_fixed_hash_map<std::vector<archetype_slice>> type_to_archetype;

	public:
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

		void erase_entity(const entity& entity){
			to_destroy.push_back(entity.id());
		}

		void do_destroy(){
			for (auto destroy : to_destroy){
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

			using chunk_spans = wrap_tuple_t<strided_span, wrap_tuple_t<component, Tuple>>;
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
			using params = wrap_tuple_t<std::decay_t, remove_mfptr_this_args<Fn>>;
			using span_tuple = wrap_tuple_t<strided_span, params>;
			this->slice_and_then<wrap_tuple_t<unwrap_first_element_t, params>>([f = std::move(fn)](span_tuple p){
				auto count = std::ranges::size(std::get<0>(p));

				wrap_tuple_t<strided_span_iterator, params> iterators{};

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
			std::type_index idx = typeid(T);
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

		template <
			typename Tuple = std::tuple<>,
			typename Fn,
			std::invocable<std::size_t>	ReserveFn = std::identity>
			requires (is_tuple_v<Tuple>)
		void slice_and_then(Fn fn, ReserveFn reserve_fn = {}){
			// using Tuple = std::tuple<int>;
			static_assert(std::tuple_size_v<Tuple> > 0);
			using chunk_spans = wrap_tuple_t<strided_span, wrap_tuple_t<component, Tuple>>;

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
						if constexpr (std::invocable<Fn, chunk_spans>){
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

		//TODO delete empty archetype map
	};
}
