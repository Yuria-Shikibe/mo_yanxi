//
// Created by Matrix on 2025/9/5.
//

export module mo_yanxi.game.ui.grid_editor:sub_panels;

import mo_yanxi.ui.elem.list;
import mo_yanxi.game.ecs.component.chamber;
import std;

namespace mo_yanxi::game::ui{
	using namespace mo_yanxi::ui;
	export struct grid_detail_pane;
	export struct grid_editor_viewport;


	struct grid_editor_panel_base{
	protected:
		friend grid_detail_pane;
		~grid_editor_panel_base() = default;

		[[nodiscard]] grid_editor_viewport& get_viewport(this const auto& self) noexcept{
			return get(static_cast<const elem&>(self));
		}

		virtual void refresh(){

		}

	private:
		static grid_editor_viewport& get(const elem& self) noexcept;
	};

	struct grid_structural_panel : ui::list, grid_editor_panel_base{
	private:
		meta::chamber::grid_structural_statistic statistic{};

	public:
		struct entry : ui::table{
			const meta::chamber::grid_building* identity{};

			[[nodiscard]] entry(scene* scene, group* group, const meta::chamber::grid_building* identity)
				: table(scene, group),
				  identity(identity){
				interactivity = interactivity::enabled;
				build();
			}

			void on_focus_changed(bool is_focused) override;

		private:
			[[nodiscard]] grid_structural_panel& get_main_panel() const noexcept{
				return *get_parent()->get_parent()->get_parent<grid_structural_panel, DEBUG_CHECK>();
			}

			void build();
		};


		[[nodiscard]] grid_structural_panel(scene* scene, group* parent)
			: list(scene, parent){
			build();
		}

		void refresh() override;

	private:
		list& get_entries_list() const noexcept;
		void build();
	};
}