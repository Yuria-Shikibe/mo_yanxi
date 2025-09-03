

export module mo_yanxi.game.ecs.component.hit_point;

import mo_yanxi.math;
import std;

namespace mo_yanxi::game::ecs{
	export
	struct static_hit_point{
		float max{2000};

		/**
		 * @brief [disabled, from(minimal functionality), to(maximum ~), max]
		 */
		math::range capability_range{500, 1500};

	};
	export
	struct hit_point : static_hit_point{
		float cur{2000};



	private:
		//float last{100};

	public:
		void reset(const static_hit_point& static_hit_point) noexcept{
			this->static_hit_point::operator=(static_hit_point);
			reset();
		}

		void reset() noexcept{
			cur = max;
		}

		[[nodiscard]] float get_capability_factor() const noexcept{
			return capability_range.normalize(cur);
		}

		[[nodiscard]] float factor() const noexcept{
			return cur / max;
		}

	    void accept(float dmg) noexcept{
	    	cur = std::clamp<float>(cur - dmg, 0, max);
	    }

	    void heal(float heal) noexcept{
	    	cur = std::clamp<float>(cur + heal, 0, max);
	    }

		void cure() noexcept{
			cur = max;
		}

		float cure_and_get_healed() noexcept{
			auto delta = max - cur;
			cur = max;
			return delta;
		}

	};
}
