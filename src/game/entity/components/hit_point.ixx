

export module mo_yanxi.game.ecs.component.hit_point;

import mo_yanxi.math;
import std;

namespace mo_yanxi::game::ecs{
	export
	struct hit_point{
		float max{2000};
		float cur{2000};

		/**
		 * @brief [disabled, from(minimal functionality), to(maximum ~), max]
		 */
		math::range functionality{500, 1500};

	private:
		//float last{100};

	public:
		[[nodiscard]] float get_functionality_factor() const noexcept{
			return functionality.normalize(cur);
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

	};
}
