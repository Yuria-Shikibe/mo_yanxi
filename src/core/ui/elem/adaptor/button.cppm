//
// Created by Matrix on 2024/9/24.
//

export module mo_yanxi.ui.elem.button;

export import mo_yanxi.ui.primitives;
export import mo_yanxi.core.ctrl.key_pack;
import std;

namespace mo_yanxi::ui{
	template <typename T>
	struct Unsupported : std::false_type{};

	namespace button_tags{
		export constexpr core::ctrl::key_code_t ignore_code = 0;

		export
		template <core::ctrl::key_pack specKeyPack>
		struct invoke_tag{
			static constexpr auto code = specKeyPack;
		};

		export {
			constexpr auto forward = invoke_tag<core::ctrl::key_pack{ignore_code, core::ctrl::act::ignore, core::ctrl::mode::ignore}>{};
			constexpr auto general = invoke_tag<core::ctrl::key_pack{ignore_code, core::ctrl::act::release, core::ctrl::mode::ignore}>{};
		}
	}

	export
	template <std::derived_from<elem> T = elem>
	struct button : public T{
	protected:
		using callback_type = std::move_only_function<void(input_event::click, button&)>;
		callback_type callback{};

		void add_button_prop(){
			elem::cursor_state.registerDefEvent(elem::events());
			elem::interactivity = interactivity::enabled;
			elem::property.maintain_focus_until_mouse_drop = true;


		}

		input_event::click_result on_click(const input_event::click click_event) override {
			T::on_click(click_event);
			if(this->disabled)return input_event::click_result::intercepted;
			if(callback && elem::contains(click_event.pos))callback(click_event, *this);
			return input_event::click_result::intercepted;
		}

	public:

		template <typename ...Args>
		[[nodiscard]] button(scene* s, group* g, Args&& ...args)
			requires (std::constructible_from<T, scene*, group*, Args...> && !std::constructible_from<T, scene*, group*, std::string_view, Args...>)
		: T{s, g, std::forward<Args>(args) ...}{
			add_button_prop();
		}

		template <typename ...Args>
		[[nodiscard]] button(scene* s, group* g, Args&& ...args)
			requires (std::constructible_from<T, scene*, group*, std::string_view, Args...>)
		: T{s, g, "button<T>", std::forward<Args>(args) ...}{
			add_button_prop();
		}


		void set_button_callback(callback_type&& func){
			callback = std::move(func);
		}

	private:
		template <core::ctrl::key_pack keyPack, std::invocable<input_event::click, button&> Func>
		auto assignTagToFunc(Func&& func){
			if constexpr(keyPack.key() == button_tags::ignore_code){
				if constexpr(keyPack.action() == core::ctrl::act::ignore && keyPack.mode() == core::ctrl::mode::ignore){
					return std::forward<Func>(func);
				} else{
					return [func = std::forward<Func>(func)](const input_event::click e, button& b){
						if(core::ctrl::act::matched(e.code.action(), keyPack.action()) && core::ctrl::mode::matched(e.code.mode(), keyPack.mode())){
							std::invoke(func, e, b);
						}
					};
				}
			} else{
				return [func = std::forward<Func>(func)](const input_event::click e, button& b){
					if(e.code.matches(keyPack)){
						std::invoke(func, e, b);
					}
				};
			}
		}

	public:
		template <core::ctrl::key_pack keyPack, typename Func>
		void set_button_callback(button_tags::invoke_tag<keyPack>, Func&& callback){
			if constexpr(std::invocable<Func, input_event::click, button&>){
				this->callback = button::assignTagToFunc<keyPack>(std::forward<Func>(callback));
			}else if constexpr (std::invocable<Func, input_event::click>){
				this->callback = button::assignTagToFunc<keyPack>([func = std::forward<Func>(callback)](const input_event::click e, button& b){
					std::invoke(func, e);
				});
			}else if constexpr (std::invocable<Func>){
				this->callback = button::assignTagToFunc<keyPack>([func = std::forward<Func>(callback)](const input_event::click e, button& b){
					std::invoke(func);
				});
			}else if constexpr (std::invocable<Func, button&>){
				this->callback = button::assignTagToFunc<keyPack>([func = std::forward<Func>(callback)](const input_event::click e, button& b){
					std::invoke(func, b);
				});
			}else{
				static_assert(Unsupported<Func>::value, "unsupported function type");
			}
		}

		template <std::invocable<> Func>
		void set_button_callback(Func&& callback){
			this->callback = [func = std::forward<Func>(callback)](const input_event::click e, button& b){
				std::invoke(func);
			};
		}

		template <std::invocable<button&> Func>
		void set_button_callback(Func&& callback){
			this->callback = [func = std::forward<Func>(callback)](const input_event::click e, button& b){
				std::invoke(func, b);
			};
		}

		template <std::invocable<input_event::click> Func>
		void set_button_callback(Func&& callback){
			this->callback = [func = std::forward<Func>(callback)](const input_event::click e, button& b){
				std::invoke(func, e);
			};
		}

		template <std::invocable<input_event::click, button&> Func>
		void set_button_callback(Func&& callback){
			this->callback = std::forward<Func>(callback);
		}


		void set_button_callback_build_tooltip(){
			this->set_button_callback(button_tags::general, [](elem& elem){
				if(elem.has_tooltip()){
					elem.tooltip_notify_drop();
				}else{
					elem.build_tooltip();
				}
			});
		}

	};

}
