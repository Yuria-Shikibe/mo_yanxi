//
// Created by Matrix on 2025/5/9.
//

export module mo_yanxi.game.world.hud;

import mo_yanxi.ui.basic;
import mo_yanxi.ui.root;
import mo_yanxi.ui.manual_table;
import mo_yanxi.ui.table;

import mo_yanxi.game.ecs.component.manage;
import mo_yanxi.game.ecs.system.collision;
import mo_yanxi.game.world.graphic;

import std;

namespace mo_yanxi::game::world{
	export
	struct hud {
		struct hud_context{
			ecs::component_manager* component_manager{};
			ecs::system::collision_system* collision_system{};
			world::graphic_context* graphic_context{};
		};

		struct hud_viewport : ui::manual_table{
			hud* hud{};
			hud_context context{};

		private:
			ecs::entity_ref main_selected{};
			std::unordered_set<ecs::entity_ref> selected{};

			ui::table* side_bar{};

			void build_hud();

			void build_entity_info();
		public:
			[[nodiscard]] hud_viewport(ui::scene* scene, ui::group* group)
				: manual_table(scene, group){
				// skip_inbound_capture = true;
				interactivity = ui::interactivity::enabled;
				build_hud();
			}

			void draw_content(const ui::rect clipSpace) const override;

			ui::input_event::click_result on_click(const ui::input_event::click click_event) override;

			void update(float delta_in_ticks) override{
				manual_table::update(delta_in_ticks);
				if(!main_selected){
					main_selected = nullptr;
				}
			}
		};
	private:
		ui::scene* scene;
		hud_viewport* viewport;

	public:
		void bind_context(const hud_context& ctx){
			viewport->context = ctx;
		}

		[[nodiscard]] hud();

		~hud();

		void focus_hud();

		hud(const hud& other) = delete;
		hud(hud&& other) noexcept = delete;
		hud& operator=(const hud& other) = delete;
		hud& operator=(hud&& other) noexcept = delete;
	};
}
