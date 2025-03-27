export module ext.guard;

import mo_yanxi.meta_programming;
import std;

export namespace mo_yanxi{
	template <typename T, bool passByMove = false>
		requires requires{
			requires
			(passByMove && std::is_move_assignable_v<T>) ||
			(!passByMove && std::is_copy_assignable_v<T>);
		}
	class [[deprecated]] [[jetbrains::guard]] guard{
		T& tgt;
		T original;

	public:
		[[nodiscard]] constexpr guard(T& tgt, const T& data) requires (!passByMove) : tgt{tgt}, original{tgt}{
			this->tgt = data;
		}

		[[nodiscard]] constexpr guard(T& tgt, T&& data) requires (passByMove) : tgt{tgt}, original{std::move(tgt)}{
			this->tgt = std::move(data);
		}

		constexpr ~guard(){
			if constexpr(passByMove){
				tgt = std::move(original);
			} else{
				tgt = original;
			}
		}
	};

	template <typename T, bool passByMove = !(std::is_trivially_copy_assignable_v<T> && std::is_copy_assignable_v<T>)>
		requires requires{
			requires std::is_object_v<T>;
			requires
			(passByMove && std::is_move_assignable_v<T>) ||
			(!passByMove && std::is_copy_assignable_v<T>);
		}
	class [[jetbrains::guard]] resumer{
		T& tgt;
		T original;

		static constexpr bool is_nothrow =
			(passByMove && std::is_nothrow_move_assignable_v<T>) ||
			(!passByMove && std::is_nothrow_copy_assignable_v<T>);

	public:
		[[nodiscard]] constexpr explicit resumer(T& tgt) noexcept(is_nothrow) requires (!passByMove) :
			tgt{tgt}, original{tgt}{}

		[[nodiscard]] constexpr explicit resumer(T& tgt) noexcept(is_nothrow) requires (passByMove) :
			tgt{tgt}, original{std::move(tgt)}{}

		constexpr ~resumer() {
			if constexpr(passByMove){
				tgt = std::move(original);
			} else{
				tgt = original;
			}
		}
	};
}
