module;

#include <cassert>
#include "ext/enum_operator_gen.hpp"

export module mo_yanxi.data_flow:node_interface;
import std;

import mo_yanxi.type_register;
import mo_yanxi.referenced_ptr;
import mo_yanxi.data_storage;

namespace mo_yanxi::data_flow{
using data_type_index = type_identity_index;

export struct manager;

export struct socket_type_pair{
	data_type_index input;
	data_type_index output;
};

struct invalid_node_type : std::invalid_argument{
	[[nodiscard]] explicit invalid_node_type(const std::string& msg)
	: invalid_argument(msg){
	}

	[[nodiscard]] explicit invalid_node_type(const char* msg)
	: invalid_argument(msg){
	}
};


export enum struct data_state{
	fresh,
	expired,
	awaiting,
	failed
};

using request_pass_handle = std::expected<data_storage<>, data_state>;

[[nodiscard]] request_pass_handle make_request_handle_unexpected(data_state error) noexcept {
	return request_pass_handle{std::unexpect, error};
}

template <typename T>
[[nodiscard]] request_pass_handle make_request_handle_expected(T&& data) noexcept {
	return request_pass_handle(std::in_place, std::in_place_type<std::remove_cvref_t<T>>, std::forward<T>(data));
}

template <typename T>
[[nodiscard]] request_pass_handle make_request_handle_expected_ref(const T& data) noexcept {
	return request_pass_handle(std::in_place, data);
}

ENUM_COMPARISON_OPERATORS(data_state, export)

export enum struct async_state{
	no_task,
	running,
};

export enum struct async_type{
	/**
	 * @brief the modifier is synchronized
	 */
	none,

	/**
	 * @brief When a new update is arrived and the previous is not done yet, cancel the previous
	 */
	async_latest,

	/**
	 * @brief When a new update is arrived and the previous is not done yet, just dispatch again
	 */
	async_all,

	def = async_latest
};


template <std::ranges::input_range Rng>
	requires (std::is_scoped_enum_v<std::ranges::range_value_t<Rng>>)
std::ranges::range_value_t<Rng> merge_data_state(const Rng& states) noexcept {
	return std::ranges::max(states, std::ranges::less{}, [](const auto v){
		return std::to_underlying(v);
	});
}

template <typename T>
void update_state_enum(T& state, T other) noexcept {
	state = T{std::max(std::to_underlying(state), std::to_underlying(other))};
}

export struct successor_entry;
export struct terminal;

export struct node : referenced_object{
	friend successor_entry;
	friend terminal;

	[[nodiscard]] node() = default;

	virtual ~node() = default;

	[[nodiscard]] virtual data_type_index get_out_socket_type_index() const noexcept{
		return nullptr;
	}
	
	[[nodiscard]] virtual std::span<const data_type_index> get_in_socket_type_index() const noexcept{
		return {};
	}

	[[nodiscard]] virtual data_state get_data_state() const noexcept{
		return data_state::failed;
	}

	bool connect_successors_unchecked(const std::size_t slot_of_successor, node& post){
		if(connect_successors_impl(slot_of_successor, post)){
			post.connect_predecessor_impl(slot_of_successor, *this);
			return true;
		}
		return false;
	}

	bool connect_predecessor_unchecked(const std::size_t slot_of_successor, node& prev){
		return prev.connect_successors_unchecked(slot_of_successor, *this);
	}

	bool connect_successors(const std::size_t slot, node& post){
		if(!std::ranges::contains(post.get_in_socket_type_index(), get_out_socket_type_index())){
			throw invalid_node_type{"Node type NOT match"};
		}

		return connect_successors_unchecked(slot, post);
	}

	bool connect_predecessor(const std::size_t slot, node& prev){
		return prev.connect_successors(slot, *this);
	}

	bool connect_successors(node& post){
		const auto rng = post.get_in_socket_type_index();
		if(auto itr = std::ranges::find(rng, get_out_socket_type_index()); itr != rng.end()){
			return connect_successors_unchecked(std::ranges::distance(rng.begin(), itr), post);
		}else{
			throw invalid_node_type{"Failed To Find Slot"};
		}
	}

	bool connect_predecessor(node& prev){
		return prev.connect_successors(*this);
	}

	bool disconnect_successors(const std::size_t slot, node& post){
		if(erase_successors_impl(slot, post)){
			post.erase_predecessor_impl(slot, *this);
			return true;
		}
		return false;
	}

	bool disconnect_predecessor(const std::size_t slot, node& prev){
		return prev.disconnect_successors(slot, *this);
	}


	bool disconnect_successors(node& post){
		const auto rng = post.get_in_socket_type_index();
		if(auto itr = std::ranges::find(rng, get_out_socket_type_index()); itr != rng.end()){
			auto slot = std::ranges::distance(rng.begin(), itr);
			if(erase_successors_impl(slot, post)){
				post.erase_predecessor_impl(slot, *this);
				return true;
			}
		}else{
			throw invalid_node_type{"Failed To Find Slot"};
		}

		return false;
	}

	bool disconnect_predecessor(node& prev){
		return prev.disconnect_successors(*this);
	}

	template <typename T, typename S>
	std::optional<T> request(this S& self, bool allow_expired){
		if(unstable_type_identity_of<T>() != self.get_out_socket_type_index()){
			throw invalid_node_type{"Node type NOT match"};
		}

		if(auto rst = static_cast<node&>(self).request_impl(allow_expired)){
			return data_storage_view<T>{rst.value()}.fetch();
		}else{
			return std::nullopt;
		}
	}

	template <typename T, typename S>
	std::expected<T, data_state> request_stated(this S& self, bool allow_expired){
		if(unstable_type_identity_of<T>() != self.get_out_socket_type_index()){
			throw invalid_node_type{"Node type NOT match"};
		}

		if(auto rst = static_cast<node&>(self).request_impl(allow_expired)){
			if(auto optal = data_storage_view<T>{rst.value()}.fetch()){
				return std::expected<T, data_state>{optal.value()};
			}
			return std::unexpected{data_state::failed};
		}else{
			return std::unexpected{rst.error()};
		}
	}

protected:

	/**
	 * @brief Pull data from parent node
	 *
	 * The data flow is from terminal to source (pull)
	 *
	 * @param allow_expired
	 */
	virtual request_pass_handle request_impl(bool allow_expired) = 0;


	virtual bool connect_successors_impl(std::size_t slot, node& post){
		return false;
	}

	virtual void connect_predecessor_impl(std::size_t slot, node& prev){

	}

	virtual bool erase_successors_impl(std::size_t slot, node& post){
		return false;
	}

	virtual void erase_predecessor_impl(std::size_t slot, node& prev){

	}

	/**
	 * @brief update the node
	 *
	 * The data flow is from source to terminal (push)
	 *
	 * @param manager
	 * @param in_data_pass_by_copy ptr to const data, provided by parent
	 */
	virtual void update(manager& manager, std::size_t from_index, const void* in_data_pass_by_copy){

	}

	/**
	 *
	 * @brief Only notify the data has changed, used for lazy nodes. Should be mutually exclusive with update when propagate.
	 *
	 */
	virtual void mark_updated(std::size_t from_index) noexcept{

	}

};

export struct successor_entry{
	std::size_t index;
	referenced_ptr<node> entity;

	[[nodiscard]] successor_entry() = default;

	[[nodiscard]] successor_entry(const std::size_t index, node& entity)
	: index(index),
	entity(std::addressof(entity)){
	}

	template <typename T>
	void update(manager& manager, const T& data) const{
		assert(unstable_type_identity_of<T>() == entity->get_in_socket_type_index()[index]);
		entity->update(manager, index, std::addressof(data));
	}

	void mark_updated() const{
		entity->mark_updated(index);
	}
};

bool try_insert(std::vector<successor_entry>& successors, std::size_t slot, node& next){
	if(std::ranges::find_if(successors, [&](const successor_entry& e){
		return e.index == slot && e.entity.get() == &next;
	}) != successors.end()){
		return false;
	}

	successors.emplace_back(slot, next);
	return true;
}

bool try_erase(std::vector<successor_entry>& successors, const std::size_t slot, const node& next){
	return std::erase_if(successors, [&](const successor_entry& e){
		return e.index == slot && e.entity.get() == &next;
	});
}


export struct provider_general : node{
	[[nodiscard]] provider_general() = default;

	[[nodiscard]] explicit provider_general(manager& manager)
	: manager_(std::addressof(manager)){
	}


	template <typename T>
	void update_value(T&& value){
		if(unstable_type_identity_of<T>() != get_out_socket_type_index()){
			throw invalid_node_type{"Failed To Find Slot"};
		}

		this->update_value_unchecked(std::move(value));
	}

	template <typename T>
	void update_value(const T& value){
		if(unstable_type_identity_of<T>() != get_out_socket_type_index()){
			throw invalid_node_type{"Failed To Find Slot"};
		}

		this->update_value_unchecked(T{value});
	}

	template <typename T>
	void update_value_unchecked(T&& value){
		this->update(std::addressof(value));
	}


private:
	friend manager;


protected:
	manager* manager_{};
	std::vector<successor_entry> successors{};

	virtual void update(void* in_data_pass_by_move);

	bool connect_successors_impl(const std::size_t slot, node& post) override{
		return try_insert(successors, slot, post);
	}

	bool erase_successors_impl(std::size_t slot, node& post) override{
		return try_erase(successors, slot, post);
	}
};

void provider_general::update(void* in_data_pass_by_move){
}

/**
 * @brief all these member functions should be called on the same thread
 */
export struct asyncable_modifier : node{


private:
	async_type async_type_{};
	std::size_t dispatched_count_{};
	std::stop_source stop_source_{std::nostopstate};


public:
	[[nodiscard]] asyncable_modifier() = default;

	[[nodiscard]] explicit asyncable_modifier(const async_type async_type)
	: async_type_(async_type){
	}

	[[nodiscard]] std::size_t get_dispatched() const noexcept{
		return dispatched_count_;
	}

	[[nodiscard]] data_flow::async_type get_async_type() const noexcept{
		return async_type_;
	}

	void set_async_type(const data_flow::async_type async_type) noexcept {
		this->async_type_ = async_type;
	}

	[[nodiscard]] std::stop_token get_stop_token() const noexcept{
		assert(stop_source_.stop_possible());
		return stop_source_.get_token();
	}

	virtual void async_resume(manager& manager, const void* data){
		--dispatched_count_;
	}

	/*virtual*/ bool async_cancel() noexcept{
		if(async_type_ == async_type::none)return false;
		if(dispatched_count_ == 0)return false;
		if(!stop_source_.stop_possible())return false;
		stop_source_.request_stop();
		stop_source_ = std::stop_source{std::nostopstate};
		return true;
	}

protected:
	virtual void async_launch_impl(manager& manager) = 0;

public:
	void async_launch(manager& manager){
		if(async_type_ == async_type::none){
			throw std::logic_error("async_launch on a synchronized object");
		}

		if(stop_source_.stop_possible()){
			if(async_type_ == async_type::async_latest){
				if(async_cancel()){
					//only reset when really requested stop
					stop_source_ = {};
				}
			}
		}else if(!stop_source_.stop_requested()){
			//stop source empty
			stop_source_ = {};
		}

		++dispatched_count_;
		async_launch_impl(manager);
	}
};

export struct terminal : node{
protected:
	referenced_ptr<node> parent{};

	void connect_predecessor_impl(const std::size_t slot, node& prev) override{
		assert(slot == 0);
		parent = std::addressof(prev);
	}

	void erase_predecessor_impl(std::size_t slot, node& prev) override{
		assert(slot == 0);
		if(parent == &prev)parent = {};
	}

	request_pass_handle request_impl(const bool allow_expired) override{
		assert(parent != nullptr);
		return parent->request_impl(allow_expired);
	}
};

export
template <typename T>
struct terminal_typed_cached;

export
template <typename T>
struct terminal_typed : terminal{
public:
	static constexpr data_type_index node_data_type_index = unstable_type_identity_of<T>();

	template <typename Ty>
	std::optional<Ty> request(bool allowExpired) = delete;

	std::optional<T> request(bool allowExpired){
		return terminal::request<T>(allowExpired);
	}

	[[nodiscard]] std::span<const data_type_index> get_in_socket_type_index() const noexcept override{
		return std::span{&node_data_type_index, 1};
	}

	[[nodiscard]] data_type_index get_out_socket_type_index() const noexcept override{
		return node_data_type_index;
	}

	[[nodiscard]] bool is_expired() const noexcept{
		return is_expired_;
	}

	[[nodiscard]] data_state get_data_state() const noexcept override{
		return this->parent->get_data_state();
	}

	void check_expired_and_update(bool allow_expired){
		if(!is_expired())return;

		if(auto rst = request<T>(allow_expired)){
			this->on_update(rst.value().fetch());
		}
	}

protected:
	virtual void on_update(const T& data){
		this->is_expired_ = false;
	}

	void mark_updated(const std::size_t from_index) noexcept override{
		assert(from_index == 0);
		this->is_expired_ = true;
	}

private:
	friend terminal_typed_cached<T>;
	bool is_expired_{};

	void update(manager& manager, const std::size_t from_index, const void* in_data_pass_by_copy) final{
		assert(from_index == 0);
		this->on_update(*static_cast<const T*>(in_data_pass_by_copy));
	}

};


export
template <typename T>
struct terminal_typed_cached : terminal_typed<T>{
public:
	template <typename Ty>
	std::optional<Ty> request(bool allowExpired) = delete;

	std::optional<T> request(bool allowExpired){
		if(!allowExpired){
			update_cache();
		}

		if(!this->is_expired() || allowExpired){
			return cache_;
		}else{
			return std::nullopt;
		}
	}

	[[nodiscard]] const T& acquire_cache(){
		update_cache();
		return cache_;
	}

	[[nodiscard]] data_state get_data_state() const noexcept override{
		if(this->is_expired())return data_state::expired;
		return data_state::fresh;
	}

protected:
	void on_update(const T& data) override{
		cache_ = data;
		terminal_typed<T>::on_update(data);
	}

private:
	T cache_{};

	request_pass_handle request_impl(const bool allow_expired) final{
		update_cache();

		if(!this->is_expired() || allow_expired){
			return data_flow::make_request_handle_expected_ref(cache_);
		}else{
			return data_flow::make_request_handle_unexpected(data_state::expired);
		}
	}

	void update_cache(){
		if(this->is_expired()){
			if(auto rst = this->template request<T>(false)){
				this->is_expired_ = false;
				this->on_update(*rst.value());
			}
		}
	}
};

}