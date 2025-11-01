module;

#include "../src/ext/enum_operator_gen.hpp"


export module mo_yanxi.ui.flags;

import std;

namespace mo_yanxi::ui{
	export enum struct propagate_mask : std::uint8_t{
		none = 0,
		local = 1u << 0,
		super = 1u << 1,
		child = 1u << 2,

		/**
		 * @brief notify parent that it's children requires some content layout update
		 */
		from_content = 1u << 3,

		all = local | super | child,
		all_visible = all | from_content,

		lower = local | child,
		upper = local | super,
	};

	BITMASK_OPS_BOOL(export, propagate_mask)
	BITMASK_OPS_ADDITIONAL(export, propagate_mask)


	export struct layout_state{
		/**
		 * @brief Represents how the space should be allocated for an element
		 */
		// SpaceAcquireType acquireType{};

		/**
		 * @brief Describes the accept direction in layout context
		 * e.g. An element in @link BedFace @endlink only accept layout notification from parent
		 */
		propagate_mask acceptMask_context{propagate_mask::all};

		/**
		 * @brief Describes the accept direction an element inherently owns
		 * e.g. An element of @link ScrollPanel @endlink deny children layout notify
		 */
		propagate_mask acceptMask_inherent{propagate_mask::all};


		/**
		 * @brief Determine whether this element is restricted in a cell, then disable up scale
		 */
		// bool restrictedByParent{false};

	private:
		bool children_changed{};
		bool parent_changed{};
		bool local_changed{};

	public:
		// [[nodiscard]] constexpr bool isRestricted() const noexcept{
		// 	return restrictedByParent;
		// }

		constexpr void ignoreChildren() noexcept{
			acceptMask_inherent -= propagate_mask::child;
		}

		[[nodiscard]] constexpr bool is_children_changed() const noexcept{
			return children_changed;
		}

		[[nodiscard]] constexpr bool check_children_changed() noexcept{
			return std::exchange(children_changed, false);
		}

		[[nodiscard]] constexpr bool check_changed() noexcept{
			return std::exchange(local_changed, false);
		}

		[[nodiscard]] constexpr bool check_parent_changed() noexcept{
			return std::exchange(parent_changed, false);
		}

		[[nodiscard]] constexpr bool check_any_changed() noexcept{
			bool a = check_changed();
			bool b = check_children_changed();
			bool c = check_parent_changed();
			return a || b || c;
		}

		[[nodiscard]] constexpr bool is_parent_changed() const noexcept{
			return parent_changed;
		}

		[[nodiscard]] constexpr bool is_changed() const noexcept{
			return local_changed;
		}

		[[nodiscard]] constexpr bool is_any_subs_changed() const noexcept{
			return local_changed || children_changed;
		}

		constexpr void notify_self_changed() noexcept{
			if(acceptMask_context & propagate_mask::local && acceptMask_inherent & propagate_mask::local){
				local_changed = true;
			}
		}

		constexpr bool notify_children_changed(const bool force = false) noexcept{
			if(force || (acceptMask_context & propagate_mask::child && acceptMask_inherent & propagate_mask::child)){
				children_changed = true;
				return true;
			}
			return false;
		}

		constexpr bool notify_parent_changed() noexcept{
			if(acceptMask_context & propagate_mask::super && acceptMask_inherent & propagate_mask::super){
				parent_changed = true;
				return false;
			}
			return false;
		}

		constexpr void clear() noexcept{
			local_changed = children_changed = parent_changed = false;
		}
	};

	export enum struct interactivity : std::uint8_t{
		disabled,
		children_only,
		enabled,
		intercepted,
	};

	export enum struct esc_flag{
		intercept,
		fall_through,
	};
}
