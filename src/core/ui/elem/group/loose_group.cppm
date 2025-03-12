export module mo_yanxi.ui.loose_group;

export import mo_yanxi.ui.elem_ptr;
export import mo_yanxi.ui.group;
// export import Core.UI.Template.Traits;

import std;

namespace mo_yanxi::ui{
	export
	struct basic_group : public group {
	protected:
		std::vector<elem_ptr> toRemove{};
		std::vector<elem_ptr> children{};

	public:
		[[nodiscard]] basic_group(scene* scene, group* group, const std::string_view tyName)
			: group(scene, group, tyName){
			interactivity = interactivity::children_only;
		}

		void clearChildren() noexcept override{
			toRemove.clear();
			children.clear();
			notify_layout_changed(spread_direction::super | spread_direction::child_content);
		}

		void postRemove(elem* elem) override{
			if(const auto itr = find(elem); itr != children.end()){
				toRemove.push_back(std::move(*itr));
				children.erase(itr);
			}
			notify_layout_changed(spread_direction::all_visible);
		}

		void instantRemove(elem* elem) override{
			if(const auto itr = find(elem); itr != children.end()){
				children.erase(itr);
			}
			notify_layout_changed(spread_direction::all_visible);
		}

		elem& addChildren(elem_ptr&& elem) override{
			setChildrenFillParentSize(*elem, content_size());
			elem->update_abs_src(abs_pos() + property.boarder.bot_lft());
			notify_layout_changed(spread_direction::upper | spread_direction::child_content);
			return *children.emplace_back(std::move(elem));
		}

		[[nodiscard]] std::span<const elem_ptr> get_children() const noexcept override{
			return children;
		}

		void update(const float delta_in_ticks) override{
			toRemove.clear();

			elem::update(delta_in_ticks);

			update_children(delta_in_ticks);
		}

		bool try_layout() override{
			if(layoutState.check_children_changed()){
				layoutState.clear();
				layout();
				layout_children();
				return true;
			}

			return group::try_layout();
		}

	protected:
		decltype(children)::iterator find(elem* elem) noexcept{
			return std::ranges::find(children, elem, &elem_ptr::get);
		}
	};

	export
	struct loose_group : basic_group{
		using basic_group::basic_group;
	};

	// export
	// struct ElementStack : public LooseGroup{
	// 	template <std::derived_from<elem> E, typename ...Args>
	// 		requires (std::constructible_from<E, Args...>)
	// 	E& emplace(Args&&... args){
	// 		return this->add<E>(elem_ptr{this, scene, new E{std::forward<Args>(args) ...}});
	// 	}
	//
	// 	template <Template::ElementCreator Tmpl>
	// 	typename Template::Traits<Tmpl>::ElementType& create(const Tmpl& tmpl){
	// 		return this->add<typename Template::Traits<Tmpl>::ElementType>(Template::Traits<Tmpl>::create(tmpl, this, scene));
	// 	}
	//
	// 	template <Template::ElementCreator Tmpl, std::invocable<typename Template::Traits<Tmpl>::ElementType&> Init>
	// 	auto& create(const Tmpl& tmpl, Init init){
	// 		auto& rst = this->create(tmpl);
	//
	// 		std::invoke(init, rst);
	// 		return rst;
	// 	}
	//
	// 	template <InvocableElemInitFunc Fn>
	// 		requires (std::is_default_constructible_v<typename ElemInitFuncTraits<Fn>::ElemType>)
	// 	auto& emplaceInit(Fn init){
	// 		return add(elem_ptr{this, scene, init});
	// 	}
	//
	// private:
	// 	template <std::derived_from<elem> E = elem>
	// 	E& add(elem_ptr&& ptr){
	// 		return static_cast<E&>(addChildren(std::move(ptr)));
	// 	}
	//
	//
	// 	elem& addChildren(elem_ptr&& elem) override{
	// 		elem->prop().fillParent = {true, true};
	//
	// 		//TODO actively re-assign these relative src after border changed
	// 		elem->prop().relativeSrc = property.boarder.bot_lft();
	// 		return basic_group::addChildren(std::move(elem));
	// 	}
	// };
}
