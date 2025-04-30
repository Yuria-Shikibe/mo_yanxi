module;

#include <cassert>

export module mo_yanxi.ui.celled_group;

export import mo_yanxi.ui.layout.cell;
export import mo_yanxi.ui.basic;
export import mo_yanxi.handle_wrapper;

import std;

namespace mo_yanxi::ui{
	export
	template <typename Elem, typename Cell>
	struct create_result{
		Elem& elem;
		Cell& cell;
	};

	export
	template <typename Elem, typename Cell>
	struct create_handle{
		struct promise_type;
		using handle = std::coroutine_handle<promise_type>;
		using value_type = create_result<Elem, Cell>;

		[[nodiscard]] create_handle() = default;

		[[nodiscard]] explicit create_handle(handle&& hdl)
			: hdl{std::move(hdl)}{
		}

		struct promise_type{
			[[nodiscard]] promise_type() = default;

			create_handle get_return_object(){
				return create_handle{handle::from_promise(*this)};
			}

			[[nodiscard]] static auto initial_suspend() noexcept{ return std::suspend_never{}; }

			[[nodiscard]] static auto final_suspend() noexcept{ return std::suspend_always{}; }

			static void return_void(){
			}

			auto yield_value(const value_type& val) noexcept{
				elem_ = &val.elem;
				cell_ = &val.cell;
				return std::suspend_always{};
			}

			[[noreturn]] static void unhandled_exception() noexcept{
				std::terminate();
			}

		private:
			friend create_handle;

			Elem* elem_;
			Cell* cell_;
		};

		void submit() const{
			hdl->resume();
		}

		[[nodiscard]] bool done() const noexcept{
			return hdl->done();
		}

		[[nodiscard]] value_type result() const noexcept{
			return {*hdl->promise().elem_, *hdl->promise().cell_};
		}

		Elem& elem() const noexcept{
			return *hdl->promise().elem_;
		}

		Cell& cell() const noexcept{
			return *hdl->promise().cell_;
		}

		Elem* operator->() const noexcept{
			return hdl->promise().elem_;
		}

		~create_handle(){
			if(hdl){
				if(!done()){
					submit();
				}
				assert(done());
				hdl->destroy();
			}
		}

	private:
		exclusive_handle_member<handle> hdl{};
	};

	export
	template <std::derived_from<basic_cell> T>
		// requires (std::is_copy_assignable_v<T> && std::is_default_constructible_v<T>)
	struct cell_adaptor{
		using cell_type = T;
		elem* element{};
		T cell{};

		[[nodiscard]] constexpr cell_adaptor() noexcept = default;

		[[nodiscard]] constexpr cell_adaptor(elem* element, const T& cell) noexcept
			: element{element},
			  cell{cell}{}

		void apply(group& group, stated_extent extent = {{size_category::external}, {size_category::external}}) const{
			cell.apply_to(group, *element, extent);
		}
	// protected:
		// constexpr void restrictSize() const noexcept{
		// 	element->property.size.setMaximumSize(cell.allocatedBound.getSize());
		// }
		//
		// constexpr void removeRestrict() const noexcept{
		// 	element->prop().clampedSize.setMaximumSize(Geom::Vec::constant<float>::max_vec2);
		//
		// }
	};

	export
	template <std::derived_from<basic_cell> CellTy, typename Adaptor = cell_adaptor<CellTy>>
		// requires requires(elem* e, const CellTy& cell){
		// 	Adaptor{e, cell};
		// }
	struct universal_group : public basic_group{
		using cell_type = CellTy;
		using adaptor_type = Adaptor;

		cell_type template_cell{};

	protected:
		std::vector<adaptor_type> cells{};

	public:
		using basic_group::basic_group;

		[[nodiscard]] std::span<const adaptor_type> get_cells() const noexcept{
			return cells;
		}

		void clear_children() noexcept override{
			basic_group::clear_children();
			cells.clear();
		}

		void post_remove(elem* element) override{
			if(const auto itr = find(element); itr != children.end()){
				cells.erase(cells.begin() + std::distance(children.begin(), itr));

				toRemove.push_back(std::move(*itr));
				children.erase(itr);
			}
			notify_layout_changed(spread_direction::all_visible);
		}

		void instant_remove(elem* element) override{
			if(const auto itr = find(element); itr != children.end()){
				cells.erase(cells.begin() + std::distance(children.begin(), itr));
				children.erase(itr);
			}
			notify_layout_changed(spread_direction::all_visible);
		}

		elem& add_children(elem_ptr&& element, std::size_t where) override{
			//TODO is this always right? e.g. may cause wrong in dynmiac table
			// element->layoutState.acceptMask_context -= spread_direction::child;
			return basic_group::add_children(std::move(element), where);
		}

		using basic_group::add_children;

		template <typename E, std::derived_from<universal_group> G, typename ...Args>
			requires requires{
				requires std::is_base_of_v<elem, E>;
				// requires std::constructible_from<E, scene*, group*, Args...> || (std::constructible_from<E, scene*, group*, std::string_view> && sizeof...(Args) == 0);
		}
		create_handle<E, cell_type> emplace(this G& self, Args&&... args){
			auto [result, adaptor] = self.template add<E>(elem_ptr{self.get_scene(), &self, std::in_place_type<E>, std::forward<Args>(args) ...});

			auto addr = self.cells.data();

			co_yield result;
			static_cast<universal_group&>(self).add_adaptor(addr, &result.elem, adaptor);
		}

		template <invocable_elem_init_func Fn, std::derived_from<universal_group> G>
		create_handle<typename elem_init_func_trait<Fn>::elem_type, cell_type> function_init(this G& self, Fn init){
			auto [result, adaptor] = self.template add<typename elem_init_func_trait<Fn>::elem_type>(elem_ptr{self.get_scene(), &self, init});
			auto addr = self.cells.data();

			co_yield result;
			static_cast<universal_group&>(self).add_adaptor(addr, &result.elem, adaptor);
		}

		// template <InvocableElemInitFunc Fn>
		// 	requires (std::is_default_constructible_v<typename ElemInitFuncTraits<Fn>::ElemType>)
		// auto createA(Fn init){
		// 	return add<typename ElemInitFuncTraits<Fn>::ElemType>(ElementUniquePtr{this, scene, init});
		// }

		CellTy& get_last_cell() noexcept{
			assert(!cells.empty());
			return cells.back().cell;
		}

	protected:
		void add_adaptor(adaptor_type* last_addr, elem* elem, adaptor_type& adaptor){
			if(last_addr == cells.data()){
				this->on_add(adaptor);
			}else{
				if(auto itr = std::ranges::find_last(cells, elem, &adaptor_type::element); itr.begin() != itr.end()){
					this->on_add(itr.front());
				}
			}
		}
		virtual void on_add(adaptor_type& adaptor){
		}

	private:
		template <std::derived_from<elem> E = elem, std::derived_from<universal_group> G>
		std::pair<create_result<E, CellTy>, adaptor_type&> add(this G& self, elem_ptr&& ptr){
			auto& adaptor = self.cells.emplace_back(&self.add_children(std::move(ptr)), self.template_cell);
			return {create_result{static_cast<E&>(*adaptor.element), adaptor.cell}, adaptor};
		}

	};

	export
	template <typename AdaptTy>
		// requires (std::derived_from<AdaptTy, cell_adaptor<typename AdaptTy::cell_type>>)
	using celled_group  = universal_group<typename AdaptTy::cell_type, AdaptTy>;

}
