//
// Created by Matrix on 2025/7/23.
//

export module mo_yanxi.game.ecs.component.chamber:grid_maneuver;

namespace mo_yanxi::game::ecs::chamber{
	export struct chamber_manifold;

	export
	struct maneuver_component{
		float force_longitudinal;
		float force_transverse;

		float torque;
		/**
		 * @brief Torque always provide given value, no matter where the component is.
		 */
		float torque_absolute;
	};

	export
	struct maneuver_subsystem{
	private:
		float force_longitudinal_{};
		float force_transverse_{};
		float torque_{};
	public:
		[[nodiscard]] float force_longitudinal() const noexcept{
			return force_longitudinal_;
		}

		[[nodiscard]] float force_transverse() const noexcept{
			return force_transverse_;
		}

		[[nodiscard]] float torque() const noexcept{
			return torque_;
		}

		void update(const chamber_manifold& manifold) noexcept;
	};
}