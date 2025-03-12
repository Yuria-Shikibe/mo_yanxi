module;

#include <cassert>

export module mo_yanxi.core.ctrl.key_binding;

export import mo_yanxi.core.ctrl.constants;
export import mo_yanxi.core.ctrl.key_pack;
import std;
// import ext.concepts;
import mo_yanxi.algo;
import mo_yanxi.referenced_ptr;

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

		void try_exec(const key_code_t act, const key_code_t mode, ParamTy... args) const{
			if(matched(act, mode)) this->exec(std::forward<ParamTy>(args)...);
		}

		void exec(ParamTy... args) const{
			func(this->pack(), std::forward<ParamTy>(args)...);
		}

		friend constexpr bool operator==(const key_binding& lhs, const key_binding& rhs) = default;
	};

	export
	template <typename ...ParamTy>
	class key_mapping_sockets{
	public:
		using bind_type = key_binding<ParamTy...>;
		using bind_func_t = typename bind_type::function_ptr;
		typename bind_type::params_type context{};

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

				case act::Repeat : pushIn = Repeat | Continuous;
					break;

				default : break;
			}

			signals[code] = pushIn | (mode & mode::Mask);

			return isDoubleClick;
		}

		void exec(const bind_type& bind, const key_code_t act, const key_code_t mode){
			[&, this]<std::size_t ...Idx>(std::index_sequence<Idx...>){
				bind.try_exec(act, mode, std::forward<std::tuple_element_t<Idx, typename bind_type::params_type>>(std::get<Idx>(context))...);
			}(std::make_index_sequence<std::tuple_size_v<typename bind_type::params_type>>{});
		}
	public:

		void set_context(ParamTy... param_ty){
			context = {param_ty...};
		}

		[[nodiscard]] bool triggered(const key_code_t code, const key_code_t action, const key_code_t mode) const noexcept{
			const signal target = signals[code];

			if(!mode::matched(target, mode)) return false;

			signal actionTgt{};
			switch(action){
				case act::press : actionTgt = press;
					break;
				case act::Continuous : actionTgt = Continuous;
					break;
				case act::release : actionTgt = Release;
					break;
				case act::Repeat : actionTgt = Repeat;
					break;
				case act::DoubleClick : actionTgt = DoubleClick;
					break;
				case act::Ignore : actionTgt = press | Continuous | Release | Repeat | DoubleClick;
					break;
				default : std::unreachable();
			}

			return target & actionTgt;
		}

		[[nodiscard]] bool triggered(const key_code_t key) const noexcept{
			return static_cast<bool>(signals[key]);
		}

		[[nodiscard]] key_code_t get_mode() const noexcept{
			const auto shift = triggered(key::Left_Shift) || triggered(key::Right_Shift) ? mode::Shift : mode::None;
			const auto ctrl = triggered(key::Left_Control) || triggered(key::Right_Control) ? mode::Ctrl : mode::None;
			const auto alt = triggered(key::Left_Alt) || triggered(key::Right_Alt) ? mode::Alt : mode::None;
			const auto caps = triggered(key::CapsLock) ? mode::CapLock : mode::None;
			const auto nums = triggered(key::NumLock) ? mode::NumLock : mode::None;
			const auto super = triggered(key::Left_Super) || triggered(key::Right_Super) ? mode::Super : mode::None;

			return shift | ctrl | alt | caps | nums | super;
		}

		[[nodiscard]] bool has_mode(const key_code_t mode) const noexcept{
			return (get_mode() & mode) == mode;
		}

		void inform(const key_code_t code, const key_code_t action, const key_code_t mods){
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

		void update(const float delta_in_tick){
			for(const key_code_t key : pressed){
				for(const auto& bind : binds_continuous[key]){
					this->exec(bind, act::Continuous, signals[key] & mode::Mask);
				}
			}

			algo::erase_if_unstable(doubleClickTimers, [delta_in_tick](DoubleClickTimer& timer){
				timer.time -= delta_in_tick;
				return timer.time <= 0.f;
			});
		}

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

	export
	template <typename ...ParamTy>
	class key_mapping : public key_mapping_sockets<ParamTy...>, public referenced_object<false>{
		using base = key_mapping_sockets<ParamTy...>;
		bool activated{true};

	public:
		void update(const float delta_in_tick){
			if(!activated) return;

			base::update(delta_in_tick);
		}

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

		using referenced_object::decr_ref;
		using referenced_object::incr_ref;
	};

}
