//
// Created by Matrix on 2025/5/9.
//

export module mo_yanxi.game.aiming;

export import mo_yanxi.game.ecs.entity;
export import mo_yanxi.game.ecs.component.physical_property;

namespace mo_yanxi::game{
	struct target{
		ecs::entity_ref entity{};
		math::vec2 local_position{};

		explicit operator bool() const noexcept{
			return !entity.is_expired();
		}


	};
}
