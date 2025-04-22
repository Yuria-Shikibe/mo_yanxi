//
// Created by Matrix on 2025/4/21.
//

export module mo_yanxi.game.ecs.component.hit_point;

namespace mo_yanxi::game::ecs{
	export
	struct hit_point{
		float max{1000};
		float cur{1000};

	private:
		float last{100};

	public:

	};
}
