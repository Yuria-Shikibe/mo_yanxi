module;

#include <cassert>

export module mo_yanxi.ui.primitives:group;

import :pre_decl;
import :elem_ptr;
import :elem;

import std;

namespace mo_yanxi::ui{

	// export struct scene;
	export
	struct group : public elem{
		using elem::elem;

		[[nodiscard]] elem* find_children_by_name(const std::string_view name) const{
			for (const elem_ptr& element : get_children()){
				if(element->name == name){
					return element.get();
				}
			}

			return nullptr;
		}
		//TODO using explicit this


	protected:
		virtual void update_children(const float delta_in_ticks){
			for(const auto& element : get_children()){
				element->update(delta_in_ticks);
			}
		}

		void layout_children(){
			for(const auto& element : get_children()){
				element->try_layout();
			}
		}
	public:

		void draw_content(const rect clipSpace) const override{
			const auto space = property.content_bound_absolute().intersection_with(clipSpace);
			draw_children(space);
		}

		bool try_layout() override{
			if(layout_state.is_children_changed() || layout_state.is_changed()){
				layout_state.clear();
				layout();

				return true;
			}

			return false;
		}

		void layout() override{
			elem::layout();
			layout_children();
		}

	protected:
		/*virtual*/ void draw_children(const rect clipSpace) const{
			for(const auto& element : get_children()){
				element->try_draw(clipSpace);
			}
		}
	public:

		void set_scene(scene* manager) override{
			elem::set_scene(manager);
			for(const auto& element : get_children()){
				element->set_scene(manager);
			}
		}

		bool resize(const math::vec2 size) override{
			if(elem::resize(size)){
				const auto newSize = content_size();

				for (auto& element : get_children()){
					setChildrenFillParentSize_legacy(*element, newSize);
				}

				// try_layout();
				return true;
			}

			return false;
		}


	protected:
		static bool set_fillparent(
			elem& item,
			math::vec2 boundSize,
			math::bool2 mask = {true, true},
			bool set_restriction_to_mastering = true,
			spread_direction direction_mask = spread_direction::all_visible){
			const auto [fx, fy] = item.prop().fill_parent && mask;
			if(!fx && !fy) return false;

			const auto [ox, oy] = item.get_size();

			using ss = stated_size;

			if(fx)item.context_size_restriction.set_width(boundSize.x);
			else{
				if(set_restriction_to_mastering){
					item.context_size_restriction.set_width(boundSize.x);
				}else{
					item.context_size_restriction.set_width_dependent();
				}
			}

			if(fy) item.context_size_restriction.set_height(boundSize.y);
			else{
				if(set_restriction_to_mastering){
					item.context_size_restriction.set_height(boundSize.y);
				}else{
					item.context_size_restriction.set_height_dependent();
				}
			}

			item.resize_masked({
					fx ? boundSize.x : ox,
					fy ? boundSize.y : oy
				}, direction_mask);

			return true;
		}

		/**
		 * @return true if all set by parent size
		 */
		static bool setChildrenFillParentSize_legacy(elem& item, math::vec2 boundSize){
			const auto [fx, fy] = item.prop().fill_parent;
			if(!fx && !fy) return false;

			const auto [vx, vy] = boundSize;
			const auto [ox, oy] = item.get_size();


			if(fx)item.context_size_restriction.set_width(vx);
			if(fy)item.context_size_restriction.set_height(vy);

			item.resize({
					fx ? vx : ox,
					fy ? vy : oy
				});

			return fx && fy;
		}

		static bool setChildrenFillParentSize_Quiet_legacy(elem& item, math::vec2 boundSize){
			const auto [fx, fy] = item.prop().fill_parent;
			if(!fx && !fy) return false;

			const auto [vx, vy] = boundSize;
			const auto [ox, oy] = item.get_size();

			if(fx)item.context_size_restriction.set_width(vx);
			if(fy)item.context_size_restriction.set_height(vy);

			item.resize_masked({
					fx ? vx : ox,
					fy ? vy : oy
				});

			return fx && fy;
		}
	// 	void modifyChildren(elem& element);
	};

	export
	struct basic_group : public group {
	protected:
		std::vector<elem_ptr> expired{};
		std::vector<elem_ptr> children{};

	public:
		[[nodiscard]] basic_group(scene* scene, group* group_, const std::string_view tyName)
			: group(scene, group_, tyName){
			interactivity = interactivity::children_only;
		}

		virtual void clear_children() noexcept{
			expired.clear();
			children.clear();
			notify_layout_changed(spread_direction::super | spread_direction::from_content);
		}

		virtual void post_remove(elem* elem){
			if(const auto itr = find(elem); itr != children.end()){
				expired.push_back(std::move(*itr));
				children.erase(itr);
			}
			notify_layout_changed(spread_direction::all_visible);
		}

		virtual void instant_remove(elem* elem){
			if(const auto itr = find(elem); itr != children.end()){
				children.erase(itr);
			}
			notify_layout_changed(spread_direction::all_visible);
		}

		virtual elem_ptr exchange_element(std::size_t where, elem_ptr&& elem){
			assert(elem != nullptr);
			if(where >= children.size())return {};

			notify_layout_changed(spread_direction::all_visible);
			return std::exchange(children[where], std::move(elem));
		}

		//TODO emplace

		virtual elem& add_children(elem_ptr&& elem, std::size_t where){
			setChildrenFillParentSize_legacy(*elem, content_size());

			elem->update_abs_src(content_src_pos());
			notify_layout_changed(spread_direction::upper | spread_direction::from_content);
			return **children.insert(children.begin() + std::min<std::size_t>(where, children.size()), std::move(elem));
		}

		elem& add_children(elem_ptr&& elem){
			return add_children(std::move(elem), std::numeric_limits<std::size_t>::max());
		}

		template <std::derived_from<elem> E, typename ...Args>
		E& emplace_children_at(std::size_t index, Args&&... args){
			auto& elem = add_children(elem_ptr{get_scene(), this, std::in_place_type<E>, std::forward<Args>(args) ...}, index);
			return static_cast<E&>(elem);
		}

		[[nodiscard]] std::span<const elem_ptr> get_children() const noexcept override{
			return children;
		}

		void update(const float delta_in_ticks) override{
			expired.clear();

			elem::update(delta_in_ticks);

			if(is_sleep())return;
			update_children(delta_in_ticks);
		}

	protected:
		decltype(children)::iterator find(elem* elem) noexcept{
			return std::ranges::find(children, elem, &elem_ptr::get);
		}
	};

	export
	struct loose_group : basic_group{
		[[nodiscard]] loose_group(scene* scene, group* group)
			: basic_group(scene, group, "loose_group"){
		}

	};
}
