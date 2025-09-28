module;

#include <cassert>
#include "plf_hive.h"
#include "gch/small_vector.hpp"

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

export module mo_yanxi.game.ecs.component.manage:entity;

import :serializer;
export import mo_yanxi.seq_chunk;
import mo_yanxi.strided_span;

import mo_yanxi.game.srl;

import mo_yanxi.algo.hash;
import mo_yanxi.basic_util;
import mo_yanxi.concepts;
import mo_yanxi.meta_programming;
import mo_yanxi.heterogeneous.open_addr_hash;

import mo_yanxi.type_register;

import std;

import :misc;

namespace mo_yanxi::game::ecs{
	export struct bad_chunk_access : std::exception{
		[[nodiscard]] bad_chunk_access() = default;

		[[nodiscard]] explicit bad_chunk_access(char const* Message)
			: exception(Message){
		}
	};


	export using entity_data_chunk_index = std::vector<int>::size_type;
	export auto invalid_chunk_idx = std::numeric_limits<entity_data_chunk_index>::max();

	export struct component_manager;

	export
	template <typename TupleT>
	struct archetype;

	export
	struct entity_ref;

	export struct entity{

	private:
		friend component_manager;
		template <typename TupleT>
		friend struct archetype;
		friend struct archetype_base;
		friend struct entity_ref;

		// entity_id id_{};
		std::atomic_size_t referenced_count{};
		entity_data_chunk_index chunk_index_{invalid_chunk_idx};
		unsigned expire_staging_counter_{};

		type_identity_index type_{};

		archetype_base* archetype_{};

	public:
		[[nodiscard]] entity() = default;

		[[nodiscard]] constexpr explicit(false) entity(type_identity_index type)
			noexcept : type_(type){
		}

		[[nodiscard]] std::size_t get_ref_count() const noexcept{
			return referenced_count.load(std::memory_order_relaxed);
		}

		[[nodiscard]] constexpr entity_id id() noexcept{
			return this;
		}

		[[nodiscard]] constexpr type_identity_index type() const noexcept{
			return type_;
		}

		[[nodiscard]] constexpr entity_data_chunk_index chunk_index() const noexcept{
			return chunk_index_;
		}



		/**
		 * @brief Only valid when an entity has its data, i.e. an entity has been truly added.
		 */
		explicit operator bool() const noexcept{
			return is_inserted();
		}

		[[nodiscard]] constexpr bool is_expired() const noexcept{
			return get_archetype() == nullptr;
		}

		[[nodiscard]] constexpr bool is_inserted() const noexcept{
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
		[[nodiscard]] bool has() const noexcept{
			assert(archetype_);
			return archetype_->has_type<T>();
		}

		template <typename T>
		[[nodiscard]] T& at() const noexcept{
#ifdef COMP_AT_CHECK

			if(!archetype_){
				std::println(std::cerr, "[FATAL ERROR] Illegal Access To Chunk<{}> on entity<{}> on empty archetype", name_of<T>(), type()->name());
				std::terminate();
			}

			if(!is_inserted()){
				std::println(std::cerr, "[FATAL ERROR] Illegal Access To Chunk<{}> on entity<{}> before insertion", name_of<T>(), type()->name());
				std::terminate();
			}
#endif

			T* ptr = archetype_->try_get_comp<T>(chunk_index_);

#ifdef COMP_AT_CHECK
			if(!ptr){
				std::println(std::cerr, "[FATAL ERROR] Illegal Access To Chunk<{}> on entity<{}> at[{}]", name_of<T>(), type()->name(), chunk_index());
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
			using class_type = trait::class_type;

			return [&, this]<std::size_t ...Idx>(std::index_sequence<Idx...>){
				return [mfptr, &object = this->at<class_type>()](std::tuple_element_t<Idx, typename trait::func_args> ...args) -> decltype(auto) {
					return std::invoke(mfptr, object, std::forward<decltype(args)>(args)...);
				};
			}(std::make_index_sequence<std::tuple_size_v<typename trait::func_args>>{});
		}
	};


	export
	template <entity_component_seq EntityChunkDesc, typename ChunkPartial>
		requires requires{
		requires contained_in<std::remove_cvref_t<ChunkPartial>, EntityChunkDesc>;
		}
	[[nodiscard]] constexpr auto chunk_of(ChunkPartial& value) noexcept -> decltype(*mo_yanxi::seq_chunk_cast<tuple_to_seq_chunk_t<EntityChunkDesc>>(&value)) {
		using Tup = tuple_to_seq_chunk_t<EntityChunkDesc>;
		decltype(auto) rst = mo_yanxi::seq_chunk_cast<Tup>(&value);
#if DEBUG_CHECK
		auto get_meta = [&]<typename C>() -> const chunk_meta& {
			return mo_yanxi::neighbour_of<chunk_meta, C>(value);
		};

		const entity_id eid = get_meta.template operator()<Tup>().id();
		assert(eid->get<EntityChunkDesc>() == rst);
#endif
		return *rst;
	}

	export
	template <typename Tgt, entity_component_seq EntityChunkDesc, typename ChunkPartial>
		requires requires{
		requires contained_in<Tgt, EntityChunkDesc>;
		requires contained_in<std::remove_cvref_t<ChunkPartial>, EntityChunkDesc>;
		}
	[[nodiscard]] constexpr decltype(auto) chunk_neighbour_of(ChunkPartial& value) noexcept{
		return get<Tgt>(ecs::chunk_of<EntityChunkDesc>(value));
	}


	template <typename TupleT>
	struct archetype : archetype_base{
		using raw_tuple = TupleT;
		// using raw_tuple = std::tuple<chunk_meta>;

		using trait = archetype_trait<raw_tuple>;
		using appended_tuple = raw_tuple;
		using components = tuple_to_seq_chunk_t<appended_tuple>;
		static constexpr std::size_t chunk_comp_count = std::tuple_size_v<appended_tuple>;

		static constexpr std::size_t single_chunk_size = sizeof(components);

		using base_to_derive_map = all_apply_to<tuple_cat_t, unary_apply_to_tuple_t<derive_map_of_trait, appended_tuple>>;
		static constexpr std::size_t type_hash_map_size = std::tuple_size_v<base_to_derive_map>;

		struct serializer : archetype_serializer{
			std::vector<typename trait::dump_chunk> buffer{};

			[[nodiscard]] archetype_serialize_identity get_identity() const noexcept final{
				return trait::serialize::identity;
			}


			void clear() noexcept final{
				buffer.clear();
			}

			void dump(const archetype& ty){
				auto view = ty.get_chunk_view();
				buffer.reserve(buffer.size() + view.size());

				for (const auto & seq_chunk : view){
					buffer.push_back(trait::behavior::dump(seq_chunk));
				}
			}

			void write(std::ostream& stream) const final{
				std::uint32_t total_count = buffer.size();
				swapbyte_if_needed(total_count);

				try{
					stream.write(reinterpret_cast<char*>(&total_count), sizeof(total_count));
					for (const auto & seq_chunk : buffer){
						srl::chunk_serialize_handle hdl = trait::serialize::write(stream, seq_chunk);
						srl::srl_size off = hdl.get_offset();
						swapbyte_if_needed(off);
						stream.write(reinterpret_cast<char*>(&off), sizeof(off));

						hdl.resume();

						// if(hdl.get_state() != srl_state::succeed){
						// 	throw bad_archetype_serialize{"assertion failed: unfinished chunk serialize"};
						// }
					}
				}catch(const srl::srl_logical_error&){
					throw;
				}catch(...){
					throw srl::srl_logical_error{"serialize failed"};
				}


				//todo insert a check hash?
			}

			void read(std::istream& stream) final{
				std::uint32_t total_count;
				stream.read(reinterpret_cast<char*>(&total_count), sizeof(total_count));

				swapbyte_if_needed(total_count);

				buffer.resize(total_count);
				auto current = buffer.begin();
				std::ranges::range_size_t<decltype(buffer)> total_try_count{};

				while(total_try_count != total_count){
					auto last = stream.tellg();

					component_chunk_offset off;
					if(!stream.read(reinterpret_cast<char*>(&off), sizeof(off))){
						throw srl::srl_logical_error{"assertion failed: unfinished chunk serialize"};
					}
					swapbyte_if_needed(off);

					try{
						trait::serialize::read(stream, off, *current);
						++current;
					}catch(...){
						//TODO logit
						stream.seekg(last + static_cast<std::streamoff>(off));
					}

					++total_try_count;
				}

				buffer.erase(current, buffer.end());
			}

			void load(component_manager& target) const final;
		};

		using hashder_array_type = std::array<
				type_index_to_convertor,
				std::bit_ceil(type_hash_map_size * 2)>;

		//believe that this is faster than static
		const hashder_array_type type_convertor_hash_map = [] {
			hashder_array_type hash_array{};

			std::array<type_index_to_convertor, type_hash_map_size> temp{};
			[&] <std::size_t ...Idx>(std::index_sequence<Idx...>){
				((temp[Idx] = type_index_to_convertor{
					unstable_type_identity_of<typename std::tuple_element_t<Idx, base_to_derive_map>::first_type>(),
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
		plf::hive<components> staging{};

		// std::mutex staging_expire_mutex_{};
		// std::vector<entity_id> staging_expires{};

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
		[[nodiscard]] std::span<const components> get_chunk_view() const noexcept{
			return chunks;
		}


		[[nodiscard]] std::size_t size() const noexcept final{
			return chunks.size();
		}

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
			auto last_cap = chunks.capacity();
			components& added_comp = chunks.emplace_back(std::move(comp));
			archetype_base::insert(eid);


			[&] <std::size_t... I>(std::index_sequence<I...>){
				(component_trait<std::tuple_element_t<I, raw_tuple>>::on_init(get<0>(added_comp), get<I>(added_comp)), ...);
			}(std::make_index_sequence<std::tuple_size_v<raw_tuple>>());

			trait::on_init(added_comp);
			eid->expire_staging_counter_ = trait::expire_counter;
			this->init(added_comp);

			if(last_cap != chunks.capacity()){
				for(components& prev_comp : chunks | std::views::reverse | std::views::drop(1)){
					[&] <std::size_t... I>(std::index_sequence<I...>){
						(component_trait<std::tuple_element_t<I, raw_tuple>>::on_relocate(get<0>(prev_comp), get<I>(prev_comp)), ...);
					}(std::make_index_sequence<std::tuple_size_v<raw_tuple>>());
				}
			}

			return idx;
		}

	public:
		std::size_t insert(const entity_id eid) final {
			if(!eid)return 0;

			if constexpr (requires{
				components{eid};
			}){
				return this->insert<true>(components{eid});
			}else if constexpr (std::is_default_constructible_v<components>){
				components comps{};
				chunk_meta& meta = get<chunk_meta>(comps);
				meta.eid_ = eid;
				return this->insert<true>(std::move(comps));
			}else{
				static_assert(false, "failed to construct components");
			}

		}

		void erase(const entity_id e) final {
			auto chunk_size = chunks.size();
			auto idx = e->chunk_index();

			if(idx >= chunk_size){
				throw std::runtime_error{"Invalid Erase"};
			}

			components& chunk = chunks[idx];

			this->terminate(chunk);
			trait::on_terminate(chunk);

			[&] <std::size_t... I>(std::index_sequence<I...>){
				(component_trait<std::tuple_element_t<I, raw_tuple>>::on_terminate(get<0>(chunk), get<I>(chunk)), ...);
			}(std::make_index_sequence<std::tuple_size_v<raw_tuple>>());

			if(idx == chunk_size - 1){
				chunks.pop_back();
			}else{
				chunk = std::move(chunks.back());
				chunks.pop_back();

				this->id_of_chunk(chunk)->chunk_index_ = idx;

				[&] <std::size_t... I>(std::index_sequence<I...>){
					(component_trait<std::tuple_element_t<I, raw_tuple>>::on_relocate(get<0>(chunk), get<I>(chunk)), ...);
				}(std::make_index_sequence<std::tuple_size_v<raw_tuple>>());

				trait::on_relocate(chunk);
			}


			archetype_base::erase(e);
		}

		[[nodiscard]] bool has_type(type_identity_index type) const final{
			return algo::access_hash(type_convertor_hash_map, type, &type_index_to_convertor::type, {}, [](const type_index_to_convertor& conv){
				return !conv.convertor;
			}) != type_convertor_hash_map.end();
		}

		void* get_chunk_partial_ptr(type_identity_index type, std::size_t idx) noexcept final {
			if(auto itr = algo::access_hash(type_convertor_hash_map, type, &type_index_to_convertor::type, {}, [](const type_index_to_convertor& conv){
				return !conv.convertor;
			}); itr != type_convertor_hash_map.end()){
				return itr->convertor.get(chunks[idx]);
			}else{
				return nullptr;
			}
		}

		strided_span<std::byte> get_staging_chunk_partial_slice(type_identity_index type) noexcept final{
			if(const auto itr = algo::access_hash(type_convertor_hash_map, type, &type_index_to_convertor::type, {}, [](const type_index_to_convertor& conv){
				return !conv.convertor;
			}); itr != type_convertor_hash_map.end()){
				if(chunks.empty()){
					return {};
				}else{
					return {static_cast<std::byte*>(itr->convertor.get(chunks.data())), chunks.size(), single_chunk_size};
				}

			}else{
				return {};
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

		components& push_staging(const entity_id e, components&& comp){
			assert(e);
			e->id()->archetype_ = this;
			chunk_meta& meta = get<chunk_meta>(comp);
			meta.eid_ = e;

			std::lock_guard _{staging_mutex_};
			return *staging.insert(std::move(comp));
		}

		void dump_staging() final {
			auto sz = chunks.size() + staging.size();
			if(sz > chunks.capacity()){
				chunks.reserve(sz);
				for(components& prev_comp : chunks){
					[&] <std::size_t... I>(std::index_sequence<I...>){
						(component_trait<std::tuple_element_t<I, raw_tuple>>::on_relocate(get<0>(prev_comp), get<I>(prev_comp)), ...);
					}(std::make_index_sequence<std::tuple_size_v<raw_tuple>>());
				}
			}

			for (auto && data : staging){
				this->insert<false>(std::move(data));
			}

			staging.clear();

			if(staging.capacity() > 128){
				staging.trim_capacity(32);
			}
		}

		[[nodiscard]] std::unique_ptr<archetype_serializer> dump() const final{
			if constexpr (trait::is_transient){
				return {};
			}else{
				std::unique_ptr<serializer> ret{std::make_unique<serializer>()};
				ret->dump(*this);
				return ret;
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
		static constexpr type_identity_index idx = unstable_type_identity_of<Tuple>();
		if(idx != type())return nullptr;

		return &unchecked_get<Tuple, T>();
	}

	template <typename Tuple>
	tuple_to_comp_t<Tuple>* entity::get() const noexcept{
		static constexpr type_identity_index idx = unstable_type_identity_of<Tuple>();
		if(idx != type())return nullptr;

		return &unchecked_get<Tuple>();
	}

	export
	struct archetype_slice{
		using acquirer_type = std::add_pointer_t<strided_span<std::byte>(archetype_base*) noexcept>;
	private:
		archetype_base* idt{};
		acquirer_type getter{};

	public:
		[[nodiscard]] constexpr archetype_slice(archetype_base* identity, acquirer_type getter)
			: idt(identity),
			  getter(getter){
		}

		[[nodiscard]] constexpr archetype_base* identity() const noexcept{
			return idt;
		}

		[[nodiscard]] constexpr acquirer_type get_slice_generator() const noexcept{
			return getter;
		}

		// using T = int;
		template <typename T>
		[[nodiscard]] constexpr strided_span<T> slice() const noexcept {
			auto span = getter(idt);
			return strided_span<T>{reinterpret_cast<T*>(span.data()), span.size(), span.stride()};
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
		// using archetype_map_type = fixed_open_hash_map<type_identity_index, std::unique_ptr<archetype_base>>;
		using archetype_map_type = std::unordered_map<type_identity_index, std::unique_ptr<archetype_base>>;

	private:
		//manager should always moved thread safely
		no_propagation_mutex<> entity_expire_mutex_{};
		no_propagation_mutex<> entity_acquire_mutex_{};
		no_propagation_mutex<> entity_staging_add_mutex_{};

		template <typename Tup>
		struct staging_add final : staging_add_base{
			tuple_to_comp_t<Tup> comp{};

			[[nodiscard]] explicit(false) staging_add(tuple_to_comp_t<Tup>&& comp)
				: comp(std::move(comp)){
			}

			type_identity_index get_idx() const noexcept override final{
				return unstable_type_identity_of<Tup>();
			}

			void add_archetype(component_manager& manager) const noexcept override{
				manager.add_archetype<Tup>();
			}

			void push_entity(component_manager& manager) noexcept override{
				auto& a = manager.archetypes.at(unstable_type_identity_of<Tup>());
				static_cast<archetype<Tup>&>(*a).push_staging(static_cast<const chunk_meta&>(get<chunk_meta>(comp)).id(), std::move(comp));
			}
		};

		pointer_hash_map<entity_id, int> to_expire{};
		std::vector<entity_id> expired{};

		plf::hive<entity> entities{};
		archetype_map_type archetypes{};


		//TODO make all archetype slices on the same vector, while hash map provides type to span
		std::vector<archetype_slice> archetype_slices_{};
		fixed_open_hash_map<type_identity_index, std::span<const archetype_slice>> type_to_archetype{};
		// std::unordered_map<type_identity_index, std::span<const archetype_slice>> type_to_archetype{};

		std::vector<std::unique_ptr<staging_add_base>> staging_adds{};

		float update_delta{};
		unsigned clock{};

	public:
		void update_update_delta(float dlt) noexcept{
			update_delta = dlt;
			++clock;
		}

		auto get_archetypes() const noexcept{
			return archetypes | std::views::values;
		}

		[[nodiscard]] FORCE_INLINE constexpr float get_update_delta() const noexcept{
			return update_delta;
		}


		[[nodiscard]] FORCE_INLINE constexpr unsigned get_clock() const noexcept{
			return clock;
		}

		/**
		 * @brief deferred add entity, whose archtype must have been created
		 * @return entity handle
		 */
		template <entity_component_seq Tuple, typename... Args>
			requires (is_tuple_v<Tuple> && (contained_in<std::decay_t<Args>, Tuple> && ...))
		auto& create_entity_deferred(Args&& ...args){
			using raw_tuple_type = std::tuple<Args&& ...>;
			raw_tuple_type ref_tuple{std::forward<Args>(args)...};

			using in_tuple = std::tuple<std::decay_t<Args>...>;
			constexpr std::size_t in_tuple_size = sizeof...(Args);

			return [&] <std::size_t ...Idx>(std::index_sequence<Idx...>) -> auto& {
				return this->create_entity_deferred<Tuple>(tuple_to_comp_t<Tuple>{[&] <std::size_t I> (){
					using cur_type = std::tuple_element_t<I, Tuple>;
					constexpr std::size_t mapped_cur_idx = tuple_index_v<cur_type, in_tuple>;

					if constexpr (mapped_cur_idx == in_tuple_size){
						return cur_type{};
					}else{
						using param_type = std::tuple_element_t<mapped_cur_idx, raw_tuple_type>;
						return cur_type{std::forward<param_type>(std::get<mapped_cur_idx>(ref_tuple))};
					}
				}.template operator()<Idx>() ...});
			}(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
		}

		/**
		 * @brief deferred add entity, whose archtype must have been created
		 * @return entity handle
		 */
		template <typename Tuple>
		auto& create_entity_deferred(tuple_to_comp_t<Tuple>&& comps){
			static constexpr type_identity_index idx = unstable_type_identity_of<Tuple>();

			entity* ent = acquire_entity<Tuple>();

			if(auto itr = archetypes.find(idx); itr != archetypes.end()){
				auto& a = static_cast<archetype<Tuple>&>(*itr->second);
				return a.push_staging(ent->id(), std::move(comps));
			}else{
				chunk_meta& meta = get<chunk_meta>(comps);
				meta.eid_ = ent;
				auto ptr = std::make_unique<staging_add<Tuple>>(std::move(comps));
				auto& comp = ptr->comp;

				{
					std::lock_guard _{entity_staging_add_mutex_};
					staging_adds.push_back(std::move(ptr));
				}

				return comp;
			}

		}

		template <typename Tuple, std::derived_from<archetype<Tuple>> Archetype = archetype<Tuple>, typename ... Args>
			requires (is_tuple_v<Tuple>)
		auto& add_archetype(Args&& ...args){
			static constexpr type_identity_index idx = unstable_type_identity_of<Tuple>();
			if(const auto itr = archetypes.find(idx); itr != archetypes.end()){
				return static_cast<Archetype&>(*itr->second);
			}

			return static_cast<Archetype&>(*archetypes.try_emplace(idx, this->create_new_archetype_map<Tuple, Archetype>(std::forward<Args>(args) ...)).first->second);
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
		entity_id acquire_entity(){
			static constexpr type_identity_index idx = unstable_type_identity_of<Tuple>();

			std::lock_guard _{entity_acquire_mutex_};
			return std::to_address(entities.emplace(idx));
		}

	private:
		template <typename Tuple>
			requires (is_tuple_v<Tuple>)
		entity_id create_entity(){
			using ArcheT = archetype<Tuple>;
			static constexpr type_identity_index idx = unstable_type_identity_of<Tuple>();
			auto& ent = *entities.emplace(idx);

			const auto itr = archetypes.try_emplace(idx, std::make_unique<ArcheT>());
			if(itr.second){
				this->add_new_archetype_map<Tuple>(static_cast<ArcheT&>(*itr.first->second));
			}
			itr.first->second->insert(ent.id());

			return std::addressof(ent);
		}

		template <typename ...Ts>
			requires (!is_tuple_v<Ts> && ...)
		entity_id create_entity(){
			return create_entity<std::tuple<Ts ...>>();
		}
	public:
		void mark_expired_all(){
			std::lock_guard _{entity_expire_mutex_};
			for (auto & entity : entities){
				to_expire.try_emplace(std::addressof(entity));
			}
		}

		bool mark_expired(entity_id entity){
			std::lock_guard _{entity_expire_mutex_};
			return to_expire.try_emplace(entity).second;
		}

		void do_deferred(){
			do_deferred_destroy();
			do_deferred_add();
		}

		void do_deferred_add(){
			if(!staging_adds.empty()){
				gch::small_vector<type_identity_index> indices{};

				assert_unlocked(entity_staging_add_mutex_);

				for (auto & staging_add_base : staging_adds){
					if(auto itr = std::ranges::find(indices, staging_add_base->get_idx()); itr == indices.end()){
						indices.push_back(staging_add_base->get_idx());
						staging_add_base->add_archetype(*this);
					}

					staging_add_base->push_entity(*this);
				}

				staging_adds.clear();
			}

			for (const auto & [type, archetype] : archetypes){
				archetype->dump_staging();
			}
		}

		void do_deferred_destroy(){
			assert_unlocked(entity_expire_mutex_);


			//TODO destroy expiration notify/check
			expired.append_range(to_expire | std::views::keys);
			to_expire.clear();

			modifiable_erase_if(expired, [this](entity_id e){
				if(e->expire_staging_counter_){
					--e->expire_staging_counter_;
				}else{
					if(e->archetype_){
						e->get_archetype()->erase(e);
					}else if(e->get_ref_count() == 0){
						const auto itr = entities.get_iterator(e);
						if(itr != entities.end())entities.erase(itr);
						return true;
					}

				}

				return false;
			});
		}

		void deferred_destroy_no_reference_entities(){
			assert_unlocked(entity_expire_mutex_);
			assert_unlocked(entity_acquire_mutex_);

			for (auto& entity : entities){
				if(entity.get_ref_count() == 0){
					to_expire.try_emplace(entity.id());
				}
			}
		}

		template <typename Tuple>
			requires (is_tuple_v<Tuple>)
		auto get_slice_of() const{
			static_assert(std::tuple_size_v<Tuple> > 0);

			using chunk_spans = unary_apply_to_tuple_t<strided_span, Tuple>;
			using searched_spans = small_vector_of<chunk_spans>;

			searched_spans result{};

			this->slice_and_then<Tuple>([&](chunk_spans spans){
				                            result.push_back(spans);
			                            }, [&](std::size_t sz){
				                            result.reserve(sz);
			                            });

			return result;
		}

		template <typename Exclusives = std::tuple<>, typename Fn, typename S>
		FORCE_INLINE void sliced_each(this S& self, Fn fn){
			using raw_params = remove_mfptr_this_args<Fn>;

			if constexpr (std::same_as<std::remove_cvref_t<std::tuple_element_t<0, raw_params>>, component_manager>){
				using params = unary_apply_to_tuple_t<std::decay_t, tuple_drop_first_elem_t<raw_params>>;
				using span_tuple = unary_apply_to_tuple_t<strided_span, params>;

				self.template slice_and_then<params, Exclusives>([&self, f = std::move(fn)](const span_tuple& p) FORCE_INLINE {
					auto count = std::ranges::size(std::get<0>(p));

					unary_apply_to_tuple_t<strided_span_iterator, params> iterators{};

					[&] <std::size_t ...Idx> (std::index_sequence<Idx...>) FORCE_INLINE {
						((std::get<Idx>(iterators) = std::ranges::begin(std::get<Idx>(p))), ...);
					}(std::make_index_sequence<std::tuple_size_v<params>>{});

					for(std::remove_cvref_t<decltype(count)> i = 0; i != count; ++i){
						[&] <std::size_t ...Idx> (std::index_sequence<Idx...>) FORCE_INLINE {
							std::invoke(f, self, *(std::get<Idx>(iterators)++) ...);
						}(std::make_index_sequence<std::tuple_size_v<params>>{});
					}
				});
			}else{
				using params = unary_apply_to_tuple_t<std::decay_t, raw_params>;
				using span_tuple = unary_apply_to_tuple_t<strided_span, params>;

				self.template slice_and_then<params, Exclusives>([f = std::move(fn)](const span_tuple& p) FORCE_INLINE {
					auto count = std::ranges::size(std::get<0>(p));

					unary_apply_to_tuple_t<strided_span_iterator, params> iterators{};

					[&] <std::size_t ...Idx> (std::index_sequence<Idx...>) FORCE_INLINE {
						((std::get<Idx>(iterators) = std::ranges::begin(std::get<Idx>(p))), ...);
					}(std::make_index_sequence<std::tuple_size_v<params>>{});

					for(std::remove_cvref_t<decltype(count)> i = 0; i != count; ++i){
						[&] <std::size_t ...Idx> (std::index_sequence<Idx...>) FORCE_INLINE {
							std::invoke(f,
								(assert(std::get<Idx>(iterators) != std::ranges::end(std::get<Idx>(p))),
									*(std::get<Idx>(iterators)++)) ...);
						}(std::make_index_sequence<std::tuple_size_v<params>>{});
					}
				});
			}

		}

		template <typename ...T>
			requires (!is_tuple_v<T> && ...)
		auto get_slice_of() const{
			return get_slice_of<std::tuple<T...>>();
		}



		template <typename Tuple>
		tuple_to_seq_chunk_t<Tuple>* get_entity_full_chunk(const entity& entity) const noexcept{
			if(entity.is_expired())return nullptr;
			static constexpr type_identity_index idx = unstable_type_identity_of<Tuple>();
			if(idx != entity.type())return nullptr;
			//TODO should this a must event? so at() directly?
			archetype<Tuple>& aty = static_cast<archetype<Tuple>&>(entity.get_archetype());
			return std::addressof(aty[entity.chunk_index()]);
		}

		template <typename T>
		T* get_entity_partial_chunk(const entity_id entity) const noexcept{
			assert(entity);
			if(entity->is_expired())return nullptr;

			static constexpr type_identity_index idx = unstable_type_identity_of<T>();
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

		struct savement{
			std::ptrdiff_t off;
			std::size_t size;
		};


		struct subrange{
			using value_type = const archetype_slice;
			using rng = std::span<value_type>;
			using itr = rng::pointer;

		// private:
			itr begin_itr;
			itr end_itr;
		public:

			[[nodiscard]] FORCE_INLINE constexpr subrange() noexcept = default;

			[[nodiscard]] FORCE_INLINE constexpr explicit(false) subrange(rng range) noexcept :
				begin_itr(std::to_address(range.begin())),
				end_itr(std::to_address(range.end())){
			}

			[[nodiscard]] FORCE_INLINE constexpr bool empty() const noexcept{
				return begin_itr == end_itr;
			}

			[[nodiscard]] FORCE_INLINE value_type& front() const noexcept{
				return *begin_itr;
			}

			[[nodiscard]] FORCE_INLINE itr begin() const noexcept{
				return begin_itr;
			}

			[[nodiscard]] FORCE_INLINE itr end() const noexcept{
				return end_itr;
			}
		};

	public:
		template <
			tuple_spec Tuple = std::tuple<>,
			tuple_spec Exclusives = std::tuple<>,
			typename Fn,
			std::invocable<std::size_t>	ReserveFn = std::identity>
			requires (!tuple_has_duplicate_types_v<tuple_cat_t<Tuple, Exclusives>>)
		void slice_and_then(Fn fn, ReserveFn reserve_fn = {}) const{
			static_assert(std::tuple_size_v<Tuple> > 0);
			using chunk_spans = unary_apply_to_tuple_t<strided_span, Tuple>;

			std::array<std::span<const archetype_slice>, std::tuple_size_v<Tuple>> spans{};

			static constexpr auto ExclusiveSize = std::tuple_size_v<Exclusives>;
			using vec = std::conditional_t<ExclusiveSize, gch::small_vector<const void*, std::clamp<std::size_t>(std::bit_ceil(ExclusiveSize * 2), 4, 32)>, std::monostate>;

			vec exclusives;

			if constexpr (ExclusiveSize > 0){
				[&, this]<std::size_t ...Idx>(std::index_sequence<Idx...>) FORCE_INLINE {
					([&, this](type_identity_index idx){
						if(auto itr = type_to_archetype.find(idx); itr != type_to_archetype.end()){

							for (const auto & slice : itr->second){
								exclusives.push_back(slice.identity());
							}
						}
					}(unstable_type_identity_of<std::tuple_element_t<Idx, Exclusives>>()), ...);
				}(std::make_index_sequence<ExclusiveSize>{});

				std::ranges::sort(exclusives);
				const auto rst = std::ranges::unique(exclusives);
				exclusives.erase(rst.begin(), rst.end());
			}

			std::size_t maximum_size{};

			{
				//TODO consider that 'none' is really casual, remove it?

				bool none = false;
				auto try_find = [&, this](type_identity_index idx) FORCE_INLINE -> std::span<const archetype_slice>{
					// std::println(std::cerr, "{}", idx.name());
					if(none)return {};

					if(auto itr = type_to_archetype.find(idx); itr != type_to_archetype.end()){
						maximum_size = std::max(maximum_size, itr->second.size());
						return itr->second;
					}

					none = true;
					return {};
				};

				[&] <std::size_t ...Idx> (std::index_sequence<Idx...>){
					((spans[Idx] = try_find(unstable_type_identity_of<std::tuple_element_t<Idx, Tuple>>())), ...);
				}(std::make_index_sequence<std::tuple_size_v<Tuple>>{});

				if(none)return;
			}

			std::array<subrange, std::tuple_size_v<Tuple>> ranges{};

			static constexpr auto comp = std::less<void>{};

			auto current_target{spans.front().front().identity()};
			auto current_max_frontier = current_target;
			ranges[0] = spans[0];
			for(std::size_t i = 1; i < std::tuple_size_v<Tuple>; ++i){
				ranges[i] = spans[i];
				const auto idt = ranges[i].front().identity();
				current_target = std::max(current_target, idt, comp);
				current_max_frontier = std::min(current_max_frontier, idt, comp);
			}

			if constexpr (!std::same_as<ReserveFn, std::identity>)(void)std::invoke(reserve_fn, maximum_size);
			while(true){
				auto current_min_frontier = current_target;
				for (auto&& rng : ranges){
					//TODO will binary search(lower bound) faster?
					if(comp(rng.front().identity(), current_target)){
						++rng.begin_itr;
						if(rng.empty()){
							return;
						}

						current_max_frontier = std::max(current_max_frontier, rng.front().identity(), comp);
						current_min_frontier = std::min(current_min_frontier, rng.front().identity(), comp);
					}
				}

				if(comp(current_target, current_max_frontier)){
					current_target = current_max_frontier;
				}else if(current_min_frontier == current_target){
					if constexpr (ExclusiveSize > 0){
						for (const auto& archetype_slice : ranges){
							if(std::ranges::binary_search(exclusives, archetype_slice.front().identity())){
								goto skip;
							}
						}
					}


					static constexpr auto checker = [] <typename RTup>(const RTup& rng) FORCE_INLINE{
#if DEBUG_CHECK
						auto stride = std::get<0>(rng).stride();

						[&] <std::size_t ... Idx>(std::index_sequence<Idx...>) FORCE_INLINE{
							(assert(std::get<Idx + 1>(rng).stride() == stride && "Incompatible stride"), ...);
						}(std::make_index_sequence<std::tuple_size_v<RTup> - 1>{});
#endif
					};


					[&] <std::size_t ...Idx> (std::index_sequence<Idx...>) FORCE_INLINE {
						if constexpr (std::invocable<Fn, std::array<archetype_slice, std::tuple_size_v<Tuple>>>){
							std::invoke(fn, std::array<archetype_slice, std::tuple_size_v<Tuple>>{static_cast<subrange&>(ranges[Idx]).front() ...});
						}else if constexpr (std::invocable<Fn, chunk_spans>){
							auto rngs = std::make_tuple(static_cast<subrange&>(ranges[Idx]).front().slice<std::tuple_element_t<Idx, Tuple>>() ...);
							checker(rngs);
							std::invoke(fn, std::move(rngs));
						}else{
							std::invoke(fn, static_cast<subrange&>(ranges[Idx]).front().slice<std::tuple_element_t<Idx, Tuple>>() ...);
						}
					}(std::make_index_sequence<std::tuple_size_v<Tuple>>{});

				skip:

					for (auto&& rng : ranges){
						++rng.begin_itr;
						if(rng.empty()){
							return;
						}

						const auto idt = rng.front().identity();
						current_target = std::max(current_target, idt, comp);
						current_max_frontier = std::max(current_max_frontier, idt, comp);
					}
				}
			}
		}

	private:
		template <typename Tuple = std::tuple<>>
		void add_new_archetype_map(archetype<Tuple>& type){
			using archetype_t = archetype<Tuple>;

			fixed_open_hash_map<type_identity_index, savement> type_to_index_map{0};

			auto insert = [&, this]<typename T>(){
				static constexpr type_identity_index idx{unstable_type_identity_of<T>()};

				auto itr = type_to_archetype.find(idx);
				const bool requires_resize = (archetype_slices_.size() == archetype_slices_.capacity());


				auto make_mark = [&]{
					if(!requires_resize)return;
					type_to_index_map.seemly_clear();
					type_to_index_map.reserve_exact(type_to_archetype.bucket_count());
					for (const auto & [key, value] : type_to_archetype){
						type_to_index_map.try_emplace(key, value.data() - archetype_slices_.data(), value.size());
					}
				};



				archetype_slice elem{
					&type,
					+[](archetype_base* arg) noexcept -> strided_span<std::byte>{
						auto slice = static_cast<archetype_t*>(arg)->template slice<T>();
						return strided_span<std::byte>{
							const_cast<std::byte*>(reinterpret_cast<const std::byte*>(std::ranges::data(slice))),
							slice.size(),
							slice.stride()
						};
					}
				};

				if(itr != type_to_archetype.end()){
					const auto span = itr->second;
					const savement savement{span.data() - archetype_slices_.data(), span.size()};
					//already has a span:
					if(const auto it = std::ranges::lower_bound(span, elem); it == span.end() || it->identity() != elem.identity()){
						make_mark();
						archetype_slices_.insert(archetype_slices_.begin() + (std::to_address(it) - archetype_slices_.data()), std::move(elem));

						if(requires_resize){
							for (auto&& [key, value] : type_to_archetype){
								auto original_idx = type_to_index_map.at(key);
								if(savement.off >= original_idx.off + original_idx.size){
									value = {archetype_slices_.data() + original_idx.off, original_idx.size};
								}else if(savement.off + savement.size <= original_idx.off){
									value = {archetype_slices_.data() + original_idx.off + 1, original_idx.size};
								}else{
									value = {archetype_slices_.data() + original_idx.off, original_idx.size + 1};
								}
							}
						}else{
							for (auto&& [key, value] : type_to_archetype){
								if(savement.off >= value.data() + value.size() - archetype_slices_.data()){
								}else if(savement.off + savement.size <= value.data() - archetype_slices_.data()){
									value = {value.data() + 1, value.size()};
								}else{
									value = {value.data(), value.size()  + 1};
								}
							}
						}
					}
				}else{
					make_mark();
					auto& inserted = archetype_slices_.emplace_back(std::move(elem));
					if(requires_resize){
						for (auto&& [key, value] : type_to_archetype){
							auto original_idx = type_to_index_map.at(key);
							value = {archetype_slices_.data() + original_idx.off, original_idx.size};
						}
					}

					type_to_archetype.try_emplace(idx, std::span{std::addressof(inserted), 1});
				}
			};

			static constexpr auto base_map_idx = unstable_type_identity_of<typename archetype_t::base_to_derive_map>();

			[&] <std::size_t ...Idx> (std::index_sequence<Idx...>){
				(insert.template operator()<typename std::tuple_element_t<Idx, typename archetype_t::base_to_derive_map>::first_type>(), ...);
			}(std::make_index_sequence<std::tuple_size_v<typename archetype_t::base_to_derive_map>>{});

		}


		template <typename Tuple, std::derived_from<archetype<Tuple>> Archetype = archetype<Tuple>, typename ...Args>
		std::unique_ptr<archetype_base> create_new_archetype_map(Args&& ...args){
			std::unique_ptr<archetype_base> ptr = std::make_unique<Archetype>(std::forward<Args>(args)...);
			this->add_new_archetype_map<Tuple>(static_cast<Archetype&>(*ptr));
			return ptr;
		}

		//TODO delete empty archetype map?
	};

	template <typename TupleT>
	void archetype<TupleT>::serializer::load(component_manager& target) const{
		archetype& archetype_ = target.add_archetype<archetype>();

		for (const auto& seq_chunk : buffer){
			archetype_.push_staging(target.acquire_entity<raw_tuple>(), trait::behavior::load(seq_chunk));
		}
	}
}

// NOLINTEND(*-misplaced-const)

