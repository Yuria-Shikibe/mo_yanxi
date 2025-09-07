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


	template <typename T, std::size_t depth = 1>
		requires (depth > 0)
	struct nested_panel_base{
	protected:
		T& get_main_panel(this const auto& self) noexcept{
			auto p = static_cast<const elem&>(self).get_parent();
			for(auto i = 1; i < depth; ++i){
				p = p->get_parent();
			}

#if DEBUG_CHECK
			return dynamic_cast<T&>(*p);
#else
			return static_cast<T&>(*p);
#endif
		}
	};


	enum struct refresh_channel{
		all,
		indirect,
	};

	struct grid_editor_panel_base{
	protected:
		friend grid_detail_pane;
		~grid_editor_panel_base() = default;

		[[nodiscard]] grid_editor_viewport& get_viewport(this const auto& self) noexcept{
			return get(static_cast<const elem&>(self));
		}

		virtual void refresh(refresh_channel channel){

		}

	private:
		static grid_editor_viewport& get(const elem& self) noexcept;
	};

	struct grid_structural_panel : ui::list, grid_editor_panel_base{
	private:
		meta::chamber::grid_structural_statistic statistic{};

	public:
		struct entry : ui::table, nested_panel_base<grid_structural_panel, 3>{
			const meta::chamber::grid_building* identity{};

			[[nodiscard]] entry(scene* scene, group* group, const meta::chamber::grid_building* identity)
				: table(scene, group),
				  identity(identity){
				interactivity = interactivity::enabled;
				build();
			}

			void on_focus_changed(bool is_focused) override;

		private:
			void build();
		};


		[[nodiscard]] grid_structural_panel(scene* scene, group* parent)
			: list(scene, parent){
			build();
		}

		void refresh(refresh_channel channel) override;

	private:
		list& get_entries_list() const noexcept;
		void build();
	};

	struct power_state_panel : ui::list, grid_editor_panel_base{

	private:
		struct bar_drawer : ui::elem, nested_panel_base<power_state_panel>{
			using elem::elem;

			void draw_content(const rect clipSpace) const override;

		};

	public:
		[[nodiscard]] power_state_panel(scene* scene, group* parent)
			: list(scene, parent){
			build();
		}

		void refresh(refresh_channel channel) override;

	private:
		void build();

	};

	struct maneuvering_panel : ui::list, grid_editor_panel_base{
	private:
		ecs::chamber::maneuver_subsystem subsystem{};

	public:
		[[nodiscard]] maneuvering_panel(scene* scene, group* parent)
			: list(scene, parent){
			build();
		}

		void refresh(refresh_channel channel) override;

	private:

	private:
		void build();
	};
}