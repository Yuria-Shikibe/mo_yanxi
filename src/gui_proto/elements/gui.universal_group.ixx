module;

#include <cassert>

export module mo_yanxi.gui.celled_group;

export import mo_yanxi.gui.infrastructure.group;
export import mo_yanxi.gui.infrastructure;
export import mo_yanxi.gui.layout.cell;
import mo_yanxi.handle_wrapper;

import std;

namespace mo_yanxi::gui{


	export
	template <typename Elem, typename Cell>
	struct create_handle{
		struct promise_type;
		using handle = std::coroutine_handle<promise_type>;
		using value_type = layout::cell_create_result<Elem, Cell>;

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

		explicit operator bool() const noexcept{
			return static_cast<bool>(hdl.handle);
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

		create_handle(const create_handle& other) = delete;
		create_handle(create_handle&& other) noexcept = default;
		create_handle& operator=(const create_handle& other) = delete;
		create_handle& operator=(create_handle&& other) noexcept = default;

	private:
		exclusive_handle_member<handle> hdl{};
	};

	export
	template <std::derived_from<layout::basic_cell> T>
	struct cell_adaptor{
		using cell_type = T;
		elem* element{};
		T cell{};

		[[nodiscard]] constexpr cell_adaptor() noexcept = default;

		[[nodiscard]] constexpr cell_adaptor(elem* element, const T& cell) noexcept
			: element{element},
			  cell{cell}{}

		void apply(elem& group, layout::optional_mastering_extent extent = {math::vectors::constant2<float>::inf_positive_vec2}) const{
			cell.apply_to(group, *element, extent);
		}

		const T* operator->() const noexcept{
			return std::addressof(cell);
		}

		T* operator->() noexcept{
			return std::addressof(cell);
		}
	};

	export
	template <std::derived_from<layout::basic_cell> CellTy, typename Adaptor = cell_adaptor<CellTy>>
	struct universal_group : public basic_group{
		using cell_type = CellTy;
		using adaptor_type = Adaptor;

		cell_type template_cell{};

	protected:
		std::pmr::vector<adaptor_type> cells_{get_scene().get_allocator()};

	public:
		using basic_group::basic_group;

		[[nodiscard]] std::span<const adaptor_type> cells() const noexcept{
			return cells_;
		}

		void clear() noexcept override{
			cells_.clear();
			basic_group::clear();
		}

		void erase_afterward(std::size_t where) override{
			cells_.erase(cells_.begin() + where);
			basic_group::erase_afterward(where);
		}

		void erase_instantly(std::size_t where) override{
			cells_.erase(cells_.begin() + where);
			basic_group::erase_instantly(where);
		}

		elem& insert(std::size_t where, elem_ptr&& elemPtr) final {
			adaptor_type& adpt = *cells_.emplace(cells_.begin() + std::min<std::size_t>(where, cells_.size()), elemPtr.get(), template_cell);
			auto& rst = basic_group::insert(where, std::move(elemPtr));
			this->on_element_add(adpt);
			return rst;
		}

		elem_ptr exchange(std::size_t where, elem_ptr&& elem, bool force_isolated_notify) final{
			auto* p = elem.get();
			auto rst = basic_group::exchange(where, std::move(elem), force_isolated_notify);
			cells_[where].element = p;
			this->on_element_add(cells_[where]);

			return rst;
		}

		bool set_scaling(math::vec2 scl) noexcept override{
			assert(!scl.is_NaN());
			if(!util::try_modify(context_scaling_, scl))return false;
			context_scaling_ = scl;
			layout_state.notify_self_changed();

			if(!children_.empty() && propagate_scaling_){
				layout_state.notify_children_changed();
				auto s = get_scaling();

				for (auto && [elem, cell] : std::views::zip(children_, cells_)){
					elem->set_scaling(s * cell.cell.scaling);

				}
			}
			return true;
		}

		template <std::derived_from<elem> E, std::derived_from<universal_group> G, typename ...Args>
			requires (std::constructible_from<E, scene&, elem*, Args...>)
		create_handle<E, cell_type> emplace(this G& self, std::size_t where, Args&&... args){
			elem_ptr eptr{self.get_scene(), &self, std::in_place_type<E>, std::forward<Args>(args) ...};
			adaptor_type adaptor{eptr.get(), self.template_cell};

			co_yield layout::cell_create_result{static_cast<E&>(*eptr), adaptor.cell};
			self.cells_.insert(self.cells_.begin() + std::min<std::size_t>(where, self.cells_.size()), adaptor);
			self.basic_group::insert(where, std::move(eptr));
			static_cast<universal_group&>(self).on_element_add(adaptor);
		}

		template <invocable_elem_init_func Fn, std::derived_from<universal_group> G, typename ...Args>
		create_handle<typename elem_init_func_trait<Fn>::elem_type, cell_type> create(
			this G& self,
			std::size_t where, Fn&& init,
			Args&& ...args
			){
			elem_ptr eptr{self.get_scene(), &self, std::forward<Fn>(init), std::forward<Args>(args)...};
			adaptor_type adaptor{eptr.get(), self.template_cell};

			co_yield layout::cell_create_result{static_cast<typename elem_init_func_trait<Fn>::elem_type&>(*eptr), adaptor.cell};
			cells_.insert(self.cells_.begin() + std::min<std::size_t>(where, self.cells_.size()), adaptor);
			self.basic_group::insert(where, std::move(eptr));
			static_cast<universal_group&>(self).on_element_add(adaptor);
		}

		template <std::derived_from<elem> E, std::derived_from<universal_group> G, typename ...Args>
			requires (std::constructible_from<E, scene&, elem*, Args...>)
		create_handle<E, cell_type> emplace_back(this G& self, Args&&... args){
			return self.template emplace<E>(self.children_.size(), std::forward<Args>(args) ...);
		}

		template <invocable_elem_init_func Fn, std::derived_from<universal_group> G, typename ...Args>
		create_handle<typename elem_init_func_trait<Fn>::elem_type, cell_type> create_back(this G& self, Fn&& init, Args&& ...args){
			return self.create(children_.size(), std::forward<Fn>(init), std::forward<Args>(args)...);
		}

		cell_type& get_last_cell() noexcept{
			assert(!cells_.empty());
			return cells_.back().cell;
		}

	protected:
		virtual void on_element_add(adaptor_type& adaptor){
			basic_group::on_element_add(*adaptor.element);
		}

		void on_element_add(elem& adaptor) const final{

		}

	};

	export
	template <typename AdaptTy>
		// requires (std::derived_from<AdaptTy, cell_adaptor<typename AdaptTy::cell_type>>)
	using celled_group  = universal_group<typename AdaptTy::cell_type, AdaptTy>;

}
