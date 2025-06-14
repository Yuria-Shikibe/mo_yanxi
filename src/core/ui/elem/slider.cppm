//
// Created by Matrix on 2024/9/24.
//

export module mo_yanxi.ui.elem.slider;

export import mo_yanxi.ui.basic;
import mo_yanxi.core.ctrl.key_pack;

import mo_yanxi.snap_shot;
import mo_yanxi.math;

import std;

namespace mo_yanxi::ui{
	export 
	struct slider : elem{


	protected:
		/**
		 * @brief Has 2 degree of freedom [x, y]
		 */
		snap_shot<math::vec2> bar_progress_{};

		std::move_only_function<void(slider&)> callback{};

		[[nodiscard]] math::vec2 getBarMovableSize() const{
			return content_size() - bar_base_size;
		}

		[[nodiscard]] math::vec2 getSegmentUnit() const{
			return getBarMovableSize() / segments.copy().max({1, 1}).as<float>();
		}

		void moveBar(const math::vec2 baseMovement) noexcept{
			if(is_segment_move_activated()){
				bar_progress_.temp =
					(bar_progress_.base + (baseMovement * sensitivity).round_by(getSegmentUnit()) / getBarMovableSize()).clamp_xy({}, {1, 1});
			} else{
				bar_progress_.temp = (bar_progress_.base + (baseMovement * sensitivity) / getBarMovableSize()).clamp_xy({}, {1, 1});
			}
		}

		void applyLast(){
			if(bar_progress_.try_apply()){
				on_data_changed();
				if(callback)callback(*this);
			}
		}

		void resumeLast(){
			bar_progress_.resume();
		}

	public:
		/**
		 * @brief set 0 to disable one freedom degree
		 * Negative value is accepted to flip the operation
		 */
		math::vec2 sensitivity{1.0f, 1.0f};

		/**
		 * @brief Negative value is accepted to flip the operation
		 */
		math::vec2 scroll_sensitivity{6.0f, 3.0f};

		math::vec2 bar_base_size{10.0f, 10.0f};
		math::upoint2 segments{};

		[[nodiscard]] slider(scene* scene, group* group)
			: elem(scene, group, "Slider"){

			register_event([](const input_event::exbound& event, slider& self){
				self.set_focused_scroll(false);
			});

			register_event([](const input_event::inbound& event, slider& self){
				self.set_focused_scroll(true);
			});

			property.maintain_focus_until_mouse_drop = true;
		}

		template <typename Func>
		decltype(auto) setCallback(Func&& func){
			if constexpr (std::invocable<Func, math::usize2>){
				return std::exchange(callback, [func = std::forward<Func>(func)](const slider& slider){
					std::invoke(func, slider.segments.copy().scl_round(slider.get_progress()));
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

		void set_progress(math::vec2 progress) noexcept{
			progress.clamp_xy({}, math::vectors::constant2<float>::base_vec2);
			this->bar_progress_ = progress;
		}

		[[nodiscard]] bool is_segment_move_activated() const noexcept{
			return static_cast<bool>(segments.x) || static_cast<bool>(segments.y);
		}

		[[nodiscard]] bool is_clamped() const noexcept{
			return is_clamped_to_hori() || is_clamped_to_vert();
		}

		[[nodiscard]] math::vec2 get_bar_last_pos() const noexcept{
			return getBarMovableSize() * bar_progress_.temp;
		}

		[[nodiscard]] math::vec2 get_bar_cur_pos() const noexcept{
			return getBarMovableSize() * bar_progress_.base;
		}

		[[nodiscard]] math::vec2 get_progress() const noexcept{
			return bar_progress_.base;
		}

		void set_hori_only() noexcept{ sensitivity.y = 0.0f; }

		[[nodiscard]] bool is_clamped_to_hori() const noexcept{ return sensitivity.y == 0.0f; }

		void set_vert_only() noexcept{ sensitivity.x = 0.0f; }

		[[nodiscard]] bool is_clamped_to_vert() const noexcept{ return sensitivity.x == 0.0f; }

	protected:
		virtual void on_data_changed(){

		}

		void draw_content(rect clipSpace) const override;

		void on_scroll(const input_event::scroll event) override{

			math::vec2 move = event.delta;

			if(event.mode & core::ctrl::mode::shift){
				move.swap_xy();
				move.x *= -1;
			}

			if(is_clamped()){
				moveBar(scroll_sensitivity * sensitivity.to_sign() * move.y * math::vec2{-1, 1});
			} else{
				if(is_segment_move_activated()){
					move.to_sign().mul(getSegmentUnit());
				} else{
					move *= scroll_sensitivity;
				}
				moveBar(move);
			}

			applyLast();
		}

	public:
		bool is_sliding() const noexcept{
			return bar_progress_.base != bar_progress_.temp;
		}

	protected:
		void on_drag(const input_event::drag event) override{
			moveBar(event.trans());
		}

		input_event::click_result on_click(const input_event::click click_event) override{
			elem::on_click(click_event);

			const auto [key, action, mode] = click_event.unpack();

			switch(action){
			case core::ctrl::act::press :{
				if(mode == core::ctrl::mode::ctrl){
					const auto move =
						click_event.pos - (get_progress() * content_size() + content_src_pos());
					moveBar(move);
					applyLast();
				}
				break;
			}

			case core::ctrl::act::release :{
				applyLast();
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
}
