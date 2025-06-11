//
// Created by Matrix on 2025/5/26.
//

export module mo_yanxi.game.ui.viewport;

export import mo_yanxi.ui.basic;
export import mo_yanxi.graphic.camera;

namespace mo_yanxi::game::ui{
	export using namespace mo_yanxi::ui;

	export constexpr float viewport_default_radius = 5000;

	export
	struct viewport : elem{
	private:
		math::vec2 last_camera_{};
	protected:
		graphic::camera2 camera{};
		math::vec2 viewport_region{viewport_default_radius * 2, viewport_default_radius * 2};

	public:
		[[nodiscard]] viewport(scene* scene, group* group, const std::string_view tyName)
			: elem(scene, group, tyName){

			camera.speed_scale = 0;
		}

	protected:
		void update(const float delta_in_ticks) override{
			elem::update(delta_in_ticks);
			const auto [w, h]{viewport_region - camera.get_viewport().size()};
			camera.clamp_position({math::vec2{}, w, h});
			camera.update(delta_in_ticks);

		}

		bool resize(const math::vec2 size) override{
			if(elem::resize(size)){
				auto [x, y] = content_size();
				camera.resize_screen(x, y);
				return true;
			}

			return false;
		}

		void on_focus_changed(bool is_focused) override{
			get_scene()->set_camera_focus(is_focused ? &camera : nullptr);
			set_focused_scroll(is_focused);
			set_focused_key(is_focused);
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
			}

			return ui::input_event::click_result::intercepted;
		}

		void viewport_begin() const {
			const auto proj = camera.get_world_to_uniformed();

			get_renderer().batch.push_projection(proj);
			get_renderer().batch.push_viewport(prop().content_bound_absolute());
			get_renderer().batch.push_scissor({camera.get_viewport()});
		}

		void viewport_end() const {
			get_renderer().batch.pop_scissor();
			get_renderer().batch.pop_viewport();
			get_renderer().batch.pop_projection();
		}

		[[nodiscard]] math::vec2 get_transferred_pos(const math::vec2 pos) const noexcept{
			return camera.get_screen_to_world(pos, content_src_pos(), true);
		}

		[[nodiscard]] math::vec2 get_transferred_cursor_pos() const noexcept{
			return get_transferred_pos(get_scene()->get_cursor_pos());
		}

	};
}
