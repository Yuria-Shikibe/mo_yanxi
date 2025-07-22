//
// Created by Matrix on 2024/9/24.
//

export module mo_yanxi.ui.elem.slider;

export import mo_yanxi.ui.primitives;
import mo_yanxi.core.ctrl.key_pack;

import mo_yanxi.snap_shot;
import mo_yanxi.math;

import std;

namespace mo_yanxi::ui{
	export
	struct slider2d_slot{
	private:
		snap_shot<math::vec2> bar_progress_{};
	public:
		math::upoint2 segments{};

		[[nodiscard]] math::vec2 get_segment_unit() const noexcept{
			return math::vec2{segments.x ? 1.f / static_cast<float>(segments.x) : 1.f, segments.y ? 1.f / static_cast<float>(segments.y) : 1.f};
		}

		[[nodiscard]] bool is_segment_move_activated() const noexcept{
			return static_cast<bool>(segments.x) || static_cast<bool>(segments.y);
		}

		void move_progress(const math::vec2 movement_in_percent) noexcept{
			if(is_segment_move_activated()){
				bar_progress_.temp =
					(bar_progress_.base + movement_in_percent).round_to(get_segment_unit()).clamp_xy_normalized();
			} else{
				bar_progress_.temp = (bar_progress_.base + movement_in_percent).clamp_xy({}, {1, 1});
			}
		}

		void move_minimum_delta(const math::vec2 move){
			if(is_segment_move_activated()){
				move_progress(move.sign_or_zero().mul(get_segment_unit()));
			}else{
				move_progress(move);
			}
		}

		void clamp(math::vec2 from, math::vec2 to) noexcept{
			from.clamp_xy_normalized();
			to.clamp_xy_normalized();
			bar_progress_.temp.clamp_xy(from, to);
			bar_progress_.base.clamp_xy(from, to);
		}

		void min(math::vec2 min) noexcept{
			min.max({});
			bar_progress_.temp.min(min);
			bar_progress_.base.min(min);
		}

		void max(math::vec2 max) noexcept{
			max.min({1, 1});
			bar_progress_.temp.max(max);
			bar_progress_.base.max(max);
		}

		bool apply() noexcept{
			if(bar_progress_.try_apply()){
				return true;
			}
			return false;
		}

		void resume(){
			bar_progress_.resume();
		}

		void set_progress_from_segments(math::usize2 current, math::usize2 total){
			segments = total;
			set_progress((current.as<float>() / total.as<float>()).nan_to0());
		}

		void set_progress_from_segments_x(unsigned current, unsigned total){
			set_progress_from_segments({current, 0}, {total, 0});
		}

		void set_progress_from_segments_y(unsigned current, unsigned total){
			set_progress_from_segments({0, current}, {0, total});
		}

		void set_progress(math::vec2 progress) noexcept{
			progress.clamp_xy({}, math::vectors::constant2<float>::base_vec2);
			this->bar_progress_ = progress;
		}

		[[nodiscard]] math::vec2 get_progress() const noexcept{
			return bar_progress_.base;
		}


		template <typename T>
		[[nodiscard]] math::vector2<T> get_segments_progress() const noexcept{
			return (bar_progress_.base.as<double>() * segments.as<double>()).round<T>();
		}

		[[nodiscard]] math::vec2 get_temp_progress() const noexcept{
			return bar_progress_.temp;
		}

		[[nodiscard]] bool is_sliding() const noexcept{
			return bar_progress_.base != bar_progress_.temp;
		}
	};

	export
	struct raw_slider : elem{
	private:
		[[nodiscard]] math::vec2 getBarMovableSize() const{
			return content_size() - bar_base_size;
		}

	protected:
		void check_apply(){
			if(bar.apply()){
				on_data_changed();
			}
		}

	public:
		slider2d_slot bar;

		/**
		 * @brief Negative value is accepted to flip the operation
		 */
		math::vec2 scroll_sensitivity{6.0f, 3.0f};

		math::vec2 sensitivity{1.0f, 1.0f};

		math::vec2 bar_base_size{10.0f, 10.0f};

		[[nodiscard]] raw_slider(scene* scene, group* group)
			: elem(scene, group){
			property.maintain_focus_until_mouse_drop = true;
		}


		[[nodiscard]] bool is_clamped() const noexcept{
			return is_clamped_to_hori() || is_clamped_to_vert();
		}

		[[nodiscard]] math::vec2 get_progress() const noexcept{
			return bar.get_progress();
		}


		void set_hori_only() noexcept{
			sensitivity.y = 0.0f;
		}

		[[nodiscard]] bool is_clamped_to_hori() const noexcept{
			return sensitivity.y == 0.0f;
		}

		void set_vert_only() noexcept{
			sensitivity.x = 0.0f;
		}

		[[nodiscard]] bool is_clamped_to_vert() const noexcept{
			return sensitivity.x == 0.0f;
		}

	protected:
		void on_inbound_changed(bool is_inbounded, bool changed) override{
			elem::on_inbound_changed(is_inbounded, changed);
			set_focused_scroll(is_inbounded);
		}

		virtual void on_data_changed(){

		}

		void draw_bar(
			const slider2d_slot& slot,
			const graphic::color base = graphic::colors::gray,
			const graphic::color temp = graphic::colors::light_gray
			) const noexcept;

		void draw_content(rect clipSpace) const override;

		void on_scroll(const input_event::scroll event) override{
			move_bar(bar, event);

			check_apply();
		}

	public:


	protected:
		void move_bar(slider2d_slot& slot, const input_event::scroll& event) const{
			math::vec2 move = event.delta;

			if(event.mode & core::ctrl::mode::shift){
				move.swap_xy();
				move.x *= -1;
			}

			if(is_clamped()){
				slot.move_minimum_delta(scroll_sensitivity * sensitivity * move.y * math::vec2{-1, 1});
			} else{
				slot.move_progress(scroll_sensitivity * sensitivity * move);
			}
		}

		void on_drag(const input_event::drag event) override{
			bar.move_progress(event.trans() * sensitivity / content_size());
		}

		input_event::click_result on_click(const input_event::click click_event) override{
			elem::on_click(click_event);

			const auto [key, action, mode] = click_event.unpack();

			switch(action){
			case core::ctrl::act::press :{
				if(mode == core::ctrl::mode::ctrl){
					const auto move = (click_event.pos - (get_progress() * content_size() + content_src_pos())) / content_size();
					bar.move_progress(move * sensitivity.sign_or_zero());
					check_apply();
				}
				break;
			}

			case core::ctrl::act::release :{
				check_apply();
			}

			default : break;
			}

			return input_event::click_result::intercepted;
		}

	public:

		[[nodiscard]] constexpr math::vec2 get_bar_size() const noexcept{
			return {
				is_clamped_to_vert() ? property.content_width() : bar_base_size.x,
				is_clamped_to_hori() ? property.content_height() : bar_base_size.y,
			};
		}

	};

	export
	struct slider : raw_slider{
	private:
		std::move_only_function<void(slider&)> callback{};

	public:
		[[nodiscard]] slider(scene* scene, group* group)
			: raw_slider(scene, group){
		}

		template <typename Func>
		decltype(auto) set_callback(Func&& func){
			if constexpr (std::invocable<Func, math::usize2>){
				return std::exchange(callback, [func = std::forward<Func>(func)](const slider& slider){
					std::invoke(func, slider.bar.get_segments_progress<unsigned>());
				});
			}else if constexpr (std::invocable<Func, math::vec2>){
				return std::exchange(callback, [func = std::forward<Func>(func)](const slider& slider){
					std::invoke(func, slider.get_progress());
				});
			}else if constexpr (std::invocable<Func, slider&>){
				return std::exchange(callback, std::forward<Func>(func));
			}else if constexpr (std::invocable<Func>){
				return std::exchange(callback, [func = std::forward<Func>(func)](slider&){
					std::invoke(func);
				});
			}else{
				static_assert(false, "callback function type not support");
			}

		}

	protected:
		void on_data_changed() override{
			if(callback)callback(*this);
		}
	};
}
