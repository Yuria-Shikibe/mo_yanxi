module;

#include <plf_hive.h>
#include <cassert>
#include <gch/small_vector.hpp>

#include "../src/ext/adapted_attributes.hpp"

export module mo_yanxi.game.ecs.component_manager;

export import mo_yanxi.game.ecs.entity;
export import mo_yanxi.heterogeneous.open_addr_hash;
export import mo_yanxi.strided_span;

import mo_yanxi.concepts;
import mo_yanxi.meta_programming;
import mo_yanxi.basic_util;

import std;



namespace mo_yanxi::game::ecs{
	export struct component_manager;

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
		// using archetype_map_type = type_fixed_hash_map<std::unique_ptr<archetype_base>>;
		using archetype_map_type = std::unordered_map<std::type_index, std::unique_ptr<archetype_base>>;

	private:
		std::mutex to_destroy_mutex{};

		pointer_hash_map<entity_id, int> to_destroy{};
		std::vector<entity_id> expired{};

		std::mutex entity_mutex{};
		plf::hive<entity> entities{};
		archetype_map_type archetypes{};

		type_fixed_hash_map<std::vector<archetype_slice>> type_to_archetype{};

		// std::vector<entity_id> deferred_entity_add_request{};
	public:
		float update_delta;
		/**
		 * @brief deferred add entity, whose archtype must have been created
		 * @return entity handle
		 */
		template <typename Tuple, typename... Args>
			requires (is_tuple_v<Tuple> && (contained_in<std::decay_t<Args>, Tuple> && ...))
		entity_ref create_entity_deferred(Args&& ...args){
			//TODO add lock?
			std::type_index idx = typeid(Tuple);
			entity* ent;
			{
				std::lock_guard _{entity_mutex};
				ent = std::to_address(entities.emplace(idx));
			}

			static_cast<archetype<Tuple>&>(*archetypes.at(idx)).push_staging(ent->id(), std::forward<Args>(args) ...);

			return ent;
		}

		template <typename Tuple, std::derived_from<archetype<Tuple>> Archetype = archetype<Tuple>>
			requires (is_tuple_v<Tuple>)
		auto& add_archetype(){
			std::type_index idx = typeid(Tuple);

			return static_cast<Archetype&>(*archetypes.try_emplace(idx, create_new_archetype_map<Tuple, Archetype>()).first->second);
		}

		template <typename Archetype>
			requires requires{
				typename Archetype::raw_tuple;
			}
		auto& add_archetype(){
			return add_archetype<typename Archetype::raw_tuple, Archetype>();
		}

		template <typename ...Tuple>
			requires (sizeof...(Tuple) > 1 && (is_tuple_v<Tuple> && ...))
		void add_archetype(){
			(add_archetype<Tuple>(), ...);
		}

		template <typename Tuple>
			requires (is_tuple_v<Tuple>)
		entity_ref create_entity(){
			using ArcheT = archetype<Tuple>;
			std::type_index idx = typeid(Tuple);
			auto& ent = *entities.emplace(idx);

			const auto itr = archetypes.try_emplace(idx, std::make_unique<ArcheT>());
			if(itr.second){
				this->add_new_archetype_map<Tuple>(static_cast<ArcheT&>(*itr.first->second));
			}
			itr.first->second->insert(ent.id());

			return ent;
		}

		template <typename ...Ts>
			requires (!is_tuple_v<Ts> && ...)
		entity_ref create_entity(){
			return create_entity<std::tuple<Ts ...>>();
		}

		bool mark_expired(entity& entity){
			std::lock_guard _{entity_mutex};
			return to_destroy.try_emplace(entity.id()).second;
		}

		void do_deferred(){
			do_deferred_destroy();
			do_deferred_add();
		}

		void do_deferred_add(){
			for (const auto & [type, archetype] : archetypes){
				archetype->dump_staging();
			}
		}

		void do_deferred_destroy(){
			modifiable_erase_if(expired, [this](entity_id e){
				if(e->get_ref_count() == 0){
					//TODO should this always true?
					const auto itr = entities.get_iterator(e);
					if(itr != entities.end())entities.erase(itr);
					return true;
				}

				return false;
			});

			//TODO destroy expiration notify/check
			for (auto&& e : to_destroy | std::views::keys){
				e->get_archetype()->erase(e);

				if(e->get_ref_count() > 0){
					expired.push_back(e);
				}else{
					auto itr = entities.get_iterator(e);
					if(itr != entities.end())entities.erase(itr);
				}
			}
			to_destroy.clear();
		}

		void deferred_destroy_no_reference_entities(){
			for (auto& entity : entities){
				if(entity.get_ref_count() == 0){
					to_destroy.try_emplace(entity.id());
				}
			}
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

			if constexpr (std::same_as<std::remove_cvref_t<std::tuple_element_t<0, raw_params>>, component_manager>){
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
		unary_apply_to_tuple_t<component, Tuple>* get_entity_full_chunk(const entity& entity) const noexcept{
			if(entity.is_expired())return nullptr;
			std::type_index idx = typeid(Tuple);
			if(idx != entity.type())return nullptr;
			//TODO should this a must event? so at() directly?
			archetype<Tuple>& aty = static_cast<archetype<Tuple>&>(entity.get_archetype());
			return &aty[entity.chunk_index()];

			return nullptr;
		}

		template <typename T>
		component<T>* get_entity_partial_chunk(const entity_id entity) const noexcept{
			assert(entity);
			if(entity->is_expired())return nullptr;

			const std::type_index idx = typeid(T);
			if(auto slices_itr = type_to_archetype.find(idx); slices_itr != type_to_archetype.end()){
				auto* archetype = entity->get_archetype();

				if(
					const auto itr = std::ranges::find(slices_itr->second, archetype, &archetype_slice::identity);
					itr != slices_itr->second.end()){
					return &itr->slice<T>()[entity->chunk_index()];
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

			using appended = tuple_cat_t<std::tuple<chunk_meta>, Tuple>;

			[&] <std::size_t ...Idx> (std::index_sequence<Idx...>){
				(insert(typeid(std::tuple_element_t<Idx, appended>), archetype_slice{
					&type,
					+[](void* arg) noexcept -> strided_span<std::byte>{
						return strided_span<std::byte>{static_cast<archetype<Tuple>*>(arg)->template slice<std::tuple_element_t<Idx, appended>>()};
					}
				}), ...);
			}(std::make_index_sequence<std::tuple_size_v<appended>>{});
		}


		template <typename Tuple, std::derived_from<archetype<Tuple>> Archetype = archetype<Tuple>>
		std::unique_ptr<archetype_base> create_new_archetype_map(){
			std::unique_ptr<archetype_base> ptr = std::make_unique<Archetype>();
			this->add_new_archetype_map<Tuple>(static_cast<Archetype&>(*ptr));
			return ptr;
		}

		//TODO delete empty archetype map?
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


namespace mo_yanxi::game::ecs{

}
