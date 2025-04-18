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
		snap_shot<math::vec2> barProgress{};

		std::move_only_function<void(slider&)> callback{};

		[[nodiscard]] math::vec2 getBarMovableSize() const{
			return content_size() - bar_base_size;
		}

		[[nodiscard]] math::vec2 getSegmentUnit() const{
			return getBarMovableSize() / segments.copy().max({1, 1}).as<float>();
		}

		void moveBar(const math::vec2 baseMovement) noexcept{
			if(is_segment_move_activated()){
				barProgress.temp =
					(barProgress.base + (baseMovement * sensitivity).round_by(getSegmentUnit()) / getBarMovableSize()).clamp_xy({}, {1, 1});
			} else{
				barProgress.temp = (barProgress.base + (baseMovement * sensitivity) / getBarMovableSize()).clamp_xy({}, {1, 1});
			}
		}

		void applyLast(){
			if(barProgress.try_apply()){
				if(callback)callback(*this);
			}
		}

		void resumeLast(){
			barProgress.resume();
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

			events().on<events::click>([](const events::click& event, elem& e){
				auto& self = static_cast<slider&>(e);

				const auto [key, action, mode] = event.unpack();

				switch(action){
					case core::ctrl::act::press :{
						if(mode == core::ctrl::mode::Ctrl){
							const auto move =
								event.pos - (self.get_progress() * self.content_size() + self.content_src_pos());
							self.moveBar(move);
							self.applyLast();
						}
						break;
					}

					case core::ctrl::act::release :{
						self.applyLast();
					}

					default : break;
				}

			});

			register_event([](const events::scroll& event, slider& self){

				math::vec2 move = event.pos;

				if(event.mode & core::ctrl::mode::Shift){
					move.swap_xy();
					move.x *= -1;
				}

				if(self.is_clamped()){
					self.moveBar(self.scroll_sensitivity * self.sensitivity.to_sign() * move.y * math::vec2{-1, 1});
				} else{
					if(self.is_segment_move_activated()){
						move.to_sign().mul(self.getSegmentUnit());
					} else{
						move *= self.scroll_sensitivity;
					}
					self.moveBar(move);
				}

				self.applyLast();
			});

			register_event([](const events::drag& event, slider& self){
				self.moveBar(event.trans());
			});

			register_event([](const events::exbound& event, slider& self){
				self.set_focused_scroll(false);
			});

			register_event([](const events::inbound& event, slider& self){
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

		void set_initial_progress(math::vec2 progress) noexcept{
			progress.clamp_xy({}, math::vectors::constant2<float>::base_vec2);
			this->barProgress = progress;
		}

		[[nodiscard]] bool is_segment_move_activated() const noexcept{
			return static_cast<bool>(segments.x) || static_cast<bool>(segments.y);
		}

		[[nodiscard]] bool is_clamped() const noexcept{
			return is_clamped_to_hori() || is_clamped_to_vert();
		}

		[[nodiscard]] math::vec2 get_bar_last_pos() const noexcept{
			return getBarMovableSize() * barProgress.temp;
		}

		[[nodiscard]] math::vec2 get_bar_cur_pos() const noexcept{
			return getBarMovableSize() * barProgress.base;
		}

		[[nodiscard]] math::vec2 get_progress() const noexcept{
			return barProgress.base;
		}

		void set_hori_only() noexcept{ sensitivity.y = 0.0f; }

		[[nodiscard]] bool is_clamped_to_hori() const noexcept{ return sensitivity.y == 0.0f; }

		void set_vert_only() noexcept{ sensitivity.x = 0.0f; }

		[[nodiscard]] bool is_clamped_to_vert() const noexcept{ return sensitivity.x == 0.0f; }

	protected:
		void draw_content(rect clipSpace) const override;
	public:

		[[nodiscard]] constexpr math::vec2 get_bar_size() const noexcept{
			return {
				is_clamped_to_vert() ? property.content_width() : bar_base_size.x,
				is_clamped_to_hori() ? property.content_height() : bar_base_size.y,
			};
		}

	};
}
