module mo_yanxi.game.ui.grid_editor;
import mo_yanxi.ui.graphic;
import mo_yanxi.format;

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


	void grid_structural_panel::refresh(refresh_channel channel){
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

	void power_state_panel::bar_drawer::draw_content(const rect clipSpace) const{
		elem::draw_content(clipSpace);
		using namespace graphic;
		auto& r = renderer_from_erased(get_renderer());
		auto acquirer{mo_yanxi::ui::get_draw_acquirer(r)};

		auto& s = get_main_panel().get_viewport().grid.get_energy_status();
		auto max_count = std::max(s.max_consume(), s.max_generate());

		static constexpr auto columns = 16;
		const auto rows = (max_count + columns - 1) / columns;

		math::vec2 unit_size = (content_size() / math::vec2(columns, rows)).min_y(16);

		for(unsigned i = 0; i < max_count; ++i){
			const auto cur_col = i % columns;
			const auto cur_row = i / columns;
			auto cur_pos = unit_size.copy().mul(cur_col, cur_row) + content_src_pos();

			const bool is_consumed = i < s.max_consume();
			const bool is_generated = i < s.max_generate();

			const auto color = is_consumed && is_generated
				             ? colors::pale_green
				             : is_consumed
				             ? colors::red_dusted
				             : colors::pale_yellow;

			static constexpr float margin = 1;
			draw::fill::rect_ortho(acquirer.get(max_count), math::raw_frect{cur_pos, unit_size}.shrink({margin, margin}), color);
		}
	}

	void power_state_panel::refresh(refresh_channel channel){
		ui::elem_cast<label>(*children[1]).set_text(std::format("{}", get_viewport().grid.get_energy_status().max_generate()));
		ui::elem_cast<label>(*children[3]).set_text(std::format("{}", get_viewport().grid.get_energy_status().max_consume()));
	}

	void power_state_panel::build(){
		function_init([](label& label){
			label.set_style(theme::styles::side_bar_general);
			label.set_text("Generated:");
			label.set_scale(.5f);
			label.text_color_scl = label.property.graphic_data.style_color_scl = graphic::colors::pale_green;
		}).cell().set_external();

		function_init([](label& label){
			label.set_style();
			label.set_fit();
			label.text_color_scl = graphic::colors::light_gray;
		}).cell().set_size(40).set_pad({.post = 8});

		function_init([](label& label){
			label.set_style(theme::styles::side_bar_general);
			label.set_text("Consumed:");
			label.set_scale(.5f);
			label.text_color_scl = label.property.graphic_data.style_color_scl = graphic::colors::red_dusted;
		}).cell().set_external();

		function_init([](label& label){
			label.set_style();
			label.set_fit();
			label.text_color_scl = graphic::colors::light_gray;
		}).cell().set_size(40);

		create(creation::general_seperator_line{
			.stroke = 20,
			.palette = theme::style_pal::front_white.copy().mul_alpha(.25f)
		}).cell().set_pad({6, 6});

		emplace<bar_drawer>()->set_style();
	}

	struct text_entry_style{
		float name_label_size = 40;
		float content_label_size = 40;
		graphic::color color = graphic::colors::light_gray;
	};

	struct text_entry : ui::list{
		[[nodiscard]] text_entry(scene* scene, group* parent,
			std::string label_name = {},
			const text_entry_style& style = {})
			: list(scene, parent){

			property.graphic_data.style_color_scl = style.color;

			set_style(theme::styles::side_bar_general);

			function_init([&](label& l){
				l.set_style();
				l.set_fit(0.5f);
				l.set_text(std::move(label_name));
			}).cell().set_pad({.post = 4}).set_size(style.name_label_size);

			function_init([&](label& l){
				l.set_style();
				l.set_fit(0.5f);
			}).cell().set_size(style.content_label_size);
		}

		[[nodiscard]] label& content_label() const noexcept{
			return at<label>(1);
		}
	};

	void maneuvering_panel::refresh(refresh_channel channel){
		auto& grid = get_viewport().grid;

		if(channel == refresh_channel::all){
			subsystem = {};
			for (const auto & building : grid.get_buildings()){
				if(auto cmp = building.get_meta_info().get_maneuver_comp()){
					subsystem.append(*cmp, building.get_indexed_region().as<int>().move(grid.get_origin_offset()).scl(ecs::chamber::tile_size_integral).get_center().as<float>());
				}
			}
		}

		at<text_entry>(0).content_label().set_text(
			std::format("{0:1}#<[_><[#{2}>N #<[#><[/> | {1:.1f}#<[_><[#{2}>u/s#<[^>2",
			make_unipfx(subsystem.force_longitudinal()), subsystem.force_longitudinal() / grid.mass(),
			graphic::colors::pale_green));

		at<text_entry>(1).content_label().set_text(
			std::format("{0:1}#<[_><[#{2}>N #<[#><[/> | {1:.1f}#<[_><[#{2}>u/s#<[^>2",
			make_unipfx(subsystem.force_transverse())	, subsystem.force_transverse()   / grid.mass(),
			graphic::colors::pale_green));

		at<text_entry>(2).content_label().set_text(
			std::format("{0:1}#<[_><[#{2}>Nu#<[#><[/> | {1:.1f}#<[_><[#{2}>deg/s#<[^>2",
			make_unipfx(subsystem.torque())			, subsystem.torque() / grid.moment_of_inertia() * math::rad_to_deg,
			graphic::colors::pale_green));
	}

	void maneuvering_panel::build(){
		name = "manuv";
		template_cell.set_pad({4});
		template_cell.set_external();

		emplace<text_entry>(std::format("#<[#{}>Longitudinal", graphic::colors::aqua));
		emplace<text_entry>("Transverse");
		emplace<text_entry>("Torque");
	}
}
