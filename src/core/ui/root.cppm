module;

#include <cassert>

export module mo_yanxi.ui.root;

export import mo_yanxi.ui.primitives;

import mo_yanxi.math.vector2;
import mo_yanxi.math.rect_ortho;

import mo_yanxi.heterogeneous;
import mo_yanxi.graphic.renderer.predecl;

import std;

namespace mo_yanxi::ui{
	namespace SceneName{
		constexpr std::string_view Main{"main"};
	}

	export
	template <typename T>
	struct scene_add_result{
		scene& scene;
		T& root_group;
	};

	export
	struct root{
		[[nodiscard]] root() = default;

		[[nodiscard]] explicit root(scene&& scene){
			auto name = std::string(scene.get_name());
			focus = &scenes.insert_or_assign(std::move(name), std::move(scene)).first->second;
		}

	private:
		string_hash_map<scene> scenes{};
		scene* focus{};

	public:

		scene* switch_scene_to(std::string_view name) noexcept{
			if(auto scene = scenes.try_find(name)){
				return std::exchange(focus, scene);
			}
			return nullptr;
		}

		scene& get_current_focus() noexcept{
			return *focus;
		}

		scene& add_scene(scene&& scene, bool focusIt = false){
			auto name = std::string(scene.get_name());

			auto itr = scenes.insert_or_assign(std::move(name), std::move(scene));
			if(focusIt){
				focus = std::addressof(itr.first->second);
			}

			return itr.first->second;
		}

		template <std::derived_from<basic_group> T, typename ...Args>
			requires (std::constructible_from<T, scene*, group*, Args&&...>)
		scene_add_result<T> add_scene(
			std::string_view name,
			const rect region,
			graphic::renderer_ui_ptr renderer_ui,
			Args&&... args){
			auto& scene = this->add_scene(
				ui::scene{
					name, new T{nullptr, nullptr, std::forward<Args>(args)...},
					renderer_ui
				});

			this->resize(region, name);
			auto& root = this->root_of<T>(name);
			return {scene, root};
		}

		bool erase_scene(std::string_view name) /*noexcept*/ {
			if(auto itr = scenes.find(name); itr != scenes.end()){
				if(std::addressof(itr->second) == focus){
					if(name == SceneName::Main){
						throw std::runtime_error{"erase main while focusing it"};
					}
					focus = &scenes.at(SceneName::Main);
				}

				scenes.erase(itr);

				return true;
			}
			return false;
		}

		void draw() const{
			if(focus)focus->root_draw();
		}

		void update(const float delta_in_ticks) const{
			if(focus)focus->update(delta_in_ticks);
		}

		void layout() const{
			if(focus)focus->layout();
		}

		void input_key(const int key, const int action, const int mode) const{
			if(focus)focus->on_key_action(key, action, mode);
		}

		void input_scroll(const float x, const float y) const{
			if(focus)focus->on_scroll({x, y});
		}

		void input_mouse(const core::ctrl::key_code_t key, const core::ctrl::key_code_t action, const core::ctrl::key_code_t mode) const{
			if(focus)focus->on_mouse_action(key, action, mode);
		}

		void input_unicode(const char32_t val) const{
			if(focus)focus->on_unicode_input(val);
		}

		void cursor_pos_update(const float x, const float y) const{
			if(focus)focus->on_cursor_pos_update({x, y}, false);
		}

		scene* get_scene(const std::string_view sceneName){
			return scenes.try_find(sceneName);
		}

		template <typename T>
		[[nodiscard]] T& root_of(const std::string_view sceneName){
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

		void resize_all(const math::frect region){
			for (auto & scene : scenes | std::views::values){
				scene.resize(region);
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
