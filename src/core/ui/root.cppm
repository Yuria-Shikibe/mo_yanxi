module;

#include <cassert>

export module mo_yanxi.ui.root;

export import mo_yanxi.ui.pre_decl;
export import mo_yanxi.ui.scene;

import mo_yanxi.math.vector2;
import mo_yanxi.math.rect_ortho;

import mo_yanxi.heterogeneous;
import std;

namespace mo_yanxi::ui{
	namespace SceneName{
		constexpr std::string_view Main{"main"};
	}


	export
	struct root{
		[[nodiscard]] root() = default;

		[[nodiscard]] explicit root(scene&& scene){
			auto name = std::string(scene.name);
			focus = &scenes.insert_or_assign(std::move(name), std::move(scene)).first->second;
		}

		string_hash_map<scene> scenes{};

		scene* focus{};

		void add_scene(scene&& scene, bool focusIt = false){
			auto name = std::string(scene.name);

			auto itr = scenes.insert_or_assign(std::move(name), std::move(scene));
			if(focusIt){
				focus = std::addressof(itr.first->second);
			}
		}

		void draw() const{
			assert(focus != nullptr);
			focus->draw();
		}

		void update(const float delta_in_ticks) const{
			assert(focus != nullptr);
			focus->update(delta_in_ticks);
		}

		void layout() const{
			assert(focus != nullptr);
			focus->layout();
		}

		void input_key(const int key, const int action, const int mode) const{
			assert(focus != nullptr);
			focus->on_key_action(key, action, mode);
		}

		void input_scroll(const float x, const float y) const{
			assert(focus != nullptr);
			focus->on_scroll({x, y});
		}

		void input_mouse(const core::ctrl::key_code_t key, const core::ctrl::key_code_t action, const core::ctrl::key_code_t mode) const{
			assert(focus != nullptr);
			focus->on_mouse_action(key, action, mode);
		}

		void input_unicode(const char32_t val) const{
			assert(focus != nullptr);
			focus->on_unicode_input(val);
		}

		void cursor_pos_update(const float x, const float y) const{
			assert(focus != nullptr);
			focus->on_cursor_pos_update({x, y});
		}

		scene* get_scene(const std::string_view sceneName){
			return scenes.try_find(sceneName);
		}

		template <typename T>
		T& root_of(const std::string_view sceneName){
			if(const auto rst = scenes.try_find(sceneName)){
				return dynamic_cast<T&>(*rst->root);
			}

			std::println(std::cerr, "Scene {} Not Found", sceneName);
			throw std::invalid_argument{"In-exist Scene Name"};
		}

		void resize(const math::frect region, const std::string_view name = SceneName::Main){
			if(const auto rst = scenes.try_find(name)){
				rst->resize(region);
			}
		}

		[[nodiscard]] bool scrollIdle() const noexcept{
			return !focus || focus->currentScrollFocus == nullptr;
		}

		[[nodiscard]] bool focusIdle() const noexcept{
			return !focus || focus->currentCursorFocus == nullptr;
		}
	};
}
