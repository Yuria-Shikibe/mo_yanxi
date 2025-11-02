module;

#include <cassert>

export module mo_yanxi.core.ctrl.main_input;

import std;

import mo_yanxi.math.vector2;
import mo_yanxi.concepts;
import mo_yanxi.heterogeneous;
export import mo_yanxi.core.ctrl.constants;
export import mo_yanxi.core.ctrl.key_binding;
// export import Core.InputListener;

export namespace mo_yanxi::core::ctrl{

	class main_input{
	public:

		using PosListener = std::function<void(float, float)>;
		std::vector<PosListener> scroll_listeners{};
		std::vector<PosListener> cursor_move_listeners{};
		std::vector<PosListener> velocity_listeners{};

		// std::vector<InputListener*> inputKeyListeners{};
		// std::vector<InputListener*> inputMouseListeners{};

		key_mapping<> main_binds{};

	protected:
		bool isInbound{false};

		math::vec2 mouse_pos_{};
		math::vec2 last_mouse_pos_{};
		math::vec2 mouse_velocity_{};
		math::vec2 scroll_offset_{};

		string_hash_map<std::unique_ptr<key_mapping_interface>> subInputs{};

	public:

		//TODO provide type conflict check?
		template <typename ...CtxArgs>
		key_mapping<CtxArgs...>& register_sub_input(const std::string_view mappingName){
			std::pair<decltype(subInputs)::iterator, bool> rst = subInputs.try_emplace(mappingName);
			if(rst.second){
				rst.first->second = std::make_unique<key_mapping<CtxArgs...>>();
			}
			rst.first->second->ref_incr();
			return static_cast<key_mapping<CtxArgs...>&>(*rst.first->second);
		}

		bool erase_sub_input(const std::string_view mappingName){
			if(const auto itr = subInputs.find(mappingName); itr != subInputs.end()){
				if(itr->second->ref_decr()){
					subInputs.erase(itr);
					return true;
				}
			}
			return false;
		}

		void inform(const key_code_t button, const key_code_t action, const key_code_t mods){
			main_binds.inform(button, action, mods);

			for(auto& subInput : subInputs | std::views::values){
				subInput->inform(button, action, mods);
			}
		}

		// /**
		//  * \brief No remove function provided. If the input binds needed to be changed, clear it all and register the new binds by the action table.
		//  * this is an infrequent operation so just keep the code clean.
		//  */
		void inform_mouse_action(const key_code_t button, const key_code_t action, const key_code_t mods){
			// for(const auto& listener : inputMouseListeners){
			// 	listener->inform(button, action, mods);
			// }

			inform(button, action, mods);
		}
		//
		void inform_key_action(const key_code_t key, const int scanCode, const key_code_t action, const key_code_t mods){
			// for(const auto& listener : inputKeyListeners){
			// 	listener->inform(key, action, mods);
			// }

			inform(key, action, mods);
		}

		void cursor_move_inform(const float x, const float y){
			last_mouse_pos_ = mouse_pos_;
			mouse_pos_.set(x, y);

			for(auto& listener : cursor_move_listeners){
				listener(x, y);
			}
		}

		[[nodiscard]] math::vec2 get_cursor_pos() const noexcept{
			return mouse_pos_;
		}

		[[nodiscard]] math::vec2 get_cursor_delta() const noexcept{
			return mouse_pos_ - last_mouse_pos_;
		}

		[[nodiscard]] math::vec2 get_last_cursor_pos() const noexcept{
			return last_mouse_pos_;
		}

		[[nodiscard]] math::vec2 get_scroll() const noexcept{
			return scroll_offset_;
		}

		void set_scroll_offset(const float x, const float y){
			scroll_offset_.set(-x, y);

			for(auto& listener : scroll_listeners){
				listener(-x, y);
			}
		}

		[[nodiscard]] bool is_cursor_inbound() const noexcept{
			return isInbound;
		}

		void set_inbound(const bool b) noexcept{
			isInbound = b;
		}

		void update(const float delta_in_tick){
			main_binds.update(delta_in_tick);

			mouse_velocity_ = mouse_pos_;
			mouse_velocity_ -= last_mouse_pos_;
			mouse_velocity_ /= delta_in_tick;

			for(auto& listener : velocity_listeners){
				listener(mouse_velocity_.x, mouse_velocity_.y);
			}

			for(auto& subInput : subInputs | std::views::values){
				subInput->update(delta_in_tick);
			}
		}
	};
}
