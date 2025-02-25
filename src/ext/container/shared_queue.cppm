//
// Created by Matrix on 2024/10/31.
//

export module ext.shared_queue;

import ext.array_queue;
import ext.meta_programming;
import mo_yanxi.concepts;
import ext.condition_variable_single;

import std;

namespace mo_yanxi{
	export
	template <typename T, std::size_t queueSize = 16>
		requires (std::is_default_constructible_v<T>)
	struct shared_queue{
		using value_type = T;
	private:
		condition_variable_single cond_v_push{};
		condition_variable_single cond_v_consume{};
		array_queue<value_type, queueSize> queue{};

		mutable std::mutex mtx{};

	public:
		void push(const value_type& value) noexcept(std::is_nothrow_copy_assignable_v<T>) requires std::is_copy_assignable_v<T>{
			{
				std::unique_lock lock{mtx};

				cond_v_push.wait(lock, [this]{
					return !queue.full();
				});

				queue.push(value);
			}

			cond_v_consume.notify_one();
		}

		void push(value_type&& value) noexcept(std::is_nothrow_move_assignable_v<T>) requires std::is_move_assignable_v<T>{
			{
				std::unique_lock lock{mtx};

				cond_v_push.wait(lock, [this]{
					return !queue.full();
				});

				queue.push(std::move(value));
			}

			cond_v_consume.notify_one();
		}

		template <typename ...Args>
			requires (std::constructible_from<value_type, Args...>)
		void emplace(Args&& ...args) noexcept(std::is_nothrow_move_assignable_v<T> && T{std::forward<Args>(args)...}) {
			{
				std::unique_lock lock{mtx};

				cond_v_push.wait(lock, [this]{
					return !queue.full();
				});

				queue.emplace(std::forward<Args>(args) ...);
			}

			cond_v_consume.notify_one();
		}

		value_type consume() noexcept{
			value_type rst;

			{
				std::unique_lock lock{mtx};

				cond_v_consume.wait(lock, [this]{
					return !queue.empty();
				});

				rst = queue.pop_front_and_get();
			}

			cond_v_push.notify_one();

			return rst;
		}

		std::optional<value_type> consume_opt() noexcept{
			std::optional<value_type> opt;

			{
				std::unique_lock lock{mtx};

				cond_v_consume.wait(lock, [this]{
					return !queue.empty();
				});

				opt = queue.pop_front_and_get_opt();
			}

			cond_v_push.notify_one();

			return opt;
		}

		std::optional<value_type> consume(const std::stop_token& stop_token) noexcept{
			std::optional<value_type> opt;

			{
				std::unique_lock lock{mtx};

				cond_v_consume.wait(lock, [this, &stop_token]{
					return !queue.empty() || stop_token.stop_requested();
				});

				opt = queue.pop_front_and_get_opt();
			}

			cond_v_push.notify_one();

			return opt;
		}

		template <class Rep, class Period>
		std::optional<value_type> consume_for(
			const std::chrono::duration<Rep, Period>& rel_time,
			const std::stop_token& stop_token) noexcept{
			std::optional<value_type> opt{std::nullopt};

			{
				std::unique_lock lock{mtx};

				if(cond_v_consume.wait_for(lock, rel_time, [this, &stop_token]{
					return !queue.empty() || stop_token.stop_requested();
				})){
					opt = queue.pop_front_and_get_opt();
				}
			}

			cond_v_push.notify_one();

			return opt;
		}

		void notify_consumer() noexcept{
			cond_v_consume.notify_one();
		}

		bool done() const noexcept{
			std::lock_guard lock{mtx};
			return queue.empty();
		}
	};

	export
	template <typename Fn = void(), std::size_t size = 16>
	struct task_queue : private shared_queue<std::packaged_task<Fn>, size>{
		using shared_queue<std::packaged_task<Fn>>::shared_queue;
		using shared_queue<std::packaged_task<Fn>>::consume;
		using shared_queue<std::packaged_task<Fn>>::consume_opt;
		using shared_queue<std::packaged_task<Fn>>::notify_consumer;
		using shared_queue<std::packaged_task<Fn>>::done;

		using task_type = std::packaged_task<Fn>;
		using future_type = std::future<typename function_traits<Fn>::return_type>;

		template <mo_yanxi::invocable<Fn> FnTy>
			requires std::constructible_from<task_type, FnTy>
		[[nodiscard]] future_type push(FnTy&& fn){
			task_type task{std::forward<FnTy>(fn)};

			future_type future = task.get_future();
			shared_queue<task_type, size>::push(std::move(task));
			return future;
		}

		[[nodiscard]] future_type push(task_type&& task){
			future_type future = task.get_future();
			shared_queue<task_type, size>::push(std::move(task));
			return future;
		}
	};

	void foo(){
		// task_queue<int&()> q{};
		// int i;
		// int& v{i};
		// auto future = q.push([&v] -> int& {
		// 	return v;
		// });
	}
}
