module;

#include <cassert>

export module mo_yanxi.gui.infrastructure:elem_ptr;


import mo_yanxi.func_initialzer;
import mo_yanxi.concepts;
import std;

namespace mo_yanxi::gui{

export struct elem;
export struct scene;

export
template <typename Fn>
concept elem_init_func = func_initializer_of<Fn, elem>;

export
template <typename Fn>
concept invocable_elem_init_func = invocable_func_initializer_of<Fn, elem>;

export
template <typename InitFunc>
struct elem_init_func_trait : protected func_initializer_trait<std::remove_cvref_t<InitFunc>>{
	using elem_type = typename func_initializer_trait<std::remove_cvref_t<InitFunc>>::target_type;
};

export
struct elem_ptr{
	[[nodiscard]] elem_ptr() = default;

	[[nodiscard]] explicit elem_ptr(elem* element)
		: element{element}{
	}

	template <typename InitFunc>
		requires (!spec_of<InitFunc, std::in_place_type_t> && invocable_elem_init_func<InitFunc>)
	[[nodiscard]] elem_ptr(scene& scene, elem* parent, InitFunc initFunc)
		: elem_ptr{
			std::addressof(scene), parent, std::in_place_type<typename elem_init_func_trait<InitFunc>::elem_type>
		}{
		std::invoke(initFunc,
		            static_cast<std::add_lvalue_reference_t<typename elem_init_func_trait<InitFunc>::elem_type>>(*
			            element));
	}

	template <typename T, typename... Args>
		requires (std::constructible_from<T, scene&, elem*, Args&&...>)
	[[nodiscard]] elem_ptr(scene& scene, elem* group, std::in_place_type_t<T>, Args&&... args)
		: element{elem_ptr::new_elem<T>(scene, group, std::forward<Args>(args)...)}{
	}

	elem& operator*() const noexcept{
		assert(element != nullptr && "dereference on a null element");
		return *element;
	}

	elem* operator->() const noexcept{
		return element;
	}

	explicit operator bool() const noexcept{
		return element != nullptr;
	}

	[[nodiscard]] elem* get() const noexcept{
		return element;
	}

	[[nodiscard]] elem* release() noexcept{
		return std::exchange(element, nullptr);
	}

	void reset() noexcept{
		this->operator=(elem_ptr{});
	}

	void reset(elem* e) noexcept{
		this->operator=(elem_ptr{e});
	}

	~elem_ptr(){
		if(element) delete_elem(element);
	}

	friend bool operator==(const elem_ptr& lhs, const elem_ptr& rhs) noexcept = default;

	bool operator==(std::nullptr_t) const noexcept{
		return element == nullptr;
	}

	elem_ptr(const elem_ptr& other) = delete;

	elem_ptr(elem_ptr&& other) noexcept
		: element{other.release()}{
	}

	elem_ptr& operator=(elem_ptr&& other) noexcept{
		if(this == &other) return *this;
		if(element) delete_elem(element);
		this->element = other.release();
		return *this;
	}

private:
	elem* element{};

	template <typename T, typename... Args>
	static T* new_elem(scene& scene, elem* parent, Args&&... args){
		T* p = alloc_of(scene).new_object<T>(scene, parent, std::forward<Args>(args)...);
		elem_ptr::set_deleter(p, +[](elem* e) noexcept {
			alloc_of(e).delete_object(static_cast<T*>(e));
		});
		return p;
	}

	static std::pmr::polymorphic_allocator<> alloc_of(const scene& s) noexcept;

	static std::pmr::polymorphic_allocator<> alloc_of(const elem* ptr) noexcept;

	static void set_deleter(elem* element, void(*p)(elem*) noexcept) noexcept;

	static void delete_elem(elem* ptr) noexcept;
};

}
