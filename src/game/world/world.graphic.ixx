module;

#include <cassert>

export module mo_yanxi.game.world.graphic;

export import mo_yanxi.graphic.renderer.world;
export import mo_yanxi.game.graphic.effect.manager;

namespace mo_yanxi::game::world{
	export
	struct graphic_context{
	private:
		graphic::renderer_world* renderer_{};
		fx::effect_manager fx_manager{1024};

	public:
		[[nodiscard]] graphic_context() = default;

		[[nodiscard]] explicit(false) graphic_context(graphic::renderer_world& renderer)
			: renderer_(&renderer){
		}

		fx::effect& create_efx(){
			return fx_manager.obtain();
		}

		[[nodiscard]] math::frect viewport() const noexcept{
			assert(renderer_);
			return renderer_->camera.get_viewport();
		}

		[[nodiscard]] graphic::renderer_world& renderer() const noexcept{
			assert(renderer_);
			return *renderer_;
		}

		void update(const float delta_in_tick, bool paused){
			renderer().update(delta_in_tick);
			fx_manager.update(delta_in_tick * !paused);
		}

		void render_efx() const noexcept{
			fx_manager.each([ctx = fx::effect_draw_context{
				.renderer = renderer(),
				.viewport = renderer().camera.get_viewport()
			}](const fx::effect& e){
				e.draw(ctx);
			});
		}

		void clear_efx() noexcept{

		}
	};
}
