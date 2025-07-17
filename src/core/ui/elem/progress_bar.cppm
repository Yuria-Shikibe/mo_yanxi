//
// Created by Matrix on 2024/12/7.
//

export module mo_yanxi.ui.elem.progress_bar;


export import mo_yanxi.ui.primitives;
import mo_yanxi.core.ctrl.key_pack;

import mo_yanxi.snap_shot;
import mo_yanxi.graphic.color;
import mo_yanxi.math;

import std;

namespace mo_yanxi::ui{
	export struct progress_bar_color{
		graphic::color from{};
		graphic::color to{};

		[[nodiscard]] constexpr progress_bar_color() = default;

		[[nodiscard]] constexpr progress_bar_color(const graphic::color& from, const graphic::color& to)
			: from(from),
			  to(to){
		}

		[[nodiscard]] constexpr explicit(false) progress_bar_color(const graphic::color& color)
			: from(color), to(color){
		}

		constexpr void lerp(const progress_bar_color& other, float prog) noexcept{
			from.lerp(other.from, prog);
			to.lerp(other.to, prog);
		}

		constexpr void lerp(const graphic::color& other, float prog) noexcept{
			from.lerp(other, prog);
			to.lerp(other, prog);
		}
	};
	
	export struct progress_bar_progress{
		//TODO using a variant to support hori, vert, 2d...
		float current{};

		void approach(const progress_bar_progress other, float speed, float delta) noexcept{
			if(speed >= 1.f){
				this->operator=(other);
			}else{
				math::lerp_inplace(current, other.current, math::clamp(delta * speed));
			}
		}

		[[nodiscard]] constexpr math::vec2 get_size(math::vec2 boundSize) const noexcept{
			return {
				math::clamp(current) * boundSize.x,
				boundSize.y,
			};
		}
	};

	export struct progress_bar;

	export
	struct progress_bar_drawer{
		virtual ~progress_bar_drawer() = default;
		virtual void draw(const progress_bar& elem) const;

	protected:
		[[nodiscard]] static rect get_region(const progress_bar& elem) noexcept;
	};

	constexpr progress_bar_drawer DefaultProgressBarDrawer{};

	struct progress_bar : elem{
	protected:

		progress_bar_progress lastProgress{};
		progress_bar_color color{graphic::colors::light_gray};
		const progress_bar_drawer* drawer{&DefaultProgressBarDrawer};

	public:
		float approach_speed = 1.;

		[[nodiscard]] progress_bar(scene* scene, group* group)
			: elem(scene, group, "progress_bar"){
			interactivity = interactivity::disabled;
		}


		void set_initial_progress(const progress_bar_progress progress) noexcept{
			this->lastProgress = progress;
		}

		[[nodiscard]] const progress_bar_progress& get_progress() const noexcept{
			return lastProgress;
		}

		[[nodiscard]] const progress_bar_color& get_color() const noexcept{
			return color;
		}

	protected:
		void draw_content(const rect clipSpace) const override;

		void update(float delta_in_ticks) override{
			elem::update(delta_in_ticks);
		}

	public:
		void update_progress(progress_bar_progress prog, float delta_in_ticks){
			prog.current = math::clamp(prog.current);
			if(!std::isfinite(prog.current)){
				prog.current = 0;
			}
			lastProgress.approach(prog, approach_speed, delta_in_ticks);
		}

		void update_color(const progress_bar_color& color, float delta_in_ticks){
			if(approach_speed >= 1.f){
				this->color = color;
			}else{
				this->color.lerp(color, approach_speed * delta_in_ticks);
			}
		}

		void update_progress(float prog, float delta_in_ticks){
			this->update_progress(progress_bar_progress{prog}, delta_in_ticks);
		}

		[[nodiscard]] constexpr math::vec2 get_bar_size() const noexcept{
			return lastProgress.get_size(content_size());
		}
	};

	export
	struct dynamic_progress_bar : progress_bar{
	protected:

		std::move_only_function<progress_bar_progress(progress_bar&)> progressProv{};
		std::move_only_function<progress_bar_color(progress_bar&)> color_prov_{};

	public:

		[[nodiscard]] dynamic_progress_bar(scene* scene, group* group)
			: progress_bar(scene, group){
		}

		template <std::invocable<> Func>
			requires (std::is_invocable_r_v<progress_bar_progress, Func>)
		void set_progress_prov(Func&& func){
			progressProv = [p = std::forward<Func>(func)](progress_bar&){
				return p();
			};
		}

		template <std::invocable<progress_bar&> Func>
			requires (std::is_invocable_r_v<progress_bar_progress, Func>)
		void set_progress_prov(Func&& func){
			progressProv = std::forward<Func>(func);
		}

		void set_progress_prov(decltype(progressProv)&& func){
			progressProv = std::move(func);
		}

		template <std::invocable<> Func>
			requires (std::is_invocable_r_v<progress_bar_color, Func>)
		void set_color_prov(Func&& func){
			color_prov_ = [p = std::forward<Func>(func)](progress_bar&){
				return p();
			};
		}

		template <std::invocable<progress_bar&> Func>
			requires (std::is_invocable_r_v<progress_bar_color, Func>)
		void set_color_prov(Func&& func){
			color_prov_ = std::forward<Func>(func);
		}

		void set_color_prov(decltype(color_prov_)&& func){
			color_prov_ = std::move(func);
		}

	protected:
		void update(float delta_in_ticks) override{
			elem::update(delta_in_ticks);
			if(progressProv)update_progress(progressProv(*this), delta_in_ticks);
			if(color_prov_){
				update_color(color_prov_(*this), delta_in_ticks);
			}
		}
	};
}

