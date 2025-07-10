module;

#include "../src/ext/adapted_attributes.hpp"
#include <plf_hive.h>
export module mo_yanxi.game.meta.turret;

export import mo_yanxi.game.meta.projectile;

import mo_yanxi.math;
import mo_yanxi.array_stack;
import std;

namespace mo_yanxi::game::meta::turret{
	export
	struct salvo_mode{
		unsigned count{1};
		float spacing{};

		// [[nodiscard]] float duration() const noexcept{
		// 	return count * spacing;
		// }
	};

	export
	struct burst_mode{
		unsigned count{1};
		float spacing{};

		float angular_spacing{};
		float horizontal_spacing{};

		[[nodiscard]] float duration() const noexcept{
			return count * spacing;
		}
	};

	export
	struct shoot_offset{
		math::vec2 trunk{};
		std::array<math::trans2, 3> branch{};
		unsigned count{1};

		FORCE_INLINE constexpr math::trans2 operator[](unsigned idx) const noexcept{
			assert(idx < count);
			return math::trans2{branch[idx]} += trunk;
		}
	};

	export
	struct scaling{
		float velocity_scl{1};
		float lifetime_scl{1};
	};

	export
	struct shoot_index{
		unsigned salvo_index;
		unsigned burst_index;
	};

	export
	struct shoot_type{
		projectile* projectile{};

		shoot_offset offset{};


		float angular_inaccuracy{};
		scaling inaccuracy{};
		scaling scaling{};


		burst_mode burst{};
		salvo_mode salvo{};

		[[nodiscard]] unsigned get_total_shoots() const noexcept{
			return burst.count * salvo.count;
		}


		[[nodiscard]] float get_shoot_duration() const noexcept{
			// const auto d1 = salvo.duration();
			const auto d2 = salvo.count * burst.duration();

			return (d2 > 0 ? d2 : 1);
		}


		[[nodiscard]] shoot_index get_current_shoot_index(float duration) const noexcept{
			duration += salvo.spacing;
			float unitDuration = salvo.spacing + burst.duration();
			shoot_index idx;
			idx.salvo_index = static_cast<unsigned>(duration / unitDuration);

			auto rem = std::fdim(math::mod(duration, unitDuration), salvo.spacing);
			idx.burst_index = rem == 0 ? 0 : burst.spacing == 0 ? burst.count : static_cast<unsigned>(rem / burst.spacing);
			return idx;
		}

		[[nodiscard]] shoot_index get_shoot_index(unsigned total_shoot_index) const noexcept{
			return {total_shoot_index / burst.count, total_shoot_index % burst.count};
		}

		math::trans2 operator[](shoot_index idx, const unsigned barrelIdx) const noexcept{
			auto t0 = offset[barrelIdx];
			t0.rot += idx.burst_index * burst.angular_spacing - (burst.count * burst.angular_spacing / 2);
			t0.vec.y += idx.burst_index * burst.horizontal_spacing - (burst.count * burst.horizontal_spacing / 2);
			return t0;
		}
	};

	export
	struct turret_body{
		std::string_view name{};
		math::section<short> energy_section{};

		math::range range{0, 2000};

		float rotational_inertia{50};
		float max_rotate_speed{std::numbers::pi / 60. / 2.f};
		float shoot_rotate_power_scale{1};

		float reload_duration{90};
		// unsigned max_reload_storage{};

		// bool skip_first_salvo_reload{};
		bool actively_reload{};
		// bool reserve_storage_if_power_off{};
		// bool reload_while_shooting{};

		shoot_type shoot_type{};
	};
}