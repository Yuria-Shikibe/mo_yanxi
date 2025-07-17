module;

#include <cassert>

export module mo_yanxi.ui.primitives:tooltip_interface;

import :pre_decl;
import :elem_ptr;
export import mo_yanxi.math.vector2;
export import mo_yanxi.ui.layout.policies;
export import align;
export import mo_yanxi.handle_wrapper;

import std;

namespace mo_yanxi::ui{
	export
	enum struct tooltip_follow{
		initial_pos,
		cursor,
		owner,
		dialog,
	};

	// export
	// enum struct TooltipRootIdentity{
	// 	ignore,
	// 	entity
	// };

	export
	struct
	tooltip_align{
		tooltip_follow follow{};
		std::optional<math::vec2> pos{};
		align::pos align{};
		stated_extent extent{extent_by_external};
		// layout_policy layout_policy{};


	};

	export
	struct tooltip_owner{
	protected:
		// static constexpr auto CursorPos = Geom::SNAN2;

		exclusive_handle_member<elem*> tooltip_handle{};

		~tooltip_owner() = default;

		[[nodiscard]] tooltip_owner() = default;
	public:



		/**
		 * @brief
		 * @return the align reference point in screen space
		 */
		[[nodiscard]] virtual tooltip_align tooltip_align_policy() const = 0;

		[[nodiscard]] virtual bool tooltip_should_maintain(math::vec2 cursorPos) const{
			return false;
		}

		[[nodiscard]] virtual bool tooltip_owner_contains(math::vec2 cursorPos) const = 0;

		[[nodiscard]] virtual bool tooltip_should_build(math::vec2 cursorPos) const = 0;

		[[nodiscard]] virtual bool tooltip_should_force_drop(math::vec2 cursorPos) const{
			return false;
		}

		virtual void tooltip_on_drop(){
			tooltip_handle = nullptr;
		}

		[[nodiscard]] bool has_tooltip() const noexcept{
			return tooltip_handle != nullptr;
		}

		elem_ptr tooltip_setup(scene& scene){
			auto ptr = tooltip_build_impl(scene, tooltip_deduce_parent(scene));
			this->tooltip_handle = ptr.get();
			return ptr;
		}

		void tooltip_notify_drop();

	protected:
		[[nodiscard]] virtual elem_ptr tooltip_build_impl(scene& scene, group* group) = 0;

	public:
		[[nodiscard]] group* tooltip_deduce_parent(scene& scene) const;

	private:
		/**
		 * @brief nullptr for default (scene.root)
		 * @return given parent
		 */
		[[nodiscard]] virtual group* tooltip_specfied_parent() const{
			return nullptr;
		}
	};


	export
	template<typename T>
	struct func_tooltip_owner : public tooltip_owner{
	protected:
		~func_tooltip_owner() = default;

	private:
		using builder_type = std::move_only_function<elem_ptr(T&, scene&)>;
		builder_type toolTipBuilder{};

	public:
		[[nodiscard]] func_tooltip_owner() = default;

		template<elem_init_func InitFunc, std::derived_from<T> S>
			requires std::invocable<InitFunc, S&, typename elem_init_func_trait<InitFunc>::elem_type&>
		decltype(auto) set_tooltip_builder(this S& self, InitFunc&& initFunc){
			return std::exchange(self.toolTipBuilder, builder_type{[func = std::forward<InitFunc>(initFunc)](T& owner, scene& scene){
				return elem_ptr{
					&scene,
					static_cast<func_tooltip_owner&>(owner).tooltip_deduce_parent(scene),
					[&](typename elem_init_func_trait<InitFunc>::elem_type& e){
						std::invoke(func, static_cast<S&>(owner), e);
				}};
			}});
		}

		template<elem_init_func InitFunc>
			requires std::invocable<InitFunc, typename elem_init_func_trait<InitFunc>::elem_type&>
		decltype(auto) set_tooltip_builder(InitFunc&& initFunc){
			return std::exchange(toolTipBuilder, decltype(toolTipBuilder){[func = std::forward<InitFunc>(initFunc)](T& owner, scene& scene){
				return elem_ptr{&scene, static_cast<func_tooltip_owner&>(owner).tooltip_deduce_parent(scene), func};
			}});
		}

		bool has_tooltip_builder() const noexcept{
			return static_cast<bool>(toolTipBuilder);
		}

	protected:
		[[nodiscard]] elem_ptr tooltip_build_impl(scene& scene, group* group) override{
			assert(has_tooltip_builder());
			return std::invoke(toolTipBuilder, static_cast<T&>(*this), scene);
		}
	};

	export
	struct tooltip_layout_info{
		tooltip_follow follow{tooltip_follow::initial_pos};

		align::pos align_owner{align::pos::bottom_left};
		align::pos align_tooltip{align::pos::top_left};

		math::vec2 offset{};
	};

	export
	struct tooltip_create_info{
		static constexpr float disable_auto_tooltip = -1.0f;
		static constexpr float def_tooltip_hover_time = 25.0f;

		tooltip_layout_info layout_info{};

		bool use_stagnate_time{false};
		bool auto_release{true};
		float min_hover_time{def_tooltip_hover_time};

		[[nodiscard]] bool auto_build() const noexcept{
			return min_hover_time >= 0.0f;
		}
	};
	//
	export
	template<typename T>
	struct stated_tooltip_owner : public func_tooltip_owner<T>{
	protected:
		~stated_tooltip_owner() = default;

		tooltip_create_info tooltip_prop_{};

	public:
		[[nodiscard]] const tooltip_create_info& get_tooltip_prop() const noexcept{
			return tooltip_prop_;
		}

		template<elem_init_func InitFunc, std::derived_from<T> S>
		void set_tooltip_state(this S& self, const tooltip_create_info& toolTipProperty, InitFunc&& initFunc) noexcept{
			self.tooltip_prop_ = toolTipProperty;
			self.set_tooltip_builder(std::forward<InitFunc>(initFunc));
		}

	};

}
