module;

#include <cassert>
#include "../adapted_attributes.hpp"

export module mo_yanxi.referenced_ptr;

import ext.cond_atomic;
import std;

namespace mo_yanxi{

export struct no_deletion_on_ref_count_to_zero{};

template <typename T>
constexpr bool no_delete_on_drop = std::is_same_v<T, no_deletion_on_ref_count_to_zero>;

export
template <typename T, typename D = std::default_delete<T>>
struct referenced_ptr;

template <typename T, typename D>
struct referenced_ptr{
	using element_type = std::remove_const_t<T>;
	using pointer = T*;

	[[nodiscard]] constexpr referenced_ptr() = default;

	[[nodiscard]] constexpr explicit(false) referenced_ptr(pointer object) : object{object}{
		if(this->object) incr();
	}

	[[nodiscard]] constexpr explicit(false) referenced_ptr(pointer object, D&& deleter) : object{object}, deleter{std::move(deleter)}{
		if(this->object) incr();
	}

	[[nodiscard]] constexpr explicit(false) referenced_ptr(pointer object, const D& deleter) : object{object}, deleter{deleter}{
		if(this->object) incr();
	}

	template <typename... Args>
	[[nodiscard]] explicit constexpr referenced_ptr(std::in_place_t, Args&&... args) : referenced_ptr{
			new element_type{std::forward<Args>(args)...}
		}{
	}

private:
	void delete_elem(pointer t) noexcept {
		std::invoke(deleter, t);
	}

	void decr() noexcept;

	void incr() noexcept;

public:
	template <typename L>
		requires (std::derived_from<T, L>)
	explicit(false) operator referenced_ptr<L>() const noexcept requires(std::same_as<D, std::default_delete<T>> && std::has_virtual_destructor_v<L>){
		return referenced_ptr<L>{object};
	}

	explicit constexpr operator bool() const noexcept{
		return object != nullptr;
	}

	constexpr T& operator*() const noexcept{
		assert(object != nullptr);
		return *object;
	}

	constexpr T* operator->() const noexcept{
		return object;
	}

	constexpr T* get() const noexcept{
		return object;
	}

	constexpr void reset(T* ptr = nullptr) noexcept{
		if(object){
			decr();
		}

		object = ptr;
		if(object) incr();
	}

	constexpr ~referenced_ptr() noexcept{
		if(object){
			decr();
		}
	}

	constexpr referenced_ptr(const referenced_ptr& other) noexcept
		: object{other.object}{
		if(object) incr();
	}

	constexpr referenced_ptr(referenced_ptr&& other) noexcept
		: object{std::exchange(other.object, {})}{
	}

	constexpr referenced_ptr& operator=(const referenced_ptr& other){
		if(this == &other) return *this;
		if(this->object == other.object) return *this;

		this->reset(other.object);

		return *this;
	}

	constexpr referenced_ptr& operator=(referenced_ptr&& other) noexcept{
		if(this == &other) return *this;
		std::swap(object, other.object);
		return *this;
	}

	constexpr bool operator==(std::nullptr_t) const noexcept{
		return object == nullptr;
	}

	constexpr bool operator==(const referenced_ptr&) const noexcept = default;

	constexpr friend bool operator==(const T* p, const referenced_ptr& self) noexcept{
		return p == self.get();
	}

	constexpr friend bool operator==(const referenced_ptr& self, const T* p) noexcept{
		return self.get() == p;
	}


private:
	T* object{};
	ADAPTED_NO_UNIQUE_ADDRESS D deleter{};
};



/**
 * @brief Specify that lifetime of a style is NOT automatically managed by reference count.
 */
constexpr inline std::size_t persistent_spec = std::dynamic_extent;

namespace tags{
export
struct persistent_tag_t{
};

export constexpr inline persistent_tag_t persistent{};

}


export
struct referenced_object{

private:
	std::size_t reference_count_{};

public:
	[[nodiscard]] constexpr referenced_object() = default;

	constexpr referenced_object(const referenced_object& other) noexcept{
	}

	constexpr referenced_object& operator=(const referenced_object& other){
		return *this;
	}

	constexpr referenced_object(referenced_object&& other) noexcept
		: reference_count_{std::exchange(other.reference_count_, {})}{
	}

	constexpr referenced_object& operator=(referenced_object&& other) noexcept{
		if(this == &other) return *this;
		std::swap(reference_count_, other.reference_count_);
		return *this;
	}

protected:

	[[nodiscard]] constexpr std::size_t ref_count() const noexcept{
		return reference_count_;
	}

	[[nodiscard]] constexpr bool droppable() const noexcept{
		return reference_count_ == 0;
	}

	/**
	 * @brief take reference
	 */
	constexpr void ref_incr() noexcept{
		++reference_count_;
	}

	/**
	 * @brief drop referebce
	 * @return true if should be destructed
	 */
	constexpr bool ref_decr() noexcept{
		--reference_count_;
		return reference_count_ == 0;
	}


	template <typename T, typename D>
	friend struct referenced_ptr;
};


export
struct referenced_object_persistable{

private:
	mutable std::size_t reference_count_{};

public:
	[[nodiscard]] constexpr referenced_object_persistable() = default;

	[[nodiscard]] constexpr explicit referenced_object_persistable(tags::persistent_tag_t)
		: reference_count_(persistent_spec){
	}

	constexpr referenced_object_persistable(const referenced_object_persistable& other) noexcept{
	}

	constexpr referenced_object_persistable& operator=(const referenced_object_persistable& other){
		return *this;
	}

	constexpr referenced_object_persistable(referenced_object_persistable&& other) noexcept
		: reference_count_{std::exchange(other.reference_count_, {})}{
	}

	constexpr referenced_object_persistable& operator=(referenced_object_persistable&& other) noexcept{
		if(this == &other) return *this;
		std::swap(reference_count_, other.reference_count_);
		return *this;
	}

protected:

	[[nodiscard]] constexpr std::size_t ref_count() const noexcept{
		return reference_count_;
	}

	[[nodiscard]] constexpr bool droppable() const noexcept{
		return reference_count_ == 0;
	}

	/**
	 * @brief take reference
	 */
	constexpr void ref_incr() const noexcept{
		if(is_persistent())return;
		++reference_count_;
		assert(reference_count_ != persistent_spec && "yes i know this should be impossible, just in case");
	}

	/**
	 * @brief drop referebce
	 * @return true if should be destructed
	 */
	constexpr bool ref_decr() const noexcept{
		if(is_persistent())return false;
		--reference_count_;
		return reference_count_ == 0;
	}

	[[nodiscard]] constexpr bool is_persistent() const noexcept{
		return reference_count_ == persistent_spec;
	}

	template <typename T, typename D>
	friend struct referenced_ptr;
};

template <typename T, typename D>
void referenced_ptr<T, D>::decr() noexcept{
	if constexpr(!no_delete_on_drop<D>){
		if(object->ref_decr()){
			this->delete_elem(object);
		}
	} else{
		assert(object->ref_count() > 0);
		object->ref_decr();
	}
}

template <typename T, typename D>
void referenced_ptr<T, D>::incr() noexcept{
	object->ref_incr();
}
}

