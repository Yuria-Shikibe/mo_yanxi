module;

#include <cassert>
#include "../src/ext/adapted_attributes.hpp"

export module mo_yanxi.game.aiming;

export import mo_yanxi.game.ecs.entity;
export import mo_yanxi.game.ecs.component.physical_property;
export import mo_yanxi.math;
export import mo_yanxi.math.trans2;
export import mo_yanxi.array_stack;

import std;

namespace mo_yanxi::game{
	namespace vel_model{
		export struct constant{};

		export struct inf{};

		export struct geometric{
			float value;
		};
		export struct arithmetic{
			float value;
		};

		export struct analyser{
			std::variant<
				constant, inf//, geometric, arithmetic
			> type;

			float initial;
			float lifetime;
		};
	}

	struct trivial_range : math::range{
		explicit operator bool() const noexcept{
			return !std::isnan(from) && !std::isnan(to);
		}
	};

	struct trivial_optional_float{
		float value;
		bool valid;

		explicit(false) operator float() const noexcept{
			return value;
		}

		explicit operator bool() const noexcept{
			return valid;
		}
	};

	FORCE_INLINE trivial_optional_float resolve(const float a, const float b, const float c) noexcept{
		if(std::fabs(a) < std::numeric_limits<decltype(a)>::epsilon()){
			const auto val = -c / b;
			return {val, std::fabs(b) > std::numeric_limits<decltype(a)>::epsilon() && val >= 0};
		}else{
			const auto delta = b * b - 4 * a * c;
			if(delta < 0){
				return {{}, false};
			}

			const auto sqrt_delta = std::sqrt(delta);
			const auto t1 = (-b - sqrt_delta) / (2 * a);
			const auto t2 = (-b + sqrt_delta) / (2 * a);

			const auto [min, max] = math::minmax(t1, t2);
			if(min > 0){
				return {min, true};
			}

			if(max > 0){
				return {max, true};
			}

			return {{}, false};
		}
	}

	export
	FORCE_INLINE trivial_optional_float estimate_shoot_hit_delay(
		const math::vec2 shoot_pos,
		const math::vec2 target,
		const math::vec2 target_vel,
		const float bullet_speed) noexcept{

		const auto local = target - shoot_pos;

		const auto a = target_vel.length2() - bullet_speed * bullet_speed;
		const auto b = 2 * local.dot(target_vel);
		const auto c = local.length2();

		return resolve(a, b, c);
	}

	export
	FORCE_INLINE trivial_optional_float estimate_shoot_hit_delay(
		const math::vec2 shoot_pos,
		const math::vec2 target,
		const math::vec2 target_vel,
		const float bullet_speed,
		const float bullet_initial_displacement) noexcept{

		const auto local = target - shoot_pos;

		const auto a = target_vel.length2() - bullet_speed * bullet_speed;
		const auto b = 2 * (local.dot(target_vel) - bullet_speed * bullet_initial_displacement);
		const auto c = local.length2() - bullet_initial_displacement * bullet_initial_displacement;

		return resolve(a, b, c);
	}


	export
	struct target{
		ecs::entity_ref entity{};
		math::vec2 local_pos{};

		explicit operator bool() const noexcept{
			return !entity.is_expired();
		}

		template <typename T, typename C>
			requires std::convertible_to<T, math::trans2>
		math::vec2 operator|(T C::* p) const noexcept{
			assert(*this);
			return local_pos | entity.operator->*(p);
		}

		template <typename T, typename C>
			requires std::convertible_to<T, math::trans2>
		math::vec2 operator|(T (C::* p)()) const noexcept{
			assert(*this);
			return local_pos | entity.operator->*(p);
		}

		math::vec2 operator|(math::trans2 trs) const noexcept{
			assert(*this);
			return local_pos | trs;
		}
	};

	export
	struct traced_target : target{
	private:
		math::vec2 last{math::vectors::constant2<float>::SNaN};

	public:
		[[nodiscard]] std::optional<math::vec2> get_last() const noexcept{
			if(last.is_NaN()){
				return std::nullopt;
			}
			return std::optional{last};
		}

		template <typename T = math::trans2>
		[[nodiscard]] math::vec2 get_displacement(const T trs) const noexcept{
			return last.is_NaN() ? math::vec2{} : (this->operator|(trs) - last);
		}

		template <typename T = math::trans2>
		void update_last(const T trs) noexcept{
			last = this->operator|(trs);
		}
	};
}
