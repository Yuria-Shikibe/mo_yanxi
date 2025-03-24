//
// Created by Matrix on 2024/12/7.
//

export module mo_yanxi.ui.elem.progress_bar;


export import mo_yanxi.ui.elem;
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

	struct progress_bar_drawer{
		virtual ~progress_bar_drawer() = default;
		virtual void draw(const progress_bar& elem) const;

	protected:
		[[nodiscard]] static rect get_region(const progress_bar& elem) noexcept;
	};

	constexpr progress_bar_drawer DefaultProgressBarDrawer{};

	export
	struct progress_bar : elem{
	protected:
		using ColorToProv = progress_bar_color;

		progress_bar_progress lastProgress{};
		ColorToProv color{};
		std::move_only_function<progress_bar_progress(progress_bar&)> progressProv{};

		std::move_only_function<ColorToProv(progress_bar&)> color_prov_{};
		const progress_bar_drawer* drawer{&DefaultProgressBarDrawer};

	public:
		float reach_speed = 1.;

		[[nodiscard]] progress_bar(scene* scene, group* group)
			: elem(scene, group, "progress_bar"){
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
			requires (std::is_invocable_r_v<ColorToProv, Func>)
		void set_color_prov(Func&& func){
			color_prov_ = [p = std::forward<Func>(func)](progress_bar&){
				return p();
			};
		}

		template <std::invocable<progress_bar&> Func>
			requires (std::is_invocable_r_v<ColorToProv, Func>)
		void set_color_prov(Func&& func){
			color_prov_ = std::forward<Func>(func);
		}

		void set_color_prov(decltype(color_prov_)&& func){
			color_prov_ = std::move(func);
		}

		void set_initial_progress(const progress_bar_progress progress) noexcept{
			this->lastProgress = progress;
		}

		[[nodiscard]] const progress_bar_progress& get_progress() const noexcept{
			return lastProgress;
		}

		[[nodiscard]] const ColorToProv& getColor() const noexcept{
			return color;
		}

	protected:
		void draw_content(const rect clipSpace) const override;

		void update(float delta_in_ticks) override{
			elem::update(delta_in_ticks);
			if(progressProv)lastProgress.approach(progressProv(*this), reach_speed, delta_in_ticks);
			if(color_prov_){
				auto next = color_prov_(*this);
				if(reach_speed >= 1.f){
					color = next;
				}else{
					color.lerp(color_prov_(*this), reach_speed * delta_in_ticks);
				}
			}else{
				color = graphic::colors::light_gray;
			}
		}

	public:
		[[nodiscard]] constexpr math::vec2 getBarSize() const noexcept{
			return lastProgress.get_size(content_size());
		}

	};
}

