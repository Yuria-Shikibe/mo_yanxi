module;

#include <cassert>

export module mo_yanxi.history_stack;

import std;

namespace mo_yanxi{
	// using T = int;
	export
	template <typename T>
	struct history_stack{
		using value_type = T;
		using container_type = std::deque<T>;
		using size_type = typename container_type::size_type;
		using iterator_type = typename container_type::iterator;

	private:
		size_type capacity_{64};
		container_type entries_{};
		iterator_type current_entry_{entries_.begin()};

		[[nodiscard]] constexpr size_type distance_to_current() const noexcept{
			return std::ranges::distance(current_entry_, entries_.end());
		}

		[[nodiscard]] constexpr size_type distance_to_initial() const noexcept{
			return std::ranges::distance(entries_.begin(), current_entry_);
		}

		constexpr void relocate_current_to_last() noexcept{
			if(entries_.empty()){
				current_entry_ = entries_.begin();
			}else{
				current_entry_ = std::ranges::prev(entries_.end());
			}
		}
	public:
		[[nodiscard]] constexpr history_stack() = default;

		template <typename ...Args>
		[[nodiscard]] constexpr explicit(sizeof...(Args) == 0) history_stack(size_type capacity, Args&& ...args)
			: capacity_(capacity), entries_{std::forward<Args>(args)...}{
		}

		[[nodiscard]] constexpr bool empty() const noexcept{
			return entries_.empty();
		}

		[[nodiscard]] constexpr auto size() const noexcept{
			return entries_.size();
		}

		constexpr void push(value_type&& val) noexcept(std::is_nothrow_move_constructible_v<value_type>){
			truncate();

			entries_.push_back(std::move(val));

			if(entries_.size() > capacity_){
				entries_.pop_front();
			}

			relocate_current_to_last();
		}

		constexpr void push(const value_type& val) requires (std::is_copy_constructible_v<value_type>){
			this->push(value_type{val});
		}

		[[nodiscard]] constexpr bool has_prev() const noexcept{
			return current_entry_ != entries_.begin();
		}

		[[nodiscard]] constexpr bool has_next() const noexcept{
			return !entries_.empty() && current_entry_ != std::ranges::prev(entries_.end());
		}

		constexpr const value_type* to_prev() noexcept{
			if(current_entry_ == entries_.begin())return nullptr;
			return std::to_address(current_entry_--);
		}

		constexpr const value_type* to_next() noexcept{
			if(entries_.empty() || current_entry_ == std::ranges::prev(entries_.end())) return nullptr;
			return std::to_address(current_entry_++);
		}

		[[nodiscard]] constexpr const value_type& get() const noexcept{
			assert(!entries_.empty());
			return *current_entry_;
		}

		[[nodiscard]] constexpr const value_type* try_get() const noexcept{
			if(entries_.empty())return nullptr;
			return std::to_address(current_entry_);
		}

		constexpr void pop() noexcept{
			if(!entries_.empty()){
				entries_.pop_back();
			}

			relocate_current_to_last();
		}

		constexpr std::optional<value_type> pop_and_get() noexcept{
			if(entries_.empty()){
				return std::nullopt;
			}

			std::optional<value_type> v{std::move(entries_.back())};

			entries_.pop_back();
			relocate_current_to_last();

			return v;
		}

		constexpr void clear() noexcept{
			entries_.clear();
			current_entry_ = entries_.begin();
		}

		constexpr void truncate() noexcept{
			if(current_entry_ == entries_.end())return;
			entries_.erase(std::ranges::next(current_entry_), entries_.end());
		}
	};
}

