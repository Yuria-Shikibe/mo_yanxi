module;

#include <cassert>
#define EMPTY_BUFFER_INIT /*[[indeterminate]]*/

export module mo_yanxi.data_storage;

import std;

namespace mo_yanxi::react_flow{

export
template <typename T>
struct data_package{
	static_assert(std::is_object_v<T>);

private:
	union U{
		const T* ref_ptr{};
		T local;

		[[nodiscard]] constexpr U(){

		}

		[[nodiscard]] constexpr U(const T* r) noexcept : ref_ptr{r}{

		}

		constexpr ~U(){

		}
	};

	bool is_local{false};
	U u;

public:

	[[nodiscard]] constexpr T* get_mut() noexcept{
		if(!is_local)return nullptr;
		return std::addressof(u.local);
	}

	[[nodiscard]] constexpr const T* get() const noexcept{
		return is_local ? std::addressof(u.local) : u.ref_ptr;
	}


	[[nodiscard]] constexpr std::optional<T> fetch() const noexcept(std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_move_constructible_v<T>){
		if(is_local){
			return std::optional<T>{std::move(u.local)};
		}else if(u.ref_ptr){
			return std::optional<T>{*u.ref_ptr};
		}
		return std::nullopt;
	}

	[[nodiscard]] constexpr std::optional<T> clone() const noexcept(std::is_nothrow_copy_constructible_v<T>){
		if(is_local){
			return std::optional<T>{u.local};
		}else if(u.ref_ptr){
			return std::optional<T>{*u.ref_ptr};
		}
		return std::nullopt;
	}

	[[nodiscard]] constexpr bool empty() const noexcept{
		return !is_local && u.ref_ptr == nullptr;
	}

	constexpr explicit operator bool() const noexcept{
		return !empty();
	}

	[[nodiscard]] constexpr data_package() noexcept = default;

	[[nodiscard]] constexpr explicit data_package(const T& to_ref) noexcept : is_local(false), u{std::addressof(to_ref)}{

	}

	template <typename ...Args>
		requires (std::constructible_from<T, Args&&...>)
	[[nodiscard]] constexpr explicit data_package(std::in_place_t, Args&& ...args)
		noexcept(std::is_nothrow_constructible_v<T, Args&&...>)
	: is_local(true){
		std::construct_at(std::addressof(u.local), std::forward<Args>(args)...);
	}

	constexpr void reset() noexcept{
		if(is_local){
			std::destroy_at(std::addressof(u.local));
			is_local = false;
		}
		u.ref_ptr = nullptr;
	}

private:
	constexpr void reset_unchecked() noexcept{
		assert(is_local);
		std::destroy_at(std::addressof(u.local));
		is_local = false;
		u.ref_ptr = nullptr;
	}
public:

	constexpr ~data_package(){
		if(is_local){
			std::destroy_at(std::addressof(u.local));
		}
	}

	constexpr data_package(const data_package& other) noexcept(std::is_nothrow_copy_constructible_v<T>)
	: is_local{other.is_local}{
		if(is_local){
			std::construct_at(std::addressof(u.local), other.u.local);
		}else{
			u.ref_ptr = other.u.ref_ptr;
		}
	}

	constexpr data_package(data_package&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
	: is_local{other.is_local}{
		if(is_local){
			std::construct_at(std::addressof(u.local), std::move(other.u.local));
			other.reset_unchecked();
		}else{
			u.ref_ptr = other.u.ref_ptr;
		}
	}

	constexpr data_package& operator=(const data_package& other) noexcept(std::is_nothrow_copy_assignable_v<T>) {
		if(this == &other) return *this;

		if(is_local && other.is_local){
			u.local = other.u.local;
		}else if(other.is_local){
			std::construct_at(std::addressof(u.local), other.u.local);
			is_local = true;
		}else if(is_local){
			std::destroy_at(std::addressof(u.local));
			is_local = false;
			u.ref_ptr = other.u.ref_ptr;
		}else{
			u.ref_ptr = other.u.ref_ptr;
		}

		return *this;
	}

	constexpr data_package& operator=(data_package&& other) noexcept(std::is_nothrow_move_assignable_v<T>){
		if(this == &other) return *this;

		if(is_local && other.is_local){
			u.local = std::move(other.u.local);
			other.reset_unchecked();
		}else if(other.is_local){
			std::construct_at(std::addressof(u.local), std::move(other.u.local));
			is_local = true;
			other.reset_unchecked();
		}else if(is_local){
			std::destroy_at(std::addressof(u.local));
			is_local = false;
			u.ref_ptr = other.u.ref_ptr;
		}else{
			u.ref_ptr = other.u.ref_ptr;
		}

		return *this;
	}
};

export
template <typename T>
struct data_package_optimal{
	static constexpr bool is_small_object = (std::is_trivially_copyable_v<T> && sizeof(T) <= 16);
	using value_type = std::conditional_t<is_small_object, T, data_package<T>>;

private:
	value_type storage_;

public:
	[[nodiscard]] T* get_mut() noexcept{
		if constexpr (is_small_object){
			return std::addressof(storage_);
		}else{
			return storage_.get_mut();
		}
	}

	[[nodiscard]] const T* get() const noexcept{
		if constexpr (is_small_object){
			return std::addressof(storage_);
		}else{
			return storage_.get();
		}
	}


	[[nodiscard]] std::optional<T> fetch() const noexcept(std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_move_constructible_v<T>){
		if constexpr (is_small_object){
			return std::optional<T>{std::move(storage_)};
		}else{
			return storage_.fetch();
		}
	}

	[[nodiscard]] std::optional<T> clone() const noexcept(std::is_nothrow_copy_constructible_v<T>){
		if constexpr (is_small_object){
			return std::optional<T>{storage_};
		}else{
			return storage_.clone();
		}
	}

	[[nodiscard]] bool empty() const noexcept{
		if constexpr (is_small_object){
			return false;
		}else{
			return storage_.empty();
		}
	}

	explicit operator bool() const noexcept{
		return !empty();
	}

	[[nodiscard]] data_package_optimal() = default;

	[[nodiscard]] explicit data_package_optimal(const T& to_ref) noexcept : storage_(to_ref){

	}

	template <typename ...Args>
		requires (std::constructible_from<T, Args&&...>)
	[[nodiscard]] explicit data_package_optimal(std::in_place_t, Args&& ...args)
		noexcept(std::is_nothrow_constructible_v<T, Args&&...>) requires (is_small_object)
	: storage_(std::forward<Args>(args)...){
	}

	template <typename ...Args>
		requires (std::constructible_from<T, Args&&...>)
	[[nodiscard]] explicit data_package_optimal(std::in_place_t, Args&& ...args)
		noexcept(std::is_nothrow_constructible_v<T, Args&&...>) requires (!is_small_object)
	: storage_(std::in_place, std::forward<Args>(args)...){
	}

};

//Legacy
template <typename T>
struct data_storage_view;

//Legacy
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
		requires (std::constructible_from<T, Args&&...>)
	[[nodiscard]] data_storage(std::in_place_type_t<T>, Args&& ...args) :
		destructor_func(+[](void* p) static noexcept{
			delete static_cast<T*>(p);
		}), reference_ptr(new T{std::forward<Args&&>(args)}){
	}

	template <typename T, typename ...Args>
		requires (sizeof(T) <= local_storage_size && alignof(T) <= local_storage_align && std::constructible_from<T, Args&&...>)
	[[nodiscard]] data_storage(std::in_place_type_t<T>, Args&& ...args) noexcept(std::is_nothrow_constructible_v<T, Args&&...>) :
		destructor_func(+[](void* p) static noexcept{
			std::destroy_at(static_cast<T*>(p));
		}), reference_ptr(local_buf_){
		std::construct_at(static_cast<T*>(reference_ptr), std::forward<Args&&>(args)...);
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
				//TODO check double move?
				return std::optional<T>{std::move(*ptr)};
			}else{
				return std::optional<T>{*ptr};
			}
		}
		return std::nullopt;
	}
	
	[[nodiscard]] std::optional<T> clone() const noexcept(std::is_nothrow_copy_constructible_v<T>){
		if(ptr){
			return std::optional<T>{*ptr};
		}
		return std::nullopt;
	}
};

}