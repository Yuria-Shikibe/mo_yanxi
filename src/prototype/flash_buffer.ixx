module;

#include "ext/adapted_attributes.hpp"
#include <cassert>

export module mo_yanxi.slide_window_buf;
import std;

namespace mo_yanxi{

export
template <typename T, unsigned Cap = 16, unsigned ReverseWithCount = 3>
	requires requires{
	requires ReverseWithCount >= 1;
}
struct slide_window_buffer{
	using value_type = T;
private:
	static constexpr unsigned reverse_count = ReverseWithCount;
	static constexpr unsigned capacity = Cap;
	std::array<value_type, reverse_count + capacity> buffer{};
	unsigned current_idx{};

public:
	[[nodiscard]] FORCE_INLINE slide_window_buffer() noexcept = default;
	[[nodiscard]] FORCE_INLINE explicit(false) slide_window_buffer(const value_type& initial) noexcept(std::is_nothrow_copy_constructible_v<T>) : buffer{initial, initial}, current_idx(2){

	}

	FORCE_INLINE bool push(const value_type& node) noexcept(std::is_nothrow_copy_assignable_v<T>) {
		if(!current_idx) [[unlikely]] {
			for(; current_idx < reverse_count - 1; ++current_idx){
				buffer[current_idx] = node;
			}

			return false;
		}

		buffer[current_idx++] = node;
		return current_idx == buffer.size();
	}

	FORCE_INLINE bool push_unchecked(const value_type& node) noexcept(std::is_nothrow_copy_assignable_v<T>) {
		assert(current_idx >= reverse_count - 1);

		buffer[current_idx++] = node;
		return current_idx == buffer.size();
	}


	FORCE_INLINE void advance() noexcept(std::is_nothrow_move_assignable_v<T>){
		assert(current_idx == buffer.size());
		move_reverse();
		current_idx = reverse_count;
	}

	FORCE_INLINE bool finalize() noexcept(std::is_nothrow_copy_assignable_v<T> && std::is_nothrow_move_assignable_v<T>){
		if(current_idx < 3)return false;
		if(current_idx == buffer.size()){
			move_reverse();
		}

		buffer[current_idx] = buffer[current_idx - 1];
		++current_idx;
		return true;
	}

	FORCE_INLINE auto span() const noexcept{
		return std::span{buffer.data(), current_idx};
	}

	FORCE_INLINE auto begin() const noexcept {
		return buffer.data();
	}

	FORCE_INLINE auto end() const noexcept{
		return buffer.data() + current_idx;
	}

private:
	void move_reverse() noexcept {
		if constexpr (std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>){
			if constexpr (reverse_count >= capacity){
				std::memcpy(buffer.data(), buffer.data() + buffer.size() - reverse_count, sizeof(T) * reverse_count);
			}else{
				std::memmove(buffer.data(), buffer.data() + buffer.size() - reverse_count, sizeof(T) * reverse_count);
			}
		}else{
			std::ranges::move(buffer.data() + buffer.size() - reverse_count, buffer.data() + buffer.size(), buffer.data());
		}
	}
};


struct slide_window_buffer_consumer_protocol{
	template <typename T, unsigned Cap = 16, unsigned ReverseWithCount = 3>
	void operator()(const slide_window_buffer<T, Cap, ReverseWithCount>& buf) = delete;

	template <typename T, unsigned Cap = 16, unsigned ReverseWithCount = 3>
	void push(this auto& self, slide_window_buffer<T, Cap, ReverseWithCount>& buf, const T& v) noexcept(noexcept(self(buf))){
		if(buf.push(v)){
			self(buf);
			buf.advance();
		}
	}

	template <typename T, unsigned Cap = 16, unsigned ReverseWithCount = 3>
	void finalize(this auto& self, slide_window_buffer<T, Cap, ReverseWithCount>& buf) noexcept(noexcept(self(buf))) {
		if(buf.finalize()){
			self(buf);
		}
	}
};

// template <typename Fn>
// struct slide_window_buffer_consumer{
// private:
// 	Fn fn;
//
// public:
// 	template <typename T, unsigned Cap = 16, unsigned ReverseWithCount = 3>
// 	void operator()(const slide_window_buffer<T, Cap, ReverseWithCount>& buf){
// 		fn()
// 	}
//
// };

export
template <typename T, unsigned Cap = 16, unsigned ReverseWithCount = 3>
struct slide_window_generator : public std::ranges::view_interface<slide_window_generator<T, Cap, ReverseWithCount>>{
	struct promise_type;
	using handle_type = std::coroutine_handle<promise_type>;
	using buffer_type = slide_window_buffer<T, Cap, ReverseWithCount>;

	struct cond_awaiter{
		buffer_type& buffer;
		bool is_ready{};

		[[nodiscard]] FORCE_INLINE constexpr bool await_ready() const noexcept {
			return is_ready;
		}

		FORCE_INLINE static constexpr void await_suspend(std::coroutine_handle<>) noexcept {}

		FORCE_INLINE constexpr void await_resume() noexcept{
			if(!is_ready)buffer.advance();
		}
	};

	struct promise_type{
		buffer_type buffer{};
        bool final_chunk_available{false};

		slide_window_generator get_return_object() noexcept {
			return slide_window_generator{handle_type::from_promise(*this)};
		}

		static std::suspend_never initial_suspend() noexcept { return {}; }
		static std::suspend_always final_suspend() noexcept { return {}; }
		static void unhandled_exception(){
			std::terminate();
		}

		template<std::convertible_to<typename buffer_type::value_type> From>
		FORCE_INLINE cond_awaiter yield_value(From&& from) noexcept {
			return cond_awaiter{buffer, !buffer.push(typename buffer_type::value_type{std::forward<From>(from)})};
		}

		FORCE_INLINE void return_void() noexcept{
			final_chunk_available = buffer.finalize();
		}
	};

	struct iterator{
		friend slide_window_generator;

	private:
		[[nodiscard]] explicit(false) iterator(handle_type handle)
			: handle(handle){
		}

		handle_type handle{};

	public:
		using value_type = buffer_type;

		FORCE_INLINE iterator& operator++() noexcept{
			if(!handle.done()) [[likely]] {
				handle.resume();
			} else [[unlikely]] {
				handle.promise().final_chunk_available = false;
			}
			return *this;
		}

		FORCE_INLINE void operator++(int) noexcept{
			(void)++*this;
		}

		FORCE_INLINE [[nodiscard]] const auto& operator*() const noexcept {
			return handle.promise().buffer;
		}

		FORCE_INLINE friend bool operator==(const iterator& it, std::default_sentinel_t) noexcept {
			return it.handle.done() && !it.handle.promise().final_chunk_available;
		}
	};

	~slide_window_generator(){
		if (h_) h_.destroy();
	}

	slide_window_generator(const slide_window_generator& other) = delete;

	slide_window_generator(slide_window_generator&& other) noexcept
		: h_{std::exchange(other.h_, {})}{
	}

	slide_window_generator& operator=(const slide_window_generator& other) = delete;

	slide_window_generator& operator=(slide_window_generator&& other) noexcept{
		if(this == &other) return *this;
		if (h_) h_.destroy();
		h_ = std::exchange(other.h_, {});
		return *this;
	}

	[[nodiscard]] FORCE_INLINE iterator begin(this auto& self) noexcept{
		return iterator{self.h_};
	}

	[[nodiscard]] FORCE_INLINE static std::default_sentinel_t end() noexcept{
		return std::default_sentinel;
	}

private:
	[[nodiscard]] explicit slide_window_generator(const handle_type& h) noexcept
		: h_(h){
	}

	[[nodiscard]] slide_window_generator() = default;

	handle_type h_{};
};

export
template <unsigned Cap = 16, unsigned ReverseWithCount = 3>
struct slide_window_dependent_generator{
	template <typename T>
	using type = slide_window_generator<T, Cap, ReverseWithCount>;
};

}