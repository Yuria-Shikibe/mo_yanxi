module;

#include <cassert>
#include "../src/ext/adapted_attributes.hpp"

export module mo_yanxi.core.ctrl.key_binding;

export import mo_yanxi.core.ctrl.constants;
export import mo_yanxi.core.ctrl.key_pack;
import std;
// import ext.concepts;
import mo_yanxi.algo;
import mo_yanxi.referenced_ptr;
import mo_yanxi.refable_tuple;

namespace mo_yanxi::core::ctrl{
 	export
	template <typename ...ParamTy>
	struct key_binding : unpacked_key{
 		using params_type = std::tuple<ParamTy...>;

 		using function_ptr = void(*)(packed_key_t, ParamTy...);
		// int expectedKey{Unknown};
		// int expectedAct{act::press};
		// int expectedMode{};

		function_ptr func;

		[[nodiscard]] explicit key_binding(const unpacked_key key, const function_ptr func)
			: unpacked_key{key}, func(func){
		}

		[[nodiscard]] key_binding() = default;


		[[nodiscard]] key_binding(const key_code_t key, const key_code_t expectedAct, const key_code_t mode, const function_ptr func)
			: key_binding{unpacked_key{key, expectedAct, mode}, func}{}

		[[nodiscard]] key_binding(const key_code_t key, const key_code_t expectedAct, const function_ptr func)
			: key_binding{unpacked_key{key, expectedAct, mode::ignore}, func}{}

		[[nodiscard]] key_binding(const key_code_t key, const function_ptr func)
 			: key_binding(key, act::press, func){}

		[[nodiscard]] constexpr bool matched(const key_code_t act, const key_code_t mode) const noexcept{
			return act::matched(act, this->act) && mode::matched(mode, this->mode);
		}

		FORCE_INLINE void try_exec(const key_code_t act, const key_code_t mode, ParamTy... args) const{
			if(matched(act, mode)) this->exec(std::forward<ParamTy>(args)...);
		}

		FORCE_INLINE void exec(ParamTy... args) const{
			func(this->pack(), std::forward<ParamTy>(args)...);
		}

		friend constexpr bool operator==(const key_binding& lhs, const key_binding& rhs) = default;
	};

	export
	struct key_mapping_interface : public referenced_object<false>{
	public:
		using referenced_object::decr_ref;
		using referenced_object::incr_ref;

		bool activated{true};

		void update(const float delta_in_tick){
			if(!activated) return;

			update_impl(delta_in_tick);
		}


		//TODO wtf is these shit setter/getters
		void set_activated(const bool active) noexcept{
			activated = active;
		}

		[[nodiscard]] bool is_activated() const noexcept{
			return activated;
		}

		void activate() noexcept{
			activated = true;
		}

		void deactivate() noexcept{
			activated = false;
		}


		virtual ~key_mapping_interface() = default;

		[[nodiscard]] virtual bool triggered(key_code_t key, key_code_t act, key_code_t mode) const noexcept = 0;
		virtual void inform(key_code_t code, key_code_t action, key_code_t mods) = 0;

		[[nodiscard]] bool triggered(const key_code_t key) const noexcept{
			return triggered(key, act::ignore, mode::ignore);
		}

		[[nodiscard]] key_code_t get_mode() const noexcept{
			const auto shift = triggered(key::Left_Shift) || triggered(key::Right_Shift) ? mode::shift : mode::none;
			const auto ctrl = triggered(key::Left_Control) || triggered(key::Right_Control) ? mode::ctrl : mode::none;
			const auto alt = triggered(key::Left_Alt) || triggered(key::Right_Alt) ? mode::alt : mode::none;
			const auto caps = triggered(key::CapsLock) ? mode::CapLock : mode::none;
			const auto nums = triggered(key::NumLock) ? mode::NumLock : mode::none;
			const auto super = triggered(key::Left_Super) || triggered(key::Right_Super) ? mode::Super : mode::none;

			return shift | ctrl | alt | caps | nums | super;
		}

		[[nodiscard]] bool has_mode(const key_code_t mode) const noexcept{
			return (get_mode() & mode) == mode;
		}

	protected:
		virtual void update_impl(float delta_in_tick) = 0;
	};

	export
	template <typename ...ParamTy>
	class key_mapping final : public key_mapping_interface{
	public:
		using bind_type = key_binding<ParamTy...>;
		using bind_func_t = typename bind_type::function_ptr;
		using context_tuple_t = refable_tuple<ParamTy...>;
		context_tuple_t context{};

	private:
		struct DoubleClickTimer{
			key_code_t key{};
			float time{};

			[[nodiscard]] DoubleClickTimer() = default;

			[[nodiscard]] explicit DoubleClickTimer(const key_code_t key)
				: key{key}, time{act::doublePressMaxSpacing}{}
		};
		
		using signal = unsigned short;
		/**
		 * @code
		 * | 0b'0000'0000'0000'0000
		 * |         CRrp|-- MOD --
		 * | C - Continuous
		 * | R - Repeat
		 * | r - release
		 * | p - press
		 * @endcode
		 */
		static constexpr signal DoubleClick = 0b0001'0000'0000'0000;
		static constexpr signal Continuous = 0b1000'0000'0000;
		static constexpr signal Repeat = 0b0100'0000'0000;
		static constexpr signal Release = 0b0010'0000'0000;
		static constexpr signal press = 0b0001'0000'0000;
		// static constexpr Signal RP_Eraser = ~(Release | press | DoubleClick | Repeat);

		std::array<std::vector<bind_type>, AllKeyCount> binds{};
		std::array<std::vector<bind_type>, AllKeyCount> binds_continuous{};
		std::array<signal, AllKeyCount> signals{};

		//OPTM using array instead of vector
		std::vector<signal> pressed{};
		std::vector<DoubleClickTimer> doubleClickTimers{};

		void updateSignal(const key_code_t key, const key_code_t act){
			switch(act){
				case act::press :{
					if(!std::ranges::contains(pressed.crbegin(), pressed.crend(), key)){
						pressed.push_back(key);
					}
					break;
				}

				case act::release :{
					signals[key] = 0;
					algo::erase_unique_unstable(pressed, key);
					break;
				}

				default : break;
			}
		}

		/**
		 * @return isDoubleClick
		 */
		bool insert(const key_code_t code, const key_code_t action, const key_code_t mode) noexcept{
			signal pushIn = signals[code] & ~mode::Mask;

			bool isDoubleClick = false;

			switch(action){
				case act::press :{
					if(const auto itr = std::ranges::find(doubleClickTimers, code, &DoubleClickTimer::key); itr !=
						doubleClickTimers.end()){
						doubleClickTimers.erase(itr);
						isDoubleClick = true;
						pushIn = press | Continuous | DoubleClick;
					} else{
						doubleClickTimers.emplace_back(code);
						pushIn = press | Continuous;
					}

					break;
				}

				case act::release :{
					pushIn = Release;
					// pressed[code] &= ~Continuous;
					break;
				}

				case act::repeat : pushIn = Repeat | Continuous;
					break;

				default : break;
			}

			signals[code] = pushIn | (mode & mode::Mask);

			return isDoubleClick;
		}

		void exec(const bind_type& bind, const key_code_t act, const key_code_t mode){
			[&, this]<std::size_t ...Idx>(std::index_sequence<Idx...>){
				bind.try_exec(act, mode, std::get<Idx>(context)...);
			}(std::make_index_sequence<std::tuple_size_v<typename bind_type::params_type>>{});
		}
	public:
		[[nodiscard]] key_mapping() = default;

		template <typename ...Args>
			requires requires(context_tuple_t& ctx, Args&& ...args){
				requires sizeof...(Args) > 1;
				ctx = mo_yanxi::make_refable_tuple(args...);
			}
		void set_context(Args&& ...param_ty){
			context = mo_yanxi::make_refable_tuple(param_ty...);
		}

		template <std::size_t Idx, typename Arg>
			requires std::assignable_from<std::add_lvalue_reference_t<std::tuple_element_t<Idx, context_tuple_t>>, Arg>
		void set_context(Arg&& param_ty){
			std::get<Idx>(context) = std::forward<Arg>(param_ty);
		}

		template <typename Arg>
			requires requires{
				requires std::assignable_from<decltype(std::get<refable_tuple_decay_ref<std::unwrap_ref_decay_t<Arg>>>(context)), Arg>;
			}
		void set_context(Arg&& param_ty){
			std::get<refable_tuple_decay_ref<std::unwrap_ref_decay_t<Arg>>>(context) = std::forward<Arg>(param_ty);
		}

		[[nodiscard]] bool triggered(const key_code_t code, const key_code_t action, const key_code_t mode) const noexcept override{
			const signal target = signals[code];

			if(!mode::matched(target, mode)) return false;

			signal actionTgt{};
			switch(action){
				case act::press : actionTgt = press;
					break;
				case act::continuous : actionTgt = Continuous;
					break;
				case act::release : actionTgt = Release;
					break;
				case act::repeat : actionTgt = Repeat;
					break;
				case act::DoubleClick : actionTgt = DoubleClick;
					break;
				case act::ignore : actionTgt = press | Continuous | Release | Repeat | DoubleClick;
					break;
				default : std::unreachable();
			}

			return target & actionTgt;
		}

		void inform(const key_code_t code, const key_code_t action, const key_code_t mods) override{
			const bool doubleClick = insert(code, action, mods);

			for(const auto& bind : binds[code]){
				this->exec(bind, action, mods);
				if(doubleClick){
					//TODO better double click event
					this->exec(bind, act::DoubleClick, mods);
				}
			}

			updateSignal(code, action);
		}

	protected:
		void update_impl(const float delta_in_tick) override{
			for(const key_code_t key : pressed){
				for(const auto& bind : binds_continuous[key]){
					this->exec(bind, act::continuous, signals[key] & mode::Mask);
				}
			}

			algo::erase_if_unstable(doubleClickTimers, [delta_in_tick](DoubleClickTimer& timer){
				timer.time -= delta_in_tick;
				return timer.time <= 0.f;
			});
		}
	public:

		void register_bind(bind_type&& bind){
			assert(bind.key < AllKeyCount && bind.key >= 0);
			
			auto& container = act::is_continuous(bind.act) ? binds_continuous : binds;

			container[bind.key].emplace_back(std::move(bind));
		}

		void register_bind(const bind_type& bind){
			this->register_bind(key_binding{bind});
		}

		void clear() noexcept{
			std::ranges::for_each(binds, &std::vector<bind_type>::clear);
			std::ranges::for_each(binds_continuous, &std::vector<bind_type>::clear);
			signals.fill(0);
			pressed.clear();
			doubleClickTimers.clear();
		}

		void register_bind(const key_code_t key, const key_code_t expectedState, const key_code_t expectedMode, bind_func_t func){
			this->register_bind(key_binding{key, expectedState, expectedMode, func});
		}

		void register_bind(const key_code_t key, const key_code_t expectedState, bind_func_t func){
			this->register_bind(key_binding{key, expectedState, func});
		}

		void register_bind(const key_code_t key, const key_code_t expectedMode, bind_func_t onPress, bind_func_t onRelease){
			this->register_bind(key_binding{key, act::press, expectedMode, onPress});
			this->register_bind(key_binding{key, act::release, expectedMode, onRelease});
		}

		void register_bind(std::initializer_list<std::pair<key_code_t, key_code_t>> pairs, bind_func_t func){
			for(auto [key, expectedState] : pairs){
				this->register_bind(key_binding{key, expectedState, func});
			}
		}

		void register_bind(std::initializer_list<std::pair<key_code_t, key_code_t>> pairs, const key_code_t expectedMode, bind_func_t func){
			for(auto [key, expectedState] : pairs){
				this->register_bind(key_binding{key, expectedState, expectedMode, func});
			}
		}

		void register_bind(std::initializer_list<unpacked_key> params, bind_func_t func){
			for(auto [key, expectedState, expectedMode] : params){
				this->register_bind(key_binding{key, expectedState, expectedMode, func});
			}
		}
	};
}
