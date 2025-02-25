//
// Created by Matrix on 2024/12/21.
//

export module ext.history_stack;

import std;

export namespace mo_yanxi{
	template <typename T>
	struct history_stack{
		using container_type = std::deque<T>;
		using size_type = typename container_type::size_type;
		using value_type = typename container_type::value_type;
	private:
		container_type data_{};
		size_type currentIndex_{};
	public:

		constexpr void clear() noexcept{
			data_.clear();
			currentIndex_ = 0;
		}

		void truncate() noexcept{
			data_.resize(currentIndex_);
		}

		[[nodiscard]] constexpr bool empty() const noexcept{
			return data_.empty();
		}

		[[nodiscard]] constexpr size_type size() const noexcept{
			return data_.size();
		}

		[[nodiscard]] constexpr size_type current_index() const noexcept{
			return currentIndex_;
		}

		[[nodiscard]] value_type* current() noexcept{
			if(currentIndex_ != 0){
				return &data_[currentIndex_ - 1];
			}

			return nullptr;
		}

		[[nodiscard]] value_type* current() const noexcept{
			return const_cast<history_stack*>(this)->current();
		}

		constexpr void push(const value_type& item){
			data_.resize(currentIndex_);
			data_.push_back(item);
			currentIndex_ = data_.size();
		}

		constexpr void push(value_type&& item){
			data_.resize(currentIndex_);
			data_.push_back(std::move(item));
			currentIndex_ = data_.size();
		}

		constexpr void pop() noexcept{
			if(data_.empty())return;

			data_.pop_back();
			currentIndex_ = std::min(currentIndex_, static_cast<size_type>(data_.size()));
		}

		constexpr std::optional<value_type> pop_and_get() noexcept{
			if(data_.empty())return std::nullopt;
			value_type tmp = std::move(data_.back());

			pop();

			return std::optional<value_type>{std::move_if_noexcept(tmp)};
		}

		constexpr value_type* backtrace() noexcept{
			if(currentIndex_ == 0)return nullptr;

			--currentIndex_;
			return &data_[currentIndex_];
		}

		constexpr value_type* backtrace_skip_top() noexcept{
			if(currentIndex_ == 0)return nullptr;

			if(currentIndex_ == data_.size())--currentIndex_;

			if(currentIndex_ == 0)return nullptr;

			--currentIndex_;
			return &data_[currentIndex_];
		}

		constexpr value_type* forwardtrace() noexcept{
			if(currentIndex_ == data_.size())return nullptr;

			auto p = &data_[currentIndex_];
			++currentIndex_;
			return p;
		}

		constexpr value_type* forwardtrace_skip_current() noexcept{
			if(currentIndex_ == data_.size())return nullptr;

			++currentIndex_;

			if(currentIndex_ == data_.size())return nullptr;

			return &data_[currentIndex_];
		}

		[[nodiscard]] constexpr bool redo_able() const noexcept{
			return currentIndex_ != data_.size() && (currentIndex_ + 1) != data_.size();
		}

		[[nodiscard]] constexpr bool undo_able() const noexcept{
			return currentIndex_ != 0;
		}
	};
}


