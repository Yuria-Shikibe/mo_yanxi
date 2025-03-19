module;

#include "../src/ext/adapted_attributes.hpp"

export module mo_yanxi.ui.action;

import mo_yanxi.math.timed;
import mo_yanxi.meta_programming;
import std;

namespace mo_yanxi::ui::action{
	export using interp_type = std::function<float(float)>;

	export
	template <typename T>
	struct action{
		using target_type = T;
		math::timed scale{};

	public:
		virtual ~action() = default;

		interp_type interp_func{};

		action() = default;

		action(const float lifetime, const interp_type& interpFunc)
			: scale(lifetime),
			  interp_func(interpFunc){}

		explicit action(const float lifetime)
			: scale(lifetime){}

		/**
		 * @return The excess time, it > 0 means this action has been done
		 */
		virtual float update(const float delta, T& elem){
			float ret = -1.0f;

			if(scale.time == 0.0f){
				this->begin(elem);
				scale.time = std::numeric_limits<float>::min();
			}

			if(const auto remain = scale.time - scale.lifetime; remain >= 0){
				return remain + delta;
			}

			scale.time += delta;

			if(const auto remain = scale.time - scale.lifetime; remain >= 0){
				ret = remain;

				scale.time = scale.lifetime;
				this->apply(elem, get_progress_of(1.f));
				this->end(elem);
			}else{
				this->apply(elem, get_progress_of(scale.get()));
			}

			return ret;
		}

	protected:
		[[nodiscard]] float get_progress_of(const float f) const{
			return interp_func ? interp_func(f) : f;
		}

		virtual void apply(T& elem, float progress){}

		virtual void begin(T& elem){}

		virtual void end(T& elem){}
	};

	export
	template <typename T>
	struct parallel_action : action<T>{

	protected:
		void select_max_lifetime(){
			for(auto& action : actions){
				this->scale.lifetime = std::max(this->scale.lifetimem, action->scale.lifetime);
			}
		}

	public:
		std::vector<std::unique_ptr<action<T>>> actions{};

		explicit parallel_action(std::vector<std::unique_ptr<action<T>>>&& actions)
			: actions(std::move(actions)){
			select_max_lifetime();
		}

		explicit(false) parallel_action(const std::initializer_list<std::unique_ptr<action<T>>> actions)
			: actions(actions){
			select_max_lifetime();
		}

		float update(const float delta, T& elem) override{
			std::erase_if(actions, [&](std::unique_ptr<action<T>>& act){
				return act->update(delta, elem) >= 0;
			});

			return action<T>::update(delta, elem);
		}
	};

	export
	template <typename T>
	struct aligned_parallel_action : action<T>{

	protected:
		void selectMaxLifetime(){
			for(auto& action : actions){
				this->scale.lifetime = std::max(this->scale.lifetimem, action->scale.lifetime);
			}
		}

	public:
		std::vector<std::unique_ptr<action<T>>> actions{};

		explicit aligned_parallel_action(const float lifetime, const interp_type& interpFunc, std::vector<std::unique_ptr<action<T>>>&& actions)
			: action<T>{lifetime, interpFunc}, actions(std::move(actions)){
			selectMaxLifetime();
		}

		aligned_parallel_action(const float lifetime, const interp_type& interpFunc, const std::initializer_list<std::unique_ptr<action<T>>> actions)
			: action<T>{lifetime, interpFunc}, actions(actions){
			selectMaxLifetime();
		}

	protected:
		void apply(T& elem, float progress) override{
			for (auto && action : actions){
				action->apply(elem, action.get_progress_of(progress));
			}
		}

		void begin(T& elem) override{
			for (auto && action : actions){
				action->begin(elem);
			}
		}

		void end(T& elem) override{
			for (auto && action : actions){
				action->end(elem);
			}
		}
	};
	
	export
	template <typename T>
	struct DelayAction : action<T>{
		explicit DelayAction(const float lifetime) : action<T>(lifetime){}

		float update(const float delta, T& elem) override{
			this->scale.time += delta;

			if(const auto remain = this->scale.time - this->scale.lifetime; remain >= 0){
				return remain;
			}

			return -1.f;
		}
	};

	template <typename T>
	struct EmptyApplyFunc{
		void operator()(T&, float){}
	};

	export
	template <typename T, std::invocable<T&, float> FuncApply, std::invocable<T&> FuncBegin = std::identity, std::invocable<T&> FuncEnd = std::identity>
	struct RunnableAction : action<T>{
		ADAPTED_NO_UNIQUE_ADDRESS std::decay_t<FuncApply> funcApply{};
		ADAPTED_NO_UNIQUE_ADDRESS std::decay_t<FuncBegin> funcBegin{};
		ADAPTED_NO_UNIQUE_ADDRESS std::decay_t<FuncEnd> funcEnd{};

		[[nodiscard]] RunnableAction() = default;

		explicit RunnableAction(FuncApply&& apply, FuncBegin&& begin = {}, FuncEnd&& end = {}) :
			funcApply{std::forward<FuncApply>(apply)},
			funcBegin{std::forward<FuncBegin>(begin)},
			funcEnd{std::forward<FuncEnd>(end)}{}

		explicit RunnableAction(FuncBegin&& begin = {}, FuncEnd&& end = {}) :
			funcBegin{std::forward<FuncBegin>(begin)},
			funcEnd{std::forward<FuncEnd>(end)}{}

		RunnableAction(const RunnableAction& other)
			noexcept(std::is_nothrow_copy_constructible_v<FuncApply> && std::is_nothrow_copy_constructible_v<FuncBegin> && std::is_nothrow_copy_constructible_v<FuncEnd>)
			requires (std::is_copy_constructible_v<FuncApply> && std::is_copy_constructible_v<FuncBegin> && std::is_copy_constructible_v<FuncEnd>) = default;

		RunnableAction(RunnableAction&& other)
			noexcept(std::is_nothrow_move_constructible_v<FuncApply> && std::is_nothrow_move_constructible_v<FuncBegin> && std::is_nothrow_move_constructible_v<FuncEnd>)
			requires (std::is_move_constructible_v<FuncApply> && std::is_move_constructible_v<FuncBegin> && std::is_move_constructible_v<FuncEnd>) = default;

		RunnableAction& operator=(const RunnableAction& other) 
			noexcept(std::is_nothrow_copy_assignable_v<FuncApply> && std::is_nothrow_copy_assignable_v<FuncBegin> && std::is_nothrow_copy_assignable_v<FuncEnd>)
			requires (std::is_copy_assignable_v<FuncApply> && std::is_copy_assignable_v<FuncBegin> && std::is_copy_assignable_v<FuncEnd>) = default;

		RunnableAction& operator=(RunnableAction&& other)
			noexcept(std::is_nothrow_move_assignable_v<FuncApply> && std::is_nothrow_move_assignable_v<FuncBegin> && std::is_nothrow_move_assignable_v<FuncEnd>)
			requires (std::is_move_assignable_v<FuncApply> && std::is_move_assignable_v<FuncBegin> && std::is_move_assignable_v<FuncEnd>) = default;

		void begin(T& elem) override{
			std::invoke(funcBegin, elem);
		}

		void end(T& elem) override{
			std::invoke(funcEnd, elem);
		}

		void apply(T& elem, float v) override{
			std::invoke(funcApply, elem, v);
		}
	};

	template <typename FuncApply, typename FuncBegin = std::identity, typename FuncEnd = std::identity>
	RunnableAction(FuncApply&&, FuncBegin&&, FuncEnd&&) ->
		RunnableAction<std::decay_t<std::tuple_element_t<0, remove_mfptr_this_args<std::decay_t<FuncApply>>>>,
		               FuncApply, FuncBegin, FuncEnd>;

	template <typename FuncBegin = std::identity, typename FuncEnd = std::identity>
	RunnableAction(FuncBegin&&, FuncEnd&&) ->
		RunnableAction<
			std::decay_t<std::tuple_element_t<0, remove_mfptr_this_args<std::decay_t<FuncBegin>>>>,
			EmptyApplyFunc<std::decay_t<std::tuple_element_t<0, remove_mfptr_this_args<std::decay_t<FuncBegin>>>>>,
			FuncBegin,
			FuncEnd>;
	
	template <typename FuncBegin = std::identity, typename FuncEnd = std::identity>
	RunnableAction(FuncBegin&&, FuncEnd&&) ->
		RunnableAction<
			std::decay_t<std::tuple_element_t<0, remove_mfptr_this_args<std::decay_t<FuncEnd>>>>,
			EmptyApplyFunc<std::decay_t<std::tuple_element_t<0, remove_mfptr_this_args<std::decay_t<FuncEnd>>>>>,
			FuncBegin,
			FuncEnd>;

}
