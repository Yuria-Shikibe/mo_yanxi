//
// Created by Matrix on 2025/5/20.
//

export module mo_yanxi.game.ui.bars;

import mo_yanxi.ui.elem.progress_bar;
import mo_yanxi.math;
import mo_yanxi.graphic.renderer.predecl;
import std;

namespace mo_yanxi::game::ui{
	export using namespace mo_yanxi::ui;

	template <std::input_iterator It, std::sentinel_for<It> Se,
		std::invocable<std::iter_difference_t<It>, float, float> LConsumer,
		std::invocable<std::iter_difference_t<It>, float, float> RConsumer>
	requires (std::convertible_to<std::iter_const_reference_t<It>, float>)
	void get_chunked_progress_region(It it, Se se, float cur, LConsumer lc, RConsumer rc){
		float last{};
		std::iter_difference_t<It> idx{};

		auto consume = [&](std::iter_difference_t<It> index, float next){
			if(cur > next){
				//(next, 1]
				lc(index, last, next);
			}else if(cur >= last){
				//[last, next)
				lc(index, last, cur);
				rc(index, cur, next);
			}else{
				rc(index, last, next);
				//[0, last)
			}
		};

		while(it != se){
			float next = *it;
			consume(idx, next);
			last = next;
			++it;
			++idx;
		}

		consume(idx, 1.f);
	}

	template <std::ranges::input_range Rng = std::initializer_list<float>,
		std::invocable<std::ranges::range_difference_t<Rng>, float, float> LConsumer,
		std::invocable<std::ranges::range_difference_t<Rng>, float, float> RConsumer>
	requires (std::convertible_to<std::ranges::range_const_reference_t<Rng>, float>)
	void get_chunked_progress_region(Rng&& rng, float cur, LConsumer lc, RConsumer rc){
		ui::get_chunked_progress_region(std::ranges::begin(std::as_const(rng)), std::ranges::end(std::as_const(rng)), cur, std::move(lc), std::move(rc));
	}

	struct basic_bar{
		void update(float delta_in_ticks) = delete;
		void draw(math::frect region, float opacity, graphic::renderer_ui_ref renderer_ref) const = delete;
	};

	export
	struct stalled_hp_bar_drawer : basic_bar{
	private:
		static constexpr float stall_time = 45;
		struct stall{
			float value;
			float time;
		};

		std::optional<stall> stall{};
		float current_value{};
		float current_target_value{};

	public:
		math::range valid_range{};
		float approach_speed{0.105f};

		void update(float delta_in_ticks) noexcept{
			if(stall){
				if(math::equal(stall->value, current_target_value, 0.005f)){
					stall.reset();
				}else{
					stall->time -= delta_in_ticks;
					if(stall->time < 0){
						stall->value = std::max(stall->value, current_value);
						math::lerp_inplace(stall->value, current_target_value, delta_in_ticks * approach_speed);
					}
				}
			}

			math::lerp_inplace(current_value, current_target_value, delta_in_ticks * approach_speed);
		}

		void draw(math::frect region, float opacity, graphic::renderer_ui_ref renderer_ref) const;


		void set_value(float value){
			if(!math::equal(value, current_target_value, 0.005f)){
				if(!stall){
					stall.emplace(current_value, stall_time);
				}else{
					stall->time = stall_time;
				}
			}

			current_target_value = value;
		}
	};

	export
	struct stalled_bar : mo_yanxi::ui::elem{
	private:
		static constexpr float stall_time = 45;
		struct stall{
			float value;
			float time;
		};

		std::optional<stall> stall{};
		float current_value{};
		float current_target_value{};

	public:
		math::range normalized_valid_range{};
		float approach_speed{0.105f};

		[[nodiscard]] stalled_bar(mo_yanxi::ui::scene* scene, mo_yanxi::ui::group* group)
			: elem(scene, group){
			interactivity = interactivity::disabled;
		}

	protected:
		void update(float delta_in_ticks) override{
			elem::update(delta_in_ticks);

			if(stall){
				if(math::equal(stall->value, current_target_value, 0.005f)){
					stall.reset();
				}else{
					stall->time -= delta_in_ticks;
					if(stall->time < 0){
						stall->value = std::max(stall->value, current_value);
						math::lerp_inplace(stall->value, current_target_value, delta_in_ticks * approach_speed);
					}
				}
			}

			math::lerp_inplace(current_value, current_target_value, delta_in_ticks * approach_speed);
		}

		void draw_content(mo_yanxi::ui::rect clipSpace) const override;
	public:
		void set_value(float value){
			if(!math::equal(value, current_target_value, 0.005f)){
				if(!stall){
					stall.emplace(current_value, stall_time);
				}else{
					stall->time = stall_time;
				}
			}

			current_target_value = value;
		}

	};

	export
	struct reload_bar_drawer : basic_bar{
	protected:
		float current_efficiency{};

	public:
		float approach_speed{0.105f};

		float current_reload_value{};
		float current_target_efficiency{};

		float efficiency_bar_scale = .25f;
		math::section<graphic::color> reload_color{graphic::colors::dark_gray.to_neutralize_light(), graphic::colors::gray.to_neutralize_light()};
		math::section<graphic::color> efficiency_color{graphic::colors::power.to_neutralize_light(), graphic::colors::power.to_neutralize_light()};

		void update(float delta_in_ticks) noexcept{
			math::lerp_inplace(current_efficiency, current_target_efficiency, delta_in_ticks * approach_speed);
		}

		void draw(math::frect region, float opacity, graphic::renderer_ui_ref renderer_ref) const;

	};

	export
	struct reload_bar : mo_yanxi::ui::elem{
	protected:
		float current_efficiency{};

	public:
		float approach_speed{0.105f};

		float current_reload_value{};
		float current_target_efficiency{};

		float efficiency_bar_scale = .25f;
		math::section<graphic::color> reload_color{};
		math::section<graphic::color> efficiency_color{};

		[[nodiscard]] reload_bar(mo_yanxi::ui::scene* scene, mo_yanxi::ui::group* group)
			: elem(scene, group){
			interactivity = interactivity::disabled;
		}
	protected:
		void update(float delta_in_ticks) override{
			elem::update(delta_in_ticks);

			// math::lerp_inplace(current_value, current_target_value, delta_in_ticks * approach_speed);
			math::lerp_inplace(current_efficiency, current_target_efficiency, delta_in_ticks * approach_speed);
		}

		void draw_content(mo_yanxi::ui::rect clipSpace) const override;
	};

	export
	struct energy_bar_state{
		int power_current;
		int power_total;
		unsigned power_minimal_expected;
		unsigned power_expected;
		unsigned power_assigned;
		unsigned power_valid;

		float charge;
		float charge_duration;
	};

	export
	struct energy_bar_drawer : basic_bar{
	private:
		energy_bar_state state_{};

		float expected_count_smooth_{};
		float expected_minimum_count_smooth_{};

	public:
		[[nodiscard]] float get_target_charge_progress() const noexcept{
			return state_.charge / state_.charge_duration;
		}

		explicit operator bool() const noexcept{
			return state_.power_total != 0;
		}

		void set_bar_state(const energy_bar_state& state) noexcept {
			this->state_ = state;
		}

		void update(float delta_in_ticks) noexcept{
			math::lerp_inplace<float>(expected_count_smooth_, state_.power_expected, delta_in_ticks * 0.075f);
			math::lerp_inplace<float>(expected_minimum_count_smooth_, state_.power_minimal_expected, delta_in_ticks * 0.075f);

		}

		void draw(math::frect region, float opacity, graphic::renderer_ui_ref renderer_ref) const;
	};

	export
	struct energy_bar : elem{
		energy_bar_drawer drawer{};

		[[nodiscard]] energy_bar(scene* scene, group* group)
			: elem(scene, group){
		}

		void update(float delta_in_ticks) override{
			elem::update(delta_in_ticks);
		}

		void draw_content(const rect clipSpace) const override{
			draw_background();
			drawer.draw(this->prop().content_bound_absolute(), gprop().get_opacity(), get_renderer());
		}
	};
}