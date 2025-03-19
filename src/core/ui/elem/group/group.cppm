module;

#include <cassert>

export module mo_yanxi.ui.group;

export import mo_yanxi.ui.elem_ptr;
export import mo_yanxi.ui.elem;

import std;

namespace mo_yanxi::ui{
	// export struct scene;

	// export struct scene;
	export
	struct group : public elem{
		using elem::elem;

		virtual void postRemove(elem* element) = 0;

		virtual void instantRemove(elem* element) = 0;

		virtual void clearChildren() noexcept = 0;

		virtual elem& addChildren(elem_ptr&& element) = 0;

		[[nodiscard]] elem* find_children_by_name(const std::string_view name) const{
			for (const elem_ptr& element : get_children()){
				if(element->prop().name == name){
					return element.get();
				}
			}

			return nullptr;
		}
		//TODO using explicit this


		virtual void update_children(const float delta_in_ticks){
			for(const auto& element : get_children()){
				element->update(delta_in_ticks);
			}
		}

		virtual void layout_children(/*Direction*/){
			for(const auto& element : get_children()){
				element->try_layout();
			}
		}

		void draw_content(const rect clipSpace, rect redirect) const override{
			const auto space = property.content_bound_absolute().intersection_with(clipSpace);
			drawChildren(space, redirect);
		}

		bool try_layout() override{
			if(layoutState.is_children_changed() || layoutState.is_changed()){
				layoutState.clear();
				layout();

				return true;
			}

			return false;
		}

		void layout() override{
			elem::layout();
			layout_children();
		}

		// virtual std::optional<Geom::Vec2> getElementFitnessSize(const elem& element) const noexcept{
		// 	return std::nullopt;
		// }

	protected:
		/*virtual*/ void drawChildren(const rect clipSpace, const rect redirect) const{
			for(const auto& element : get_children()){
				element->draw(clipSpace, redirect);
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
					setChildrenFillParentSize(*element, newSize);
				}

				// try_layout();
				return true;
			}

			return false;
		}

		// bool update_abs_src(const math::vec2 parentAbsSrc) override{
		// 	if(elem::update_abs_src(parentAbsSrc)){
		// 		for(const auto& element : get_children()){
		// 			element->update_abs_src(abs_pos());
		// 		}
		// 		return true;
		// 	}
		// 	return false;
		// }


		// template <std::derived_from<elem> E>
		// decltype(auto) emplaceChildren(){
		// 	auto ptr = ElementUniquePtr{this, scene, new E};
		// 	auto rst = ptr.get();
		// 	addChildren(std::move(ptr));
		// 	return *static_cast<E*>(rst);
		// }
		//
		// template <InvocableElemInitFunc Fn>
		// 	requires (std::is_default_constructible_v<typename ElemInitFuncTraits<Fn>::ElemType>)
		// decltype(auto) emplaceInitChildren(Fn init){
		// 	auto ptr = ElementUniquePtr{this, scene, init};
		// 	auto rst = ptr.get();
		// 	addChildren(std::move(ptr));
		// 	return *static_cast<typename ElemInitFuncTraits<Fn>::ElemType*>(rst);
		// }

	protected:
		/**
		 * @return true if all set by parent size
		 */
		static bool setChildrenFillParentSize(elem& item, math::vec2 boundSize){
			const auto [fx, fy] = item.prop().fill_parent;
			if(!fx && !fy) return false;

			const auto [vx, vy] = boundSize;
			const auto [ox, oy] = item.get_size();


			if(fx)item.context_size_restriction.width = {size_category::mastering, vx};
			if(fy)item.context_size_restriction.height = {size_category::mastering, vy};

			item.resize({
					fx ? vx : ox,
					fy ? vy : oy
				});

			return fx && fy;
		}

		static bool setChildrenFillParentSize_Quiet(elem& item, math::vec2 boundSize){
			const auto [fx, fy] = item.prop().fill_parent;
			if(!fx && !fy) return false;

			const auto [vx, vy] = boundSize;
			const auto [ox, oy] = item.get_size();

			if(fx)item.context_size_restriction.width = {size_category::mastering, vx};
			if(fy)item.context_size_restriction.height = {size_category::mastering, vy};

			item.resize_quiet({
					fx ? vx : ox,
					fy ? vy : oy
				});

			return fx && fy;
		}
	// 	void modifyChildren(elem& element);
	};
}
