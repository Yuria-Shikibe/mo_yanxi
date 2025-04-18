export module mo_yanxi.concepts;

import std;

export import mo_yanxi.meta_programming;

namespace mo_yanxi {
	export
	template <typename T>
	concept small_object = requires{
		requires sizeof(T) <= sizeof(void*) * 2;
	};

	/**
	 * \brief coonditional variant but friendly to IDEs
	 */
	export
	template <bool Test, class T>
	struct conditional_reference {
		using type = std::add_lvalue_reference_t<T>;
	};

	template <class T>
	struct conditional_reference<false, T> {
		using type = T;
	};

	/**
	 * \brief Decide whether to use reference or value version of the given of the given template classes
	 * \tparam T Value Type
	 */
	export template <typename T, size_t size>
	using conditional_pass_type = typename conditional_reference<
		(size > sizeof(void*) * 2),
		T
	>::type;

    export template <typename T>
    concept number = std::is_arithmetic_v<T>;

    export template <class DerivedT, class Base>
    concept derived = std::derived_from<DerivedT, Base>;

    export template <class DerivedT, class... Bases>
    concept multi_derived = (std::derived_from<Bases, DerivedT> && ...);

	export template <class T>
	concept DefConstructable = std::is_default_constructible_v<T>;

	export template <typename T, typename functype>
	concept invocable = function_traits<functype>::template is_invocable<T>;

	// export template <typename T, typename functype>
	// concept InvokableVoid = function_traits<functype>::template invocable_as_v<T>();

	// export template <typename T, typename functype>
	// concept InvokeNullable = std::same_as<std::nullptr_t, T> || invocable<T, functype>;

	// export template <typename T, typename functype>
	// concept InvokableFunc = std::is_convertible_v<T, std::function<functype>>;

	export template <typename T>
	concept enum_type = std::is_enum_v<T>;

	export template <typename T>
	concept non_negative = std::is_unsigned_v<T>;

	export template <typename T>
	concept signed_number = !std::is_unsigned_v<T> && number<T>;

	// constexpr bool b = std::is_unsigned_v<float>;

	export template <typename T, typename ItemType = std::nullptr_t>
	concept range_of = requires{
		requires std::ranges::range<T>;
		requires std::same_as<ItemType, std::nullptr_t> || std::same_as<std::ranges::range_value_t<T>, ItemType>;
	};

	// export template <typename Callable>
	// concept InvokeNoexcept = noexcept(Callable()) || noexcept(std::declval<Callable>()());

	// export template <typename T, typename NumberType = float>
	// concept pos = requires(T t){
	// 	std::is_base_of_v<decltype(t.getX()), NumberType>;
	// 	std::is_base_of_v<decltype(t.getY()), NumberType>;
	// };


	template <template <class...> class Template, class... Args>
	// ReSharper disable once CppFunctionIsNotImplemented
	void derived_from_specialization_impl(const Template<Args...>&);

	export
	template <class T, template <class...> class Template>
	concept spec_of = requires(const T& obj) {
		mo_yanxi::derived_from_specialization_impl<Template>(obj);
	};

	export
	template <class T, template <class...> class Template>
	concept decayed_spec_of = spec_of<std::decay_t<T>, Template>;

	export
	template <class T, template <class...> class Template>
	struct is_spec_of : std::bool_constant<spec_of<T, Template>>{};




    template <typename T>
    concept complete_type = requires{
        sizeof(T);
    };
}