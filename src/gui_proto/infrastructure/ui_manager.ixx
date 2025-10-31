//
// Created by Matrix on 2025/11/1.
//

export module mo_yanxi.gui.infrastructure:ui_manager;

import mo_yanxi.heterogeneous;
import :scene;

namespace mo_yanxi::gui{

namespace scene_names{
constexpr std::string_view main{"main"};
}

export
template <typename T>
struct scene_add_result{
	scene& scene;
	T& root_group;
};

export
struct ui_manager{
	[[nodiscard]] ui_manager() = default;

	[[nodiscard]] explicit ui_manager(std::string name, scene&& scene){
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

	scene& add_scene(std::string_view name, scene&& scene, bool focusIt = false){
		auto itr = scenes.insert_or_assign(name, std::move(scene));
		if(focusIt){
			focus = std::addressof(itr.first->second);
		}

		return itr.first->second;
	}

	template <std::derived_from<elem> T, typename... Args>
		requires (std::constructible_from<T, scene&, elem*, Args&&...>)
	scene_add_result<T> add_scene(
		std::string_view name,
		gui::renderer& renderer_ui,
		Args&&... args){
		auto& scene_ = this->add_scene(
			name,
			scene{
				renderer_ui, std::in_place_type<T>,
				std::forward<Args>(args)...
			});

		auto& root = this->root_of<T>(name);
		return {scene_, root};
	}

	bool erase_scene(std::string_view name) /*noexcept*/{
		if(auto itr = scenes.find(name); itr != scenes.end()){
			if(std::addressof(itr->second) == focus){
				if(name == scene_names::main){
					throw std::runtime_error{"erase main while focusing it"};
				}
				focus = &scenes.at(scene_names::main);
			}

			scenes.erase(itr);

			return true;
		}
		return false;
	}

	void draw() const{
		if(focus) focus->draw();
	}

	void update(const float delta_in_ticks) const{
		if(focus) focus->update(delta_in_ticks);
	}

	// void layout() const{
	// 	if(focus) focus->layout();
	// }

	void input_key(const input_handle::key_set k) const{
		if(focus) focus->on_key_action(k);
	}

	void input_scroll(const float x, const float y) const{
		if(focus) focus->on_scroll({x, y});
	}


	void input_unicode(const char32_t val) const{
		if(focus) focus->on_unicode_input(val);
	}

	void cursor_pos_update(const float x, const float y) const{
		if(focus) focus->inform_cursor_move({x, y});
	}

	scene* get_scene(const std::string_view sceneName){
		return scenes.try_find(sceneName);
	}

	template <typename T>
	[[nodiscard]] T& root_of(const std::string_view sceneName){
		if(const auto rst = scenes.try_find(sceneName)){
			rst->root<T>();
		}

		std::println(std::cerr, "Scene {} Not Found", sceneName);
		throw std::invalid_argument{"In-exist Scene Name"};
	}

	void resize(const math::frect region, const std::string_view name = scene_names::main){
		if(const auto rst = scenes.try_find(name)){
			rst->resize(region);
		}
	}

	void resize_all(const math::frect region){
		for(auto& scene : scenes | std::views::values){
			scene.resize(region);
		}
	}

	[[nodiscard]] bool is_scroll_idle() const noexcept{
		return !focus || focus->focus_scroll_ == nullptr;
	}

	[[nodiscard]] bool is_focus_idle() const noexcept{
		return !focus || focus->focus_cursor_ == nullptr;
	}
};
}
