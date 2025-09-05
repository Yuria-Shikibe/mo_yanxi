module mo_yanxi.game.ui.grid_editor;
import mo_yanxi.ui.graphic;

namespace mo_yanxi::game::ui{
	grid_editor_viewport& grid_editor_panel_base::get(const elem& self) noexcept{
		//self -> maunal_table -> viewport
		return *self.get_parent()->get_parent<grid_editor_viewport, DEBUG_CHECK>();
	}

	void grid_structural_panel::entry::on_focus_changed(bool is_focused){
		if(is_focused){
			get_main_panel().get_viewport().subpanel_drawer = [this](const grid_editor_viewport& viewport){
				auto& clusters = get_main_panel().statistic[identity];
				using namespace graphic;
				auto& r = renderer_from_erased(get_renderer());
				auto acquirer{mo_yanxi::ui::get_draw_acquirer(r)};


				for (const auto & building : clusters.buildings){
					draw::fill::rect_ortho(acquirer.get(
						clusters.buildings.size()),
						viewport.get_region_at(building->get_indexed_region()), colors::white.copy().set_a(.4));
				}
			};
		}else{
			get_main_panel().get_viewport().subpanel_drawer = nullptr;
		}

		table::on_focus_changed(is_focused);
	}

	void grid_structural_panel::entry::build(){
		set_style(ui::theme::styles::side_bar_whisper);
		function_init([this](ui::label& label){
			label.set_fit();
			label.set_style();
			auto& clusters = get_main_panel().statistic[identity];
			label.set_text(std::format("Count: {}\n HP: {:.0f}", clusters.size(), clusters.get_total_hitpoint()));
		});
	}


	void grid_structural_panel::refresh(){
		statistic = meta::chamber::grid_structural_statistic{get_viewport().grid};
		static_cast<label&>(*children[0]).set_text(std::format("HP: {:.0f}", statistic.get_structural_hitpoint()));
		auto& l = get_entries_list();
		l.clear_children();

		for (const auto& cluster : statistic.get_clusters()){
			auto hdl = l.emplace<entry>(cluster.identity());
		}
	}

	list& grid_structural_panel::get_entries_list() const noexcept{
		return static_cast<scroll_pane&>(*children[1]).get_item_unchecked<list>();
	}

	void grid_structural_panel::build(){
		function_init([](ui::label& label){
			// label.set_style();
			label.set_fit();
		}).cell().set_size(50).set_pad({.post = 4});

		auto pane = emplace<scroll_pane>();
		pane->set_style();
		pane->function_init([](ui::list& list){
			list.set_style();
			list.template_cell.set_pad({.post = 4});
			list.template_cell.set_size(80);
		});
	}
}
