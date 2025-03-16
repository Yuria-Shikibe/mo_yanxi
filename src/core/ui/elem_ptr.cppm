module;

#include <cassert>

export module mo_yanxi.ui.elem_ptr;

import std;
import mo_yanxi.ui.pre_decl;
import mo_yanxi.owner;
import mo_yanxi.meta_programming;
import mo_yanxi.func_initialzer;


namespace mo_yanxi::ui{
	// export struct elem;

	export
	template <typename Fn>
	concept elem_init_func = func_initializer_of<Fn, elem>;

	export
	template <typename Fn>
	concept invocable_elem_init_func = invocable_func_initializer_of<Fn, elem>;

	export
	template <typename InitFunc>
	struct elem_init_func_trait : protected func_initializer_trait<InitFunc>{
		using elem_type = typename func_initializer_trait<InitFunc>::target_type;
	};

	export
	struct elem_ptr{
		[[nodiscard]] constexpr elem_ptr() = default;

		[[nodiscard]] constexpr explicit elem_ptr(const owner<elem*> element)
			: element{element}{}

		template <invocable_elem_init_func InitFunc>
		[[nodiscard]] elem_ptr(scene* scene, group* group, InitFunc initFunc)
			: elem_ptr{scene, group, std::in_place_type<typename elem_init_func_trait<InitFunc>::elem_type>}{
			std::invoke(initFunc,
			            static_cast<std::add_lvalue_reference_t<typename elem_init_func_trait<InitFunc>::elem_type>>(*element));
		}

		template <typename T, typename ...Args>
			requires (std::constructible_from<T, scene*, group*, Args...>)
		[[nodiscard]] elem_ptr(scene* scene, group* group, std::in_place_type_t<T>, Args&& ...args)
			: element{new T{scene, group, std::forward<Args>(args)...}}{
			if(group){
				check_group_set();
			}

			if(scene){
				check_scene_set();
			}
		}

		constexpr elem& operator*() const noexcept{
			assert(element != nullptr && "dereference on a null element");
			return *element;
		}

		constexpr elem* operator->() const noexcept{
			return element;
		}

		explicit constexpr operator bool() const noexcept{
			return element != nullptr;
		}

		[[nodiscard]] constexpr elem* get() const noexcept{
			return element;
		}

		[[nodiscard]] constexpr owner<elem*> release() noexcept{
			return std::exchange(element, nullptr);
		}

		constexpr void reset() noexcept{
			this->operator=(elem_ptr{});
		}

		constexpr void reset(const owner<elem*> e) noexcept{
			this->operator=(elem_ptr{e});
		}

		~elem_ptr();

		constexpr friend bool operator==(const elem_ptr& lhs, const elem_ptr& rhs) noexcept = default;

		constexpr bool operator==(std::nullptr_t) const noexcept{
			return element != nullptr;
		}

		elem_ptr(const elem_ptr& other) = delete;

		constexpr elem_ptr(elem_ptr&& other) noexcept
			: element{other.release()}{
		}

		constexpr elem_ptr& operator=(elem_ptr&& other) noexcept{
			if(this == &other)return *this;
			std::swap(this->element, other.element);
			return *this;
		}

	private:
		owner<elem*> element{};

		void check_group_set() const noexcept;
		void check_scene_set() const noexcept;
	};
}
