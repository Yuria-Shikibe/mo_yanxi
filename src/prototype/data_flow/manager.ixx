module;

export module mo_yanxi.data_flow:manager;

import :node_interface;
import mo_yanxi.referenced_ptr;
import mo_yanxi.spsc_queue;

namespace mo_yanxi::data_flow{

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

export struct manager{
private:
	std::vector<referenced_ptr<provider_general>> providers{};
	std::vector<referenced_ptr<node>> modifiers{};
	std::vector<referenced_ptr<terminal>> terminals{};

	//TODO named nodes?
	using async_task_queue = mpsc_queue<std::move_only_function<void()>>;
	async_task_queue pending_received_updates{};
	async_task_queue::container_type recycled_queue_container_{};

	mpsc_queue<std::unique_ptr<async_task_base>> pending_async_modifiers_{};
	std::mutex done_mutex_{};
	std::vector<std::unique_ptr<async_task_base>> done_[2]{};

	std::jthread async_thread_{[](std::stop_token stop_token, manager& manager){
		execute_async_tasks(std::move(stop_token), manager);
	}, std::ref(*this)};

private:
	template <typename T, typename ...Args>
	referenced_ptr<T> make_node(Args&& ...args){
		if constexpr (std::constructible_from<T, manager&, Args&&...>){
			return referenced_ptr<T>(std::in_place, *this, std::forward<Args>(args)...);
		}else{
			return referenced_ptr<T>(std::in_place, std::forward<Args>(args)...);
		}
	}

public:
	[[nodiscard]] manager() = default;

	template <std::derived_from<provider_general> T, typename ...Args>
	T& add_provider(Args&& ...args){
		auto& ptr = providers.emplace_back(this->make_node<T>(std::forward<Args>(args)...));
		return static_cast<T&>(*ptr);
	}

	template <std::derived_from<node> T, typename ...Args>
	T& add_modifier(Args&& ...args){
		auto& ptr = modifiers.emplace_back(this->make_node<T>(std::forward<Args>(args)...));
		return static_cast<T&>(*ptr);
	}

	template <std::derived_from<terminal> T, typename ...Args>
	T& add_terminal(Args&& ...args){
		auto& ptr = terminals.emplace_back(this->make_node<T>(std::forward<Args>(args)...));
		return static_cast<T&>(*ptr);
	}


	/**
	 * @brief Called from OTHER thread that need do sth on the main data flow thread.
	 */
	template <std::invocable<> Fn>
		requires (std::move_constructible<std::remove_cvref_t<Fn>>)
	void push_posted_act(Fn&& fn){
		pending_received_updates.emplace(std::forward<Fn>(fn));
	}

	void push_task(std::unique_ptr<async_task_base> task){
		pending_async_modifiers_.push(std::move(task));
	}

	void update(){
		if(pending_received_updates.swap(recycled_queue_container_)){
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

private:
	static void execute_async_tasks(std::stop_token stop_token, manager& manager){
		while(!stop_token.stop_requested()){
			auto&& task = manager.pending_async_modifiers_.consume([&stop_token]{
				return stop_token.stop_requested();
			});

			if(stop_token.stop_requested())return;

			if(task){
				task.value()->execute();
				std::lock_guard lg{manager.done_mutex_};
				manager.done_[1].push_back(std::move(task.value()));
			}
		}
	}
};

}