module;

export module mo_yanxi.react_flow:manager;

import :node_interface;
import mo_yanxi.referenced_ptr;
import mo_yanxi.utility;
import mo_yanxi.mpsc_queue;

namespace mo_yanxi::react_flow{

export struct manager;

struct async_task_base{
friend manager;

public:
	[[nodiscard]] async_task_base() = default;

	virtual ~async_task_base() = default;

	virtual void execute(){

	}

	virtual void on_finish(manager& manager){

	}
};

//TODO allocator?

export struct manager_no_async_t{
};

export constexpr inline manager_no_async_t manager_no_async{};

export struct manager{
private:
	std::vector<node_ptr> nodes_anonymous_{};

	using async_task_queue = mpsc_queue<std::move_only_function<void()>>;
	async_task_queue pending_received_updates_{};
	async_task_queue::container_type recycled_queue_container_{};

	mpsc_queue<std::unique_ptr<async_task_base>> pending_async_modifiers_{};
	std::mutex done_mutex_{};
	std::vector<std::unique_ptr<async_task_base>> done_[2]{};

	std::mutex async_request_mutex_{};
	std::vector<std::packaged_task<void()>> async_request_[2]{};


	std::jthread async_thread_{};


public:
	[[nodiscard]] manager() : async_thread_([](std::stop_token stop_token, manager& manager){
		execute_async_tasks(std::move(stop_token), manager);
	}, std::ref(*this)) {}

	[[nodiscard]] explicit manager(manager_no_async_t){}

	template <std::derived_from<node> T, typename ...Args>
	[[nodiscard]] referenced_ptr<T> make_node(Args&& ...args){
		return mo_yanxi::back_redundant_construct<referenced_ptr<T>, 1>(std::in_place, *this, std::forward<Args>(args)...);
	}

	template <std::derived_from<node> T, typename ...Args>
	T& add_node(Args&& ...args){
		auto& ptr = nodes_anonymous_.emplace_back(this->make_node<T>(std::forward<Args>(args)...));
		return static_cast<T&>(*ptr);
	}

	bool erase_node(node& n, bool disconnect_it) noexcept {
		if(auto itr = std::ranges::find(nodes_anonymous_, &n, [](const node_ptr& nd){
			return nd.get();
		}); itr != nodes_anonymous_.end()){
			if(disconnect_it){
				(*itr)->disconnect_self_from_context();
			}

			*itr = std::move(nodes_anonymous_.back());
			nodes_anonymous_.pop_back();

			return true;
		}
		return false;
	}


	/**
	 * @brief Called from OTHER thread that need do sth on the main data flow thread.
	 */
	template <std::invocable<> Fn>
		requires (std::move_constructible<std::remove_cvref_t<Fn>>)
	void push_posted_act(Fn&& fn){
		pending_received_updates_.emplace(std::forward<Fn>(fn));
	}

	void push_task(std::unique_ptr<async_task_base> task){
		if(async_thread_.joinable()){
			pending_async_modifiers_.push(std::move(task));
		}else{
			task->execute();
			done_[1].push_back(std::move(task));
		}
	}

	void update(){
		if(pending_received_updates_.swap(recycled_queue_container_)){
			for (auto&& move_only_function : recycled_queue_container_){
				move_only_function();
			}
			recycled_queue_container_.clear();
		}

		{

			{
				std::lock_guard lg{done_mutex_};
				if(done_[1].empty()){
					goto RET;
				}
				std::swap(done_[0], done_[1]);
			}

			for (auto && task_base : done_[0]){
				task_base->on_finish(*this);
			}
			done_[0].clear();

			RET:
			(void)0;
		}
	}

	~manager(){
		if(async_thread_.joinable()){
			async_thread_.request_stop();
			pending_async_modifiers_.notify();
			async_thread_.join();
		}
	}

private:
	static void execute_async_tasks(std::stop_token stop_token, manager& manager){
		while(!stop_token.stop_requested()){
			auto&& task = manager.pending_async_modifiers_.consume([&stop_token]{
				return stop_token.stop_requested();
			});

			if(task){
				task.value()->execute();
				std::lock_guard lg{manager.done_mutex_};
				manager.done_[1].push_back(std::move(task.value()));
			}else{
				return;
			}
		}
	}
};

}