//
// Created by Matrix on 2024/9/24.
//

export module mo_yanxi.ui.elem.button;

export import mo_yanxi.ui.elem;
export import mo_yanxi.core.ctrl.key_pack;
import std;

namespace mo_yanxi::ui{
	template <typename T>
	struct Unsupported : std::false_type{};

	namespace button_tags{
		export enum tag_enum : unsigned{
			none = 0,
			ignore_key = 1 << 0,
		};

		export
		template <tag_enum Tag, core::ctrl::key_pack specKeyPack>
		struct invoke_tag{
			static constexpr auto code = specKeyPack;
			static constexpr tag_enum tag = Tag;
		};

		export {
			constexpr auto forward = invoke_tag<ignore_key, core::ctrl::key_pack{0, core::ctrl::act::ignore, core::ctrl::mode::ignore}>{};
			constexpr auto general = invoke_tag<ignore_key, core::ctrl::key_pack{0, core::ctrl::act::release, core::ctrl::mode::ignore}>{};
		}
	}

	export
	template <std::derived_from<elem> T>
	struct button : public T{
	protected:
		using CallbackType = std::move_only_function<void(events::click, button&)>;
		CallbackType callback{};

		void add_button_prop(){
			elem::cursor_state.registerDefEvent(elem::events());

			elem::register_event([](const events::click& e, button& self){
				if(self.disabled)return;
				if(self.callback && self.elem::contains(e.pos))self.callback(e, self);
			});
			
			elem::interactivity = interactivity::enabled;
			elem::property.maintain_focus_until_mouse_drop = true;
		}

	public:
		
		[[nodiscard]] button(scene* s, group* g)
			requires (std::constructible_from<T, scene*, group*>)
		: T{s, g}{
			add_button_prop();
		}
		
		[[nodiscard]] button(scene* s, group* g)
			requires (std::constructible_from<T, scene*, group*, std::string_view>)
		: T{s, g, "button<T>"}{
			add_button_prop();
		}


		void set_button_callback(CallbackType&& func){
			callback = std::move(func);
		}

	private:
		template <button_tags::tag_enum tag, core::ctrl::key_pack keyPack, std::invocable<button&, events::click> Func>
		auto assignTagToFunc(Func&& func){
			if constexpr(tag & button_tags::ignore_key){
				if constexpr(keyPack.action() == core::ctrl::act::ignore && keyPack.mode() == core::ctrl::mode::ignore){
					return std::forward<Func>(func);
				} else{
					return [func = std::forward<Func>(func)](button& b, const events::click e){
						if(core::ctrl::act::matched(e.code.action(), keyPack.action()) && core::ctrl::mode::matched(e.code.mode(), keyPack.mode())){
							std::invoke(func, b, e);
						}
					};
				}
			} else{
				return [func = std::forward<Func>(func)](button& b, const events::click e){
					if(e.code.matched(keyPack)){
						std::invoke(func, b, e);
					}
				};
			}
		}

	public:
		template <button_tags::tag_enum tag, core::ctrl::key_pack keyPack, typename Func>
		void set_button_callback(button_tags::invoke_tag<tag, keyPack>, Func&& callback){
			if constexpr(std::invocable<Func, events::click, button&> && std::assignable_from<CallbackType, Func>){
				this->callback = button::assignTagToFunc<tag, keyPack>(std::forward<Func>(callback));
			}else if constexpr (std::invocable<Func, events::click>){
				this->callback = button::assignTagToFunc<tag, keyPack>([func = std::forward<Func>(callback)](const events::click e, button& b){
					std::invoke(func, e);
				});
			}else if constexpr (std::invocable<Func>){
				this->callback = button::assignTagToFunc<tag, keyPack>([func = std::forward<Func>(callback)](const events::click e, button& b){
					std::invoke(func);
				});
			}else if constexpr (std::invocable<Func, button&>){
				this->callback = button::assignTagToFunc<tag, keyPack>([func = std::forward<Func>(callback)](const events::click e, button& b){
					std::invoke(func, b);
				});
			}else{
				static_assert(Unsupported<Func>::value, "unsupported function type");
			}
		}

		template <std::invocable<> Func>
		void set_button_callback(Func&& callback){
			this->callback = [func = std::forward<Func>(callback)](const events::click e, button& b){
				std::invoke(func);
			};
		}

		template <std::invocable<button&> Func>
		void set_button_callback(Func&& callback){
			this->callback = [func = std::forward<Func>(callback)](const events::click e, button& b){
				std::invoke(func, b);
			};
		}

		template <std::invocable<events::click> Func>
		void set_button_callback(Func&& callback){
			this->callback = [func = std::forward<Func>(callback)](const events::click e, button& b){
				std::invoke(func, e);
			};
		}

		template <std::invocable<events::click, button&> Func>
		void set_button_callback(Func&& callback){
			this->callback = std::forward<Func>(callback);
		}
	};

}
