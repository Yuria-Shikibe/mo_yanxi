module;

#include "../src/ext/adapted_attributes.hpp"
#include <plf_hive.h>
export module mo_yanxi.game.meta.turret;

export import mo_yanxi.game.meta.projectile;

import mo_yanxi.math;
import std;

namespace mo_yanxi::game::meta::turret{
	export
	struct salvo_mode{
		unsigned count{};
		float spacing{};
		float angular_spacing{};
		float horizontal_spacing{};
	};

	export
	struct burst_mode{
		unsigned count{};
		float spacing{};
	};

	export
	struct shoot_offset{
		math::vec2 trunk{};
		std::array<math::optional_vec2<float>, 3> branch{[]{
			std::array<math::optional_vec2<float>, 3> buf{};
			buf.fill({math::vectors::constant2<float>::SNaN});
			return buf;
		}()};
	};

	export
	struct scaling{
		float velocity_scl{1};
		float lifetime_scl{1};
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
	};

	export
	struct turret_body{
		std::string_view name{};
		math::section<short> energy_section{};

		math::range range{};

		float rotational_inertia{};
		float max_rotate_speed{};
		float shoot_rotate_power_scale{1};

		float reload_duration{};
		unsigned max_reload_storage{};

		bool skip_first_salvo_reload{};
		bool actively_reload{};
		bool reserve_storage_if_power_off{};

		shoot_type shoot_type{};
	};
}