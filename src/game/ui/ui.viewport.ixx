//
// Created by Matrix on 2025/5/26.
//

export module mo_yanxi.game.ui.viewport;

export import mo_yanxi.ui.primitives;
export import mo_yanxi.graphic.camera;
export import mo_yanxi.graphic.renderer.ui;

import std;

namespace mo_yanxi::game::ui{
	export using namespace mo_yanxi::ui;

	export constexpr float viewport_default_radius = 5000;

	export
	template <std::derived_from<elem> E = elem>
	struct viewport : E{
	private:
		math::vec2 last_camera_{};
	protected:
		graphic::camera2 camera{};
		math::vec2 viewport_region{viewport_default_radius * 2, viewport_default_radius * 2};

	public:
		[[nodiscard]] viewport(scene* scene, group* group)
			requires (std::constructible_from<E, ui::scene*, ui::group*, std::string_view>)
			: E(scene, group, "viewport"){

			this->interactivity = interactivity::enabled;
			camera.speed_scale = 0;
		}

		[[nodiscard]] viewport(scene* scene, group* group)
			requires (std::constructible_from<E, ui::scene*, ui::group*> && !std::constructible_from<E, ui::scene*, ui::group*, std::string_view>)
			: E(scene, group){

			this->interactivity = interactivity::enabled;
			camera.speed_scale = 0;
		}

	protected:
		void update(const float delta_in_ticks) override{
			E::update(delta_in_ticks);
			const auto [w, h]{viewport_region - camera.get_viewport().extent()};
			camera.clamp_position({math::vec2{}, w, h});
			camera.update(delta_in_ticks);

		}

		bool resize(const math::vec2 size) override{
			if(E::resize(size)){
				auto [x, y] = this->content_size();
				camera.resize_screen(x, y);
				return true;
			}

			return false;
		}

		void on_focus_changed(bool is_focused) override{
			this->get_scene()->set_camera_focus(is_focused ? &camera : nullptr);
			this->set_focused_scroll(is_focused);
			this->set_focused_key(is_focused);
		}

		void on_scroll(const ui::input_event::scroll event) override{
			camera.set_scale_by_delta(event.delta.y * 0.05f);
		}

		void on_drag(const ui::input_event::drag e) override{
			if(e.code.key() == core::ctrl::mouse::CMB){
				auto src = get_transferred_pos(e.pos);
				auto dst = get_transferred_pos(e.dst);
				camera.set_center(last_camera_ - (dst - src));
			}
		}

		ui::input_event::click_result on_click(const ui::input_event::click click_event) override{
			if(click_event.code.key() == core::ctrl::mouse::CMB){
				last_camera_ = camera.get_stable_center();
				return ui::input_event::click_result::intercepted;
			}

			return E::on_click(click_event);
		}

		void viewport_begin() const {
			const auto proj = camera.get_world_to_uniformed();
			graphic::renderer_ui& r = graphic::renderer_from_erased(this->get_renderer());

			r.batch.push_projection(proj);
			r.batch.push_viewport(this->prop().content_bound_absolute());
			r.batch.push_scissor({camera.get_viewport()});
		}

		void viewport_end() const {
			graphic::renderer_ui& r = graphic::renderer_from_erased(this->get_renderer());

			r.batch.pop_scissor();
			r.batch.pop_viewport();
			r.batch.pop_projection();
		}

		[[nodiscard]] math::vec2 get_transferred_pos(const math::vec2 pos) const noexcept{
			return camera.get_screen_to_world(pos, this->content_src_pos(), true);
		}

		[[nodiscard]] math::vec2 get_transferred_cursor_pos() const noexcept{
			return viewport::get_transferred_pos(this->get_scene()->get_cursor_pos());
		}

	};
}
