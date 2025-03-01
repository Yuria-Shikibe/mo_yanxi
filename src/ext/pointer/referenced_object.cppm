module;

#include <cassert>
#include "../adapted_attributes.hpp"
export module mo_yanxi.referenced_ptr;

import ext.cond_atomic;
import std;

namespace mo_yanxi{
	export
	template <typename T>
	struct referenced_ptr{
		using element_type = T;
		using pointer = T*;

		[[nodiscard]] constexpr referenced_ptr() = default;

		[[nodiscard]] constexpr explicit referenced_ptr(T* object);

		template <typename ...Args>
		[[nodiscard]] explicit constexpr referenced_ptr(std::in_place_t, Args&&... args) : referenced_ptr{
			new T{std::forward<Args>(args)...}
		}{}

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
				if(object->decr_ref()){
					delete object;
				}
			}

			object = ptr;
			if(object)object->incr_ref();
		}

		constexpr ~referenced_ptr() noexcept;

		referenced_ptr(const referenced_ptr& other) noexcept
			: object{other.object}{
			if(object) object->incr_ref();
		}

		referenced_ptr(referenced_ptr&& other) noexcept
			: object{std::exchange(other.object, {})}{}

		referenced_ptr& operator=(const referenced_ptr& other){
			if(this == &other) return *this;
			if(this->object == other.object) return *this;

			this->reset(other.object);

			return *this;
		}

		referenced_ptr& operator=(referenced_ptr&& other) noexcept{
			if(this == &other) return *this;
			std::swap(object, other.object);
			return *this;
		}

		constexpr bool operator==(std::nullptr_t) const noexcept{
			return object == nullptr;
		}

		constexpr bool operator==(const referenced_ptr&) const noexcept = default;

		template <typename T1>
			requires std::is_convertible_v<T*, T1*>
		constexpr explicit(false) operator referenced_ptr<T1>() const noexcept{
			return referenced_ptr<T1>{object};
		}

		/*
		template <typename T1>
		constexpr referenced_ptr<T1, Deleter> dynamic_cast_to() const noexcept{
			if constexpr (std::derived_from<std::remove_cvref_t<T>, std::remove_cvref_t<T1>>){
				return referenced_ptr<T1, Deleter>{object};
			}else{
				return referenced_ptr<T1, Deleter>{dynamic_cast<T1*>(object)};
			}
		}*/

	private:
		T* object{};

		template <typename Ty>
		friend struct referenced_ptr;
	};

	export
	template <bool isAtomic = false>
	struct referenced_object{
		[[nodiscard]] constexpr std::size_t get_ref_count() const noexcept{
			return reference_count;
		}

		[[nodiscard]] constexpr referenced_object() = default;

		//TODO delete these?
		constexpr referenced_object(const referenced_object& other) noexcept requires (!isAtomic){
		}

		constexpr referenced_object& operator=(const referenced_object& other) requires (!isAtomic){
			return *this;
		}

		referenced_object(referenced_object&& other) noexcept requires (!isAtomic)
			: reference_count{std::exchange(other.reference_count, {})}{
		}

		referenced_object& operator=(referenced_object&& other) noexcept requires (!isAtomic){
			if(this == &other) return *this;
			std::swap(reference_count, other.reference_count);
			return *this;
		}

	protected:
		cond_atomic<std::size_t, isAtomic> reference_count{};

		constexpr bool droppable() const noexcept{
			return reference_count == 0;
		}

		/**
		 * @brief take reference
		 */
		constexpr void incr_ref() noexcept{
			if constexpr (isAtomic){
				reference_count.fetch_add(1, std::memory_order_relaxed);
			}else{
				++reference_count;
			}
		}

		/**
		 * @brief drop referebce
		 * @return true if should be destructed
		 */
		constexpr bool decr_ref() noexcept{
			if constexpr (isAtomic){
				auto prev = reference_count.fetch_sub(1, std::memory_order_acq_rel);
				return prev == 1;
			}else{
				//TODO should this marked as illegal?
				if(reference_count == 0){
					return false;
				}

				--reference_count;
				return reference_count == 0;
			}
		}

		template <typename T>
		friend struct referenced_ptr;
	};
}

module : private;


template <typename T>
constexpr mo_yanxi::referenced_ptr<T>::referenced_ptr(T* object): object{object}{
	if(this->object)this->object->incr_ref();
}

template <typename T>
constexpr mo_yanxi::referenced_ptr<T>::~referenced_ptr() noexcept{
	if(object){
		if(object->decr_ref()){
			delete object;
			object = nullptr;
		}
	}
}