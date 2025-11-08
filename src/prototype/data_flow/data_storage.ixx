module;

#define EMPTY_BUFFER_INIT /*[[indeterminate]]*/

export module mo_yanxi.data_storage;

import std;

namespace mo_yanxi::data_flow{
export
template <typename T>
struct data_storage_view;

export
template <std::size_t Size = 32, std::size_t Align = alignof(std::max_align_t)>
struct data_storage{
	static constexpr std::size_t local_storage_size{Size};
	static constexpr std::size_t local_storage_align{std::max(Align, alignof(void*))};

	template <typename T>
	friend struct data_storage_view;
private:
	//State0: empty (all null)
	//State1: by_reference(no owning ship) (dctr_ptr == null, ref != null)
	//State2: owner/local
	//State3: owner/heap

	void(* destructor_func)(void*) noexcept = nullptr;
	void* reference_ptr{};

	alignas(local_storage_align) std::byte local_buf_[local_storage_size] EMPTY_BUFFER_INIT;

public:
	[[nodiscard]] bool empty() const noexcept{
		return destructor_func == nullptr && reference_ptr == nullptr;
	}

	explicit operator bool() const noexcept{
		return !empty();
	}

	[[nodiscard]] bool is_owner() const noexcept{
		return destructor_func != nullptr;
	}

	template <typename T>
	[[nodiscard]] T* get_mut() noexcept{
		if(!is_owner())return nullptr;
		return static_cast<T*>(reference_ptr);
	}

	template <typename T>
	[[nodiscard]] const T* get() const noexcept{
		if(empty())return nullptr;
		return static_cast<const T*>(reference_ptr);
	}

	[[nodiscard]] data_storage() = default;

	template <typename T>
	[[nodiscard]] explicit data_storage(const T& to_ref) :
		reference_ptr(const_cast<void*>(static_cast<const void*>(std::addressof(to_ref)))){}

	template <typename T, typename ...Args>
		requires (sizeof(T) <= local_storage_size && alignof(T) <= local_storage_align && std::constructible_from<T, Args&&...>)
	[[nodiscard]] data_storage(std::in_place_type_t<T>, Args&& ...args) noexcept(std::is_nothrow_constructible_v<T, Args&&...>) :
		destructor_func(+[](void* p) static noexcept{
			std::destroy_at(static_cast<T*>(p));
		}), reference_ptr(local_buf_){
		std::construct_at(static_cast<T*>(reference_ptr), std::forward<Args&&>(args)...);
	}

	template <typename T, typename ...Args>
		requires (std::constructible_from<T, Args&&...>)
	[[nodiscard]] data_storage(std::in_place_type_t<T>, Args&& ...args) :
		destructor_func(+[](void* p) static noexcept{
			delete static_cast<T*>(p);
		}), reference_ptr(new T{std::forward<Args&&>(args)}){
	}

	~data_storage(){
		if(is_owner()){
			destructor_func(reference_ptr);
		}
	}

	data_storage(const data_storage& other) = delete;
	data_storage(data_storage&& other) noexcept = delete;
	data_storage& operator=(const data_storage& other) = delete;
	data_storage& operator=(data_storage&& other) noexcept = delete;
};

template <typename T>
struct data_storage_view{
private:
	bool storage_is_owner_{};
	T* ptr{};

public:
	[[nodiscard]] data_storage_view() noexcept = default;

	template <std::size_t Size, std::size_t Align>
	[[nodiscard]] explicit(false) data_storage_view(data_storage<Size, Align>& storage) noexcept :
	storage_is_owner_{storage.is_owner()}, ptr{static_cast<T*>(storage.reference_ptr)}{}


	[[nodiscard]] T* get_mut() noexcept{
		if(!storage_is_owner_)return nullptr;
		return static_cast<T*>(ptr);
	}

	[[nodiscard]] const T* get() const noexcept{
		return static_cast<const T*>(ptr);
	}

	[[nodiscard]] std::optional<T> fetch() const noexcept(std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_move_constructible_v<T>){
		if(ptr){
			if(storage_is_owner_){
				return std::optional<T>{std::move(*ptr)};
			}else{
				return std::optional<T>{*ptr};
			}
		}
		return std::nullopt;
	}
};

}