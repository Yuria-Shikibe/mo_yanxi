module;

#include <cassert>

export module mo_yanxi.react_flow:nodes;

import :manager;
import :node_interface;

import mo_yanxi.meta_programming;
import std;

namespace mo_yanxi::react_flow{
export
template <typename T>
struct provider_cached : provider_general<T>{
	using value_type = T;
	static_assert(std::is_object_v<value_type>);

private:
	T data_{};
	bool lazy_{};

public:
	[[nodiscard]] provider_cached() = default;

	[[nodiscard]] explicit provider_cached(manager& manager, bool is_lazy = false)
	: provider_general<T>(manager), lazy_(is_lazy){
	}

	using provider_general<T>::update_value;

	template <bool check_equal = false, std::invocable<T&> Proj, typename Ty>
		requires (std::assignable_from<std::invoke_result_t<Proj, T&>, Ty&&>)
	void update_value(Proj proj, Ty&& value){

		auto& val = std::invoke(std::move(proj), data_);
		if constexpr (check_equal){
			if(val == value){
				return;
			}
		}

		val = std::forward<Ty>(value);
		on_update();
	}

	[[nodiscard]] bool is_lazy() const noexcept{
		return lazy_;
	}

	void set_lazy(const bool lazy) noexcept{
		lazy_ = lazy;
		//TODO update when set to false?
	}

	[[nodiscard]] data_state get_data_state() const noexcept override{
		return data_state::fresh;
	}

	request_pass_handle<T> request_raw(bool allow_expired) override{
		//source is always fresh
		return react_flow::make_request_handle_expected_ref(data_);
	}


protected:

	void on_push(void* in_data_pass_by_move) override{
		T& target = *static_cast<T*>(in_data_pass_by_move);

		data_ = std::move(target);
		on_update();
	}

	void on_push(manager& manager, std::size_t, const void* in_data_pass_by_copy) override{
		const T& target = *static_cast<const T*>(in_data_pass_by_copy);

		data_ = target;
		on_update();
	}

	void mark_updated(std::size_t) noexcept override {
		//TODO does provider really need it?
		for(const successor_entry& successor : this->successors){
			successor.mark_updated();
		}
	}

private:
	void on_update(){
		for(const successor_entry& successor : this->successors){
			if(lazy_){
				successor.mark_updated();
			} else{
				assert(this->manager_ != nullptr);
				successor.update(*this->manager_, data_);
			}
		}
	}
};

//TODO provider transient?

template <typename T, typename ...Args>
struct modifier_async_task;



template <typename Ret, typename... Args>
struct modifier_base : type_aware_node<Ret>{
	static_assert((std::is_object_v<Args> && ...) && std::is_object_v<Ret>);

	static constexpr std::size_t arg_count = sizeof...(Args);
	static constexpr std::array<data_type_index, arg_count> in_type_indices{
		unstable_type_identity_of<Args>()...
	};
	using arg_type = std::tuple<std::remove_const_t<Args>...>;

private:
	std::array<node_ptr, arg_count> parents{};
	std::vector<successor_entry> successors{};

	async_type async_type_{};
	std::size_t dispatched_count_{};
	std::stop_source stop_source_{std::nostopstate};

public:
	[[nodiscard]] modifier_base() = default;

	[[nodiscard]] explicit modifier_base(async_type async_type)
		: async_type_(async_type){
	}

	[[nodiscard]] std::size_t get_dispatched() const noexcept{
		return dispatched_count_;
	}

	[[nodiscard]] async_type get_async_type() const noexcept{
		return async_type_;
	}

	void set_async_type(const async_type async_type) noexcept{
		async_type_ = async_type;
	}

	[[nodiscard]] std::stop_token get_stop_token() const noexcept{
		assert(stop_source_.stop_possible());
		return stop_source_.get_token();
	}

	bool async_cancel() noexcept{
		if(async_type_ == async_type::none) return false;
		if(dispatched_count_ == 0) return false;
		if(!stop_source_.stop_possible()) return false;
		stop_source_.request_stop();
		stop_source_ = std::stop_source{std::nostopstate};
		return true;
	}

	[[nodiscard]] data_state get_data_state() const noexcept override{
		data_state states{};

		for(const node_ptr& p : parents){
			update_state_enum(states, p->get_data_state());
		}

		return states;
	}

	[[nodiscard]] std::span<const data_type_index> get_in_socket_type_index() const noexcept final{
		return std::span{in_type_indices};
	}

	void disconnect_self_from_context() noexcept final{
		for(std::size_t i = 0; i < parents.size(); ++i){
			if(node_ptr& ptr = parents[i]){
				ptr->erase_successors_impl(i, *this);
				ptr = nullptr;
			}
		}
		for (const auto & successor : successors){
			successor.entity->erase_predecessor_impl(successor.index, *this);
		}
		successors.clear();
	}

	[[nodiscard]] std::span<const node_ptr> get_inputs() const noexcept final{
		return parents;
	}

	[[nodiscard]] std::span<const successor_entry> get_outputs() const noexcept final{
		return successors;
	}

private:
	void async_resume(manager& manager, Ret& data) const{
		this->update_children(manager, data);
	}

	bool connect_successors_impl(std::size_t slot, node& post) final{
		return try_insert(successors, slot, post);
	}

	bool erase_successors_impl(std::size_t slot, node& post) noexcept final{
		return try_erase(successors, slot, post);
	}

	void connect_predecessor_impl(std::size_t slot, node& prev) final{
		parents[slot] = std::addressof(prev);
	}

	void erase_predecessor_impl(std::size_t slot, node& prev) noexcept final{
		if(parents[slot] == &prev){
			parents[slot] = {};
		}
	}

protected:
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
		} else if(!stop_source_.stop_requested()){
			//stop source empty
			stop_source_ = {};
		}

		++dispatched_count_;

		if(arg_type arguments{}; this->load_argument_to(arguments, true)){
			manager.push_task(std::make_unique<modifier_async_task<Ret, Args...>>(*this, std::move(arguments)));
		}
	}

	void mark_updated(std::size_t from) noexcept override{
		for(const successor_entry& successor : this->successors){
			successor.mark_updated();
		}
	}

	virtual void update_children(manager& manager, Ret& val) const{
		for(const successor_entry& successor : this->successors){
			successor.update(manager, val);
		}
	}

	std::optional<Ret> apply( const std::stop_token& stop_token, arg_type&& arguments){
		return [&, this]<std::size_t... Idx>(std::index_sequence<Idx...>) -> std::optional<Ret>{
			return this->operator()(stop_token, std::move(std::get<Idx>(arguments)) ...);
		}(std::index_sequence_for<Args...>());
	}

	std::optional<Ret> apply( const std::stop_token& stop_token, const arg_type& arguments){
		return [&, this]<std::size_t... Idx>(std::index_sequence<Idx...>) -> std::optional<Ret>{
			return this->operator()(stop_token, std::get<Idx>(arguments) ...);
		}(std::index_sequence_for<Args...>());
	}

	//TODO support which changed?

	virtual std::optional<Ret> operator()(const std::stop_token& stop_token, Args&&... args){
		return this->operator()(stop_token, std::as_const(args)...);
	}

	virtual std::optional<Ret> operator()(const std::stop_token& stop_token, const Args&... args) = 0;

	virtual bool load_argument_to(arg_type& arguments, bool allow_expired){
		return [&, this]<std::size_t ... Idx>(std::index_sequence<Idx...>){
			return ([&, this]<std::size_t I>(){
				node& n = *parents[I];
				using Ty = std::tuple_element_t<I, arg_type>;
				if(auto rst = node_type_cast<Ty>(n).request(allow_expired)){
					std::get<I>(arguments) = std::move(rst).value();
					return true;
				}
				return false;
			}.template operator()<Idx>() && ...);
		}(std::index_sequence_for<Args...>{});
	}


	friend modifier_async_task<Ret, Args...>;
};

template <typename T, typename ...Args>
struct modifier_async_task final : async_task_base{
private:
	using type = modifier_base<T, Args...>;
	type* modifier_{};
	std::stop_token stop_token_{};

	std::tuple<Args...> arguments_{};
	std::optional<T> rst_cache_{};

public:
	[[nodiscard]] explicit modifier_async_task(type& modifier, std::tuple<Args...>&& args) :
	modifier_(std::addressof(modifier)),
	stop_token_(modifier.get_stop_token()), arguments_{std::move(args)}{
	}

	void on_finish(manager& manager) override{
		--modifier_->dispatched_count_;

		if(rst_cache_){
			modifier_->async_resume(manager, rst_cache_.value());
		}
	}

	node* get_owner_if_node() noexcept override{
		return modifier_;
	}

private:
	void execute() override{
		rst_cache_ = modifier_->apply(stop_token_, std::move(arguments_));
	}
};

export
template <typename Ret, typename... Args>
struct modifier_transient : modifier_base<Ret, Args...>{
	using base = modifier_base<Ret, Args...>;

	using base::modifier_base;

	request_pass_handle<Ret> request_raw(bool allow_expired) override{
		if(this->get_async_type() != async_type::none){
			if(this->get_dispatched() > 0){
				return make_request_handle_unexpected<Ret>(data_state::awaiting);
			}else{
				return make_request_handle_unexpected<Ret>(data_state::failed);
			}
		}

		typename base::arg_type arguments;
		if(this->load_argument_to(arguments, allow_expired)){
			if(auto return_value = this->apply({}, std::move(arguments))){
				return react_flow::make_request_handle_expected(std::move(return_value).value());
			}else{
				return make_request_handle_unexpected<Ret>(data_state::failed);
			}
		}

		return make_request_handle_unexpected<Ret>(data_state::expired);
	}

protected:
	void on_push(manager& manager, std::size_t target_index, const void* in_data_pass_by_copy) override{
		typename base::arg_type arguments{};

		bool any_failed{false};
		[&, this]<std::size_t ... Idx>(std::index_sequence<Idx...>){
			([&, this]<std::size_t I>(){
				using Ty = std::tuple_element_t<I, typename base::arg_type>;
				if(I == target_index){
					std::get<I>(arguments) = *static_cast<const Ty*>(in_data_pass_by_copy);
				}else{
					auto& nptr = static_cast<const node_ptr&>(this->get_inputs()[I]);
					if(!nptr){
						static_assert(std::is_default_constructible_v<Ty>);
						std::get<I>(arguments) = Ty{};
					}else{
						auto& n = *nptr;
						if(auto rst = node_type_cast<Ty>(n).request(true)){
						std::get<I>(arguments) = std::move(rst).value();
						}else{
							any_failed = true;
						}
					}


				}
			}.template operator()<Idx>(), ...);
		}(std::index_sequence_for<Args...>{});

		if(any_failed){
			this->mark_updated(-1);
		}else if(this->get_async_type() == async_type::none){
			if(auto cur = this->apply({}, std::move(arguments))){
				this->base::update_children(manager, *cur);
			}
		}else{
			this->async_launch(manager);
		}

	}
};

export
template <typename Ret, typename... Args>
struct modifier_argument_cached : modifier_base<Ret, Args...>{
	using base = modifier_base<Ret, Args...>;

private:
	bool lazy_{};
	std::bitset<base::arg_count> dirty{}; //only lazy nodes can be set dirty
	base::arg_type arguments{};
public:
	[[nodiscard]] modifier_argument_cached() = default;

	[[nodiscard]] explicit modifier_argument_cached(async_type async_type, bool is_lazy = false)
	: modifier_base<Ret, Args...>(async_type), lazy_(is_lazy){
	}

	[[nodiscard]] bool is_lazy() const noexcept{
		return lazy_;
	}

	void set_lazy(const bool lazy) noexcept{
		lazy_ = lazy;
	}

	[[nodiscard]] data_state get_data_state() const noexcept override{
		if(dirty.any()){
			return data_state::expired;
		}else{
			return data_state::fresh;
		}
	}

	request_pass_handle<Ret> request_raw(bool allow_expired) override{
		if(this->get_async_type() != async_type::none){
			if(this->get_dispatched() > 0){
				return make_request_handle_unexpected<Ret>(data_state::awaiting);
			}else{
				return make_request_handle_unexpected<Ret>(data_state::failed);
			}
		}

		auto expired = update_arguments();

		if(!expired || allow_expired){
			if(auto return_value = this->apply({}, arguments)){
				return react_flow::make_request_handle_expected(std::move(return_value).value());
			}else{
				return make_request_handle_unexpected<Ret>(data_state::failed);
			}

		}

		return make_request_handle_unexpected<Ret>(data_state::expired);
	}

protected:
	void on_push(manager& manager, std::size_t slot, const void* in_data_pass_by_copy) override{
		assert(slot < base::arg_count);
		update_data(slot, in_data_pass_by_copy);

		if(lazy_){
			base::mark_updated(-1);
		} else{
			if(this->get_async_type() == async_type::none){
				if(auto cur = this->apply({}, arguments)){
					this->base::update_children(manager, *cur);
				}
			}else{
				this->async_launch(manager);
			}
		}
	}

	void mark_updated(std::size_t from) noexcept override{
		dirty.set(from, true);
		base::mark_updated(from);
	}

	bool load_argument_to(modifier_base<Ret, Args...>::arg_type& arguments, bool allow_expired) final{
		auto is_expired = update_arguments();

		if(!is_expired || allow_expired){
			arguments = this->arguments;
			return true;
		}

		return false;
	}

private:

	bool update_arguments(){
		if(!dirty.any())return false;
		bool any_expired{};
		[&, this]<std::size_t ... Idx>(std::index_sequence<Idx...>){
			([&, this]<std::size_t I>(){
				if(!dirty[I])return;

				node& n = *static_cast<const node_ptr&>(this->get_inputs()[Idx]);
				if(auto rst = node_type_cast<std::tuple_element_t<Idx, typename base::arg_type>>(n).request(false)){
					dirty.set(I, false);
					std::get<Idx>(arguments) = std::move(rst).value();
				}else{
					any_expired = true;
				}
			}.template operator()<Idx>(), ...);
		}(std::index_sequence_for<Args...>{});
		return any_expired;
	}

	void update_data(std::size_t slot, const void* in_data_pass_by_copy){
		dirty.set(slot, false);

		[&, this]<std::size_t ... Idx>(std::index_sequence<Idx...>){
			(void)((Idx == slot && (std::get<Idx>(arguments) = *static_cast<const std::tuple_element_t<Idx, typename
				base::arg_type>*>(in_data_pass_by_copy), true)) || ...);
		}(std::index_sequence_for<Args...>{});
	}
};

template <typename T>
struct intermediate_cache : type_aware_node<T>{
private:
	T cache_{};
	bool is_expired_{};
	bool is_lazy_{};

	node_ptr parent{};
	std::vector<successor_entry> successors{};

public:
	request_pass_handle<T> request_raw(bool allow_expired) override{
		if(is_expired_){
			if(auto rst = node_type_cast<T>(*parent).request(allow_expired)){
				is_expired_ = false;
				cache_ = std::move(rst).value();
			}
		}

		if(!is_expired_ || allow_expired){
			return react_flow::make_request_handle_expected_ref(cache_);
		}

		return react_flow::make_request_handle_unexpected<T>(data_state::expired);
	}
protected:

	void mark_updated(std::size_t from_index) noexcept override{
		is_expired_ = true;
		std::ranges::for_each(successors, &successor_entry::mark_updated);
	}

	void on_push(manager& manager, std::size_t from_index, const void* in_data_pass_by_copy) override{
		cache_ = static_cast<T*>(in_data_pass_by_copy);
		is_expired_ = false;

		if(is_lazy_){
			for(const successor_entry& successor : successors){
				successor.update(manager, std::addressof(cache_));
			}
		} else{
			std::ranges::for_each(successors, &successor_entry::mark_updated);
		}
	}

	void disconnect_self_from_context() noexcept final{
		for (const auto & successor : successors){
			successor.entity->erase_predecessor_impl(successor.index, *this);
		}
		successors.clear();
		parent->erase_successors_impl(0, *this);
		parent = nullptr;
	}


	[[nodiscard]] std::span<const node_ptr> get_inputs() const noexcept final{
		return {&parent, 1};
	}

	[[nodiscard]] std::span<const successor_entry> get_outputs() const noexcept final{
		return successors;
	}

private:
	bool connect_successors_impl(std::size_t slot, node& post) final{
		return try_insert(successors, slot, post);
	}

	bool erase_successors_impl(std::size_t slot, node& post) noexcept final{
		return try_erase(successors, slot, post);
	}

	void connect_predecessor_impl(std::size_t slot, node& prev) final{
		assert(slot == 0);
		parent = std::addressof(prev);
	}

	void erase_predecessor_impl(std::size_t slot, node& prev) noexcept final{
		assert(slot == 0);
		if(parent == &prev)parent = {};
	}


public:
	[[nodiscard]] intermediate_cache() = default;

	[[nodiscard]] explicit intermediate_cache(bool is_lazy)
	: is_lazy_(is_lazy){
	}

	[[nodiscard]] data_state get_data_state() const noexcept override{
		return is_expired_ ? data_state::expired : data_state::fresh;
	}

};

template <typename T>
struct optional_value_type : std::type_identity<T>{

};

template <typename T>
struct optional_value_type<std::optional<T>> : std::type_identity<T>{

};

template <typename T>
using optional_value_type_t = typename optional_value_type<T>::type;

template <typename Fn, typename... Args>
consteval auto test_invoke_result(){
	if constexpr (std::invocable<Fn, const std::stop_token&, Args&&...>){
		return std::type_identity<optional_value_type_t<std::invoke_result_t<Fn, const std::stop_token&, Args...>>>{};
	}else{
		return std::type_identity<optional_value_type_t<std::invoke_result_t<Fn, Args...>>>{};
	}
}

//TODO allow optional<T&> one day...

template <typename Fn, typename... Args>
using transformer_base_t = modifier_transient<std::remove_cvref_t<typename decltype(test_invoke_result<Fn, Args&&...>())::type>, Args...>;

template <typename Fn, typename ...Args>
struct transformer : transformer_base_t<Fn, Args...>{
private:
	Fn fn;

public:
	[[nodiscard]] explicit transformer(async_type async_type, Fn&& fn)
	: transformer_base_t<Fn, Args...>(async_type), fn(std::move(fn)){
	}

	[[nodiscard]] explicit transformer(async_type async_type, const Fn& fn)
	: transformer_base_t<Fn, Args...>(async_type), fn(fn){
	}

protected:
	std::optional<typename function_traits<Fn>::return_type> operator()(
		const std::stop_token& stop_token,
		const Args& ...args) override{
		if constexpr (std::invocable<Fn, const std::stop_token&, Args&&...>){
			return std::invoke(fn, stop_token, args...);
		}else{
			return std::invoke(fn, args...);
		}
	}

	std::optional<typename function_traits<Fn>::return_type> operator()(
		const std::stop_token& stop_token,
		Args&& ...args) override{
		if constexpr (std::invocable<Fn, const std::stop_token&, Args&&...>){
			return std::invoke(fn, stop_token, std::move(args)...);
		}else{
			return std::invoke(fn, std::move(args)...);
		}
	}
};


template <typename RawFn, typename Tup>
struct transformer_unambiguous_helper;

template <typename RawFn, typename... Args>
struct transformer_unambiguous_helper<RawFn, std::tuple<Args...>>{
	using type = transformer<RawFn, Args...>;
};

export
template <typename Fn>
using transformer_unambiguous = typename transformer_unambiguous_helper<
	std::remove_cvref_t<Fn>,
	typename function_traits<std::remove_cvref_t<Fn>>::mem_func_args_type
>::type;

export
template <typename Fn>
transformer_unambiguous<Fn> make_transformer(async_type async_type, Fn&& fn){
	return transformer_unambiguous<Fn>{async_type, std::forward<Fn>(fn)};
}

}
