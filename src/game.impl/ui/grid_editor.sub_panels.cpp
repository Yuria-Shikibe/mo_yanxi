module mo_yanxi.game.ui.grid_editor;
import mo_yanxi.ui.graphic;
import mo_yanxi.format;
import mo_yanxi.algo;

namespace mo_yanxi::game::meta::chamber{
	void ideal_path_finder_request::update(path_finder& finder){
		using node = path_node;

		if(state != path_find_request_state::running){
			return;
		}

		for(int i = 0; i < 8; ++i){
			if (!open_list.empty()) {
				node* current = open_list.top();
				open_list.pop();

				// 到达终点
				if (current->pos == dest) {
					// 回溯路径
					ideal_path.seq.reserve(iteration);
					node* temp = current;
					while (temp != nullptr) {
						ideal_path.seq.push_back(temp->pos.as<unsigned>());
						temp = temp->parent;
					}
					std::ranges::reverse(ideal_path.seq);
					state = path_find_request_state::finished;
					set_collapsed(finder);
					return;
				}

				current->is_optimal = true;

				static constexpr math::ivec2 mov[]{
					{ 1,  0},
					{ 0,  1},
					{-1,  0},
					{ 0, -1},
				};

				auto& grid = *this->target_grid;
				for (const auto m : mov) {
					auto next = current->pos + m;
					if(!next.within({}, grid.get_extent().as<int>()))continue;

					if (!grid[next.as<unsigned>()].is_accessible()) {
						continue;
					}

					const auto it = all_nodes.find(next);

					// 计算 tentative_g
					float tentative_g = current->cost + /*grid[next.as<unsigned>()].*/ + 1;

					if (it == all_nodes.end()) {
						float h_val = heuristic(next);
						auto& next_node = all_nodes[next] = node(next, tentative_g, h_val, current);;
						open_list.push(&next_node);
					} else{
						if(it->second.is_optimal){
							continue;
						}

						if (node* existing_node = &it->second; tentative_g < existing_node->cost) {
							existing_node->cost = tentative_g;
							existing_node->parent = current;

							open_list.push(existing_node);
						}
					}
				}


		++iteration;
			}else{
				state = path_find_request_state::failed;
				set_collapsed(finder);
				return;
			}
		}
	}

	void ideal_path_finder_request::set_collapsed(path_finder& finder) noexcept{
		algo::erase_unique_unstable(finder.pendings, this);
		is_collapsed.store(true);
		is_collapsed.notify_all();
	}

	ideal_path_finder_request* path_finder::acquire_path(const grid& grid, math::upoint2 initial, math::upoint2 dest){

		const auto& src = grid[initial];
		if(!src.is_accessible())return nullptr;
		const auto& dst = grid[initial];
		if(!dst.is_accessible())return nullptr;

		if(!grid.reachable_between(initial, dest)){
			return nullptr;
		}

		ideal_path_finder_request* request{};
		{
			semaphore_acq_guard acq_guard{requests_lock};
			request = std::addressof(*requests_.emplace(grid, initial, dest));
		}

		{
			semaphore_acq_guard acq_guard{pending_lock};
			pendings.push_back(request);
		}


		try_init_thread();

		return request;
	}

	bool path_finder::reacquire_path(ideal_path_finder_request& last_request, const grid& grid, math::upoint2 initial,
	                                 math::upoint2 dest){

		const auto& src = grid[initial];
		if(!src.is_accessible())return false;
		const auto& dst = grid[initial];
		if(!dst.is_accessible())return false;

		if(!grid.reachable_between(initial, dest)){
			return false;
		}

		{
			semaphore_acq_guard acq_guard{pending_lock};
			if(!std::ranges::contains(pendings, &last_request)){
				pendings.push_back(&last_request);
			}
			last_request.reset(grid, initial, dest);
		}

		return true;
	}
}

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
