module;


export module mo_yanxi.game.ecs.system.renderer;
export import mo_yanxi.shared_stack;

import mo_yanxi.game.ecs.component.manage;
import mo_yanxi.game.ecs.component.drawer;
import mo_yanxi.game.ecs.component.manifold;
import mo_yanxi.game.world.graphic;
import std;

namespace mo_yanxi::game::ecs::system{
	export
	struct renderer{
		struct draw_data{
			const drawer::entity_drawer* drawer;
		};
		shared_stack<draw_data> drawers{};
		std::vector<draw_data> overflow_drawers{};

		void draw(const world::graphic_context& graphic_context) const {
			for (const auto & drawer : drawers.locked_range()){
				drawer.drawer->draw(graphic_context);
				drawer.drawer->post_effect(graphic_context);
			}

			for (const auto & drawer : overflow_drawers){
				drawer.drawer->draw(graphic_context);
				drawer.drawer->post_effect(graphic_context);
			}
		}

		void filter_screen_space(component_manager& manager, math::frect viewport){
			if(overflow_drawers.empty() && (drawers.capacity() - drawers.size()) > 256){
				drawers.reset();
			}

			drawers.clear_and_reserve(std::max<std::size_t>(128, drawers.size() + overflow_drawers.size()));
			overflow_drawers.clear();

			manager.sliced_each([&](const manifold& manifold, const drawer::entity_drawer& drawer){
				if(drawer.clip.value_or(manifold.hitbox.max_wrap_bound()).expand(drawer.clipspace_margin).overlap_inclusive(viewport)){
					if(auto ptr = drawers.try_push_uninitialized()){
						std::construct_at(ptr, &drawer);
						drawers.release_on_write();
					}else{
						overflow_drawers.push_back({&drawer});
					}
				}
			});

			(void)drawers.locked_range();
		}
	};
}
