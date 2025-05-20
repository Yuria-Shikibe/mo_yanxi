export module mo_yanxi.game.ecs.world.top;

export import mo_yanxi.game.ecs.component.manager;
export import mo_yanxi.game.ecs.system.collision;
export import mo_yanxi.game.world.graphic;

namespace mo_yanxi::game::world{
	export
	struct entity_top_world{
		ecs::component_manager component_manager{};
		graphic_context graphic_context{};

		ecs::system::collision_system collision_system{};

		[[nodiscard]] entity_top_world() = default;

		[[nodiscard]] explicit(false) entity_top_world(graphic::renderer_world& renderer)
			: graphic_context(renderer){
		}
	};
}
