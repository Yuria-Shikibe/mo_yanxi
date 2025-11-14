//
// Created by Matrix on 2025/6/1.
//

export module mo_yanxi.ui.menu;

import mo_yanxi.ui.elem.list;
import mo_yanxi.ui.elem.table;
import mo_yanxi.ui.elem.scroll_pane;
import mo_yanxi.ui.elem.button;
import std;

namespace mo_yanxi::ui{

	export
	template <typename B, typename E>
	struct menu_elem_create_handle{
		create_handle<B, list::cell_type> button;
		E& elem;
	};

	export
	struct menu : list{
	private:
		static constexpr std::size_t content_index = 1;
		static constexpr align::padding1d<float> button_group_pad = {4, 4};

		static constexpr unsigned no_switched = std::numeric_limits<unsigned>::max();
		unsigned last_index_{no_switched};
		scroll_pane* button_menu_pane{};
		std::vector<elem_ptr> sub_elements{};

		template <std::derived_from<elem> T>
		struct button_of : T{
		private:
			using base = T;

			menu& get_menu() const noexcept{
				// self -> parent(list) -> parent(scroll_pane) -> parent(menu)
				return static_cast<menu&>(*this->get_parent()->get_parent()->get_parent());
			}

			input_event::click_result on_click(const input_event::click click_event) override{
				if(click_event.code.matches(core::ctrl::lmb_click)){
					menu& menu = get_menu();
					auto idx = menu.get_index_of(this);
					if(menu.last_index_ == idx){
						get_menu().reset_switch();
					}else{
						get_menu().call_switch(idx);
					}

				}

				return base::on_click(click_event);
			}

			void update(float delta_in_ticks) override{
				base::update(delta_in_ticks);

				menu& menu = get_menu();
				auto idx = menu.get_index_of(this);
				this->elem::activated = menu.last_index_ == idx;
			}

		public:
			template <typename ...Args>
			[[nodiscard]] button_of(scene* scene, group* group, Args&& ...args)
				: base(scene, group, std::forward<Args>(args) ...){
				this->interactivity = interactivity::enabled;

			}



		};

		unsigned get_index_of(const elem* elem) const noexcept{
			const auto& t = get_button_group();
			const auto rng = t.get_children();
			const auto itr = std::ranges::find(rng, elem, &elem_ptr::get);
			if(itr != rng.end()){
				const auto idx = std::ranges::distance(rng.begin(), itr);
				return idx;
			}

			return no_switched;
		}

		void call_switch(const unsigned idx){
			if(last_index_ != no_switched){
				auto element_at_last_index = exchange_element(content_index, std::move(sub_elements[idx]), true);
				sub_elements[idx] = std::exchange(sub_elements[last_index_], std::move(element_at_last_index));
			}else{
				sub_elements[idx] = exchange_element(content_index, std::move(sub_elements[idx]), true);
			}
			last_index_ = idx;
		}

		void reset_switch(){
			if(last_index_ != no_switched){
				sub_elements[last_index_] = exchange_element(content_index, std::move(sub_elements[last_index_]), true);
				last_index_ = no_switched;
			}
		}

	public:
		[[nodiscard]] table& get_default_elem() const noexcept{
			if(last_index_ != no_switched){
				return static_cast<table&>(*sub_elements[last_index_]);
			}else{
				return static_cast<table&>(*get_children()[content_index]);
			}
		}

		[[nodiscard]] list& get_button_group() const noexcept{
			return button_menu_pane->get_item<list>();
		}

		[[nodiscard]] scroll_pane& get_button_group_pane() const noexcept{
			return *button_menu_pane;
		}

		[[nodiscard]] menu(scene* scene, group* group)
			: list(scene, group){

			//TODO implement vertical layout

			auto pane = list::emplace<scroll_pane>();
			pane.cell().pad.set(8);


			button_menu_pane = std::to_address(pane);
			button_menu_pane->set_layout_policy(layout_policy::vert_major);
			button_menu_pane->set_style();
			button_menu_pane->function_init([](list& lst){
				lst.set_layout_policy(layout_policy::vert_major);
				lst.template_cell.set_external();
				lst.set_style();
			});

			pane.cell().set_size(80);

			//Default element
			list::emplace<ui::table>();
		}

		auto emplace() = delete;
		auto function_init() = delete;
		auto create() = delete;

		void set_button_group_height(const float height){
			cells[0].cell.set_size(height);
			notify_isolated_layout_changed();
			// notify_layout_changed(spread_direction::all_visible);
		}

		template <typename B, typename E>
		menu_elem_create_handle<button_of<B>, E> add_elem(){
			auto& g = get_button_group();
			if(g.has_children()){
				g.get_last_cell().pad = button_group_pad;
			}

			auto b = get_button_group().emplace<button_of<B>>();
			auto& e = sub_elements.emplace_back(elem_ptr{get_scene(), this, std::in_place_type<E>});
			return menu_elem_create_handle<button_of<B>, E>{std::move(b), static_cast<E&>(*e)};
		}

		void erase_elem(std::size_t index) noexcept{
			if(index >= sub_elements.size())return;
			if(last_index_ == index){
				reset_switch();
			}

			sub_elements.erase(sub_elements.begin() + index);
		}

	private:
		elem_ptr exchange_element(const std::size_t where, elem_ptr&& elem, bool isolated_notify) override{
			return universal_group::exchange_element(where, std::move(elem), isolated_notify);
		}
	};

	// void foo(){
	// 	menu menu{nullptr, nullptr};
	// 	menu.function_init();
	// }
}