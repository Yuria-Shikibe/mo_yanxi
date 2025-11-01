module;

#include <cassert>

export module mo_yanxi.ui.primitives:dialog_manager;

import :elem;
import mo_yanxi.utility;

namespace mo_yanxi::ui{
	export enum struct dialog_extent_type : std::uint8_t{
		scaling,
		mastering,
		dependent
	};

	export
	struct dialog_layout{
		math::vector2<dialog_extent_type> dependent{};
		align::pos align{align::pos::center};
		rect region{0, 0, 1, 1};
	};

	export
	struct dialog{
		elem_ptr elem{};
		dialog_layout extent{};

		void update_bound(const math::vec2 scene_viewport_size) const{
			elem->property.size.set_maximum_size(scene_viewport_size);

			math::vec2 src{extent.region.src};
			math::vec2 size{extent.region.extent()};
			if(extent.dependent.x == dialog_extent_type::scaling){
				size.x *= scene_viewport_size.x;
				src.x *= scene_viewport_size.x;
			}

			if(extent.dependent.y == dialog_extent_type::scaling){
				size.y *= scene_viewport_size.y;
				src.y *= scene_viewport_size.y;
			}

			stated_extent sz{
				stated_size{extent.dependent.x == dialog_extent_type::dependent ? size_category::dependent : size_category::mastering, size.x},
				stated_size{extent.dependent.y == dialog_extent_type::dependent ? size_category::dependent : size_category::mastering, size.y}
			};

			elem->context_size_restriction = sz;

			auto rst_sz = elem->pre_acquire_size(sz).value_or(size);
			elem->resize(rst_sz);

			rst_sz = elem->get_size();
			const rect bound{tags::from_extent, src, rst_sz};

			const auto embed = align::transform_offset(extent.align, scene_viewport_size, bound);
			elem->update_abs_src(embed);
		}

		[[nodiscard]] std::string_view get_name() const noexcept{
			assert(elem != nullptr);
			return elem->name;
		}

		[[nodiscard]] math::vector2<bool> fill_parent() const noexcept{
			assert(elem != nullptr);
			return elem->property.fill_parent;
		}

		ui::elem* get() const noexcept{
			return elem.get();
		}
	};

	struct dialog_fading : dialog{
		static constexpr float fading_time{15};
		float duration{fading_time};
	};

	export
	struct dialog_manager{

	private:
		friend scene;

		using container = std::vector<dialog>;

		container dialogs{};
		std::vector<dialog_fading> fading_dialogs{};
		std::vector<elem*> draw_sequence{};

		scene* scene_{};
		elem* top_{};
	public:
		template <invocable_elem_init_func Fn>
		typename elem_init_func_trait<Fn>::elem_type& create(dialog_layout layout, Fn fn){
			dialog& dlg = dialogs.emplace_back(dialog{elem_ptr{scene_, nullptr, std::move(fn)}, layout});
			update_top();
			draw_sequence.push_back(top());
			return static_cast<typename elem_init_func_trait<Fn>::elem_type&>(dlg.elem.operator*());
		}

		template <typename T, typename ...Args>
			requires (std::constructible_from<T, scene*, group*, Args...>)
		T& emplace(dialog_layout layout, Args&&... args){
			clear_tooltip();
			dialog& dlg = dialogs.emplace_back(dialog{elem_ptr{scene_, nullptr, std::in_place_type<T>, std::forward<Args>(args) ...}, layout});
			update_top();
			draw_sequence.push_back(top());
			return static_cast<T&>(dlg.elem.operator*());
		}

		void pop_back() noexcept{
			if(dialogs.empty()){
				return;
			}

			auto dialog = std::move(dialogs.back());
			dialogs.pop_back();
			update_top();
			fading_dialogs.push_back({std::move(dialog)});
		}

		elem* top() const noexcept{
			return top_;
		}

		void truncate(const elem* elem){
			if(auto itr = std::ranges::find(dialogs, elem, &dialog::get); itr != dialogs.end()){
				truncate(itr);
			}
		}

		void truncate(container::iterator where){
			std::ranges::subrange rng{where, dialogs.end()};
			for (const auto & dialog : rng){
				dialog.elem->clear_external_references_recursively();
			}
			std::ranges::move(
				rng | std::views::transform([](auto&& v){
					return dialog_fading{std::move(v)};
				}), std::back_inserter(fading_dialogs));

			dialogs.erase(where, dialogs.end());
			update_top();
		}

		void draw_all(rect clipspace) const;

		[[nodiscard]] bool empty() const noexcept{
			return dialogs.empty();
		}

		void update(float delta_in_tick);

	private:
		void clear_tooltip() const;

		void update_top() noexcept;

		esc_flag on_esc() noexcept;
	};

}
