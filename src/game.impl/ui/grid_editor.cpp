module;

#include "srl/srl.game.hpp"
#include "srl/srl.hpp"

module mo_yanxi.game.ui.grid_editor;

import mo_yanxi.ui.graphic;
import mo_yanxi.graphic.draw.multi_region;
import mo_yanxi.graphic.layers.ui.grid_drawer;


import mo_yanxi.game.meta.grid.srl;

import mo_yanxi.game.content;

import std;
import mo_yanxi.game.meta.hitbox;

mo_yanxi::game::meta::hitbox_transed load_hitbox_from(const std::filesystem::path& path){
	using namespace mo_yanxi;
	using namespace game;
	meta::hitbox_transed meta{};
	std::ifstream stream(path, std::ios::in | std::ios::binary);
	io::loader<meta::hitbox_transed>::parse_from(stream, meta);

	return meta;
}

void mo_yanxi::game::ui::grid_editor_viewport::grid_info_bar::draw_content(const ui::rect clipSpace) const{
	elem::draw_content(clipSpace);

	auto& edit = get_editor();

	auto max_consume = edit.grid.get_energy_status().max_consume();
	auto max_generate = edit.grid.get_energy_status().max_generate();

	auto max_count = std::max(max_consume, max_generate);
	if(max_count == 0)return;

	const auto src = content_src_pos();
	const auto extent = content_size();
	const auto width = extent.x / 2.f;
	const auto height = std::min<float>(extent.y / max_count, 24);

	using namespace graphic;
	auto acquirer{mo_yanxi::ui::get_draw_acquirer(get_renderer())};


	for(unsigned i = 0; i < max_generate; ++i){
		draw::fill::rect_ortho(acquirer.get(max_generate), rect{tags::from_extent, src.copy().add_y(i * height), width, height}.shrink(2), colors::power);
	}

	for(unsigned i = 0; i < max_consume; ++i){
		draw::fill::rect_ortho(acquirer.get(max_consume), rect{tags::from_extent, src.copy().add(width, i * height), width, height}.shrink(2), colors::red_dusted);
	}


}


void mo_yanxi::game::ui::grid_editor_viewport::draw_content(const rect clipSpace) const{
	viewport_begin();
	using namespace graphic;
	auto& r = renderer_from_erased(get_renderer());
	auto acquirer{mo_yanxi::ui::get_draw_acquirer(r)};


	{
		batch_layer_guard guard(r.batch, std::in_place_type<layers::grid_drawer>);

		draw::fill::rect_ortho(acquirer.get(), camera.get_viewport(), colors::black.copy().set_a(.35));
	}

	acquirer.proj.set_layer(ui::draw_layers::base);

	for(const auto& comp : reference){
		draw::line::quad_expanded(acquirer, comp.crop().view_as_quad(), -4, colors::light_gray.copy().set_a(.35));
	}

	auto placement_drawer = [this, &acquirer](math::usize2 chamber_extent, math::raw_frect region, auto pred){
		math::irect place_region = get_selected_place_region(chamber_extent, region);
		draw::line::rect_ortho(acquirer, place_region.as<float>().scl(ecs::chamber::tile_size, ecs::chamber::tile_size), 4, colors::pale_green);

		auto chamber_sz = chamber_extent.as<int>();

		acquirer.proj.mode_flag = draw::mode_flags::slide_line;
		game::each_tile(place_region, chamber_sz, [&, this](math::point2 pos){
			if(auto idxp = grid.coord_to_index(pos)){
				if(pred(*idxp)){
					draw::fill::rect_ortho(acquirer.get(), get_region_at({tags::from_extent, pos, chamber_sz}).shrink(4), colors::pale_green.copy().set_a(.5f));
				}
			}
		});
		acquirer.proj.mode_flag = {};
	};

	auto remove_drawer = [this, &acquirer](math::raw_frect region, auto pred){
		math::irect place_region = get_selected_place_region({1, 1}, region);
		draw::line::rect_ortho(acquirer, place_region.as<float>().scl(ecs::chamber::tile_size, ecs::chamber::tile_size), 4, colors::red_dusted);

		acquirer.proj.mode_flag = draw::mode_flags::slide_line;
		game::each_tile(place_region, math::isize2{1, 1}, [&, this](math::point2 pos){
			if(auto idxp = grid.coord_to_index(pos)){
				if(pred(*idxp)){
					draw::fill::rect_ortho(acquirer.get(), get_region_at({pos, 1, 1}).shrink(4), colors::red_dusted.copy().set_a(.5f));
				}
			}
		});
		acquirer.proj.mode_flag = {};
	};

	if(focus_on_grid){
		if(grid_op_ == operation::move){
			grid.draw(r, camera, {}, .5f);
			grid.draw(r, camera, get_grid_edit_offset(get_scene()->get_cursor_pos()));
		}else{
			grid.draw(r, camera);

			if (get_scene()->get_input_mode() == 0){
				auto currentCoord = get_transferred_cursor_pos().div(ecs::chamber::tile_size).floor().as<int>();
				math::irect to_place_region{tags::from_extent, currentCoord, 1, 1};
				auto world_region = get_region_at(to_place_region);

				if(auto idxp = grid.coord_to_index(currentCoord)){
					draw::line::rect_ortho(acquirer, world_region, 4, colors::pale_green.copy().set_a(.5f));
				}
			}

			if(last_click_ && get_scene()->get_input_mode() & core::ctrl::mode::ctrl_shift){
				math::rect_ortho_trivial rect{get_select_box(last_click_)};

				if(get_scene()->is_mouse_pressed(core::ctrl::mouse::LMB)){
					placement_drawer({1, 1}, rect, [this](math::upoint2 idx){
						return !grid[idx].placeable;
					});
				}else if(get_scene()->is_mouse_pressed(core::ctrl::mouse::RMB)){
					remove_drawer(rect, [this](math::upoint2 idx){
						return grid[idx].placeable && grid[idx].building == nullptr;
					});
				}
			}
		}
	}else{
		grid.draw(r, camera);

		acquirer.proj.mode_flag = {};
		acquirer.proj.set_layer(ui::draw_layers::def);

		if (current_chamber && get_scene()->get_input_mode() == 0){
			auto extent = current_chamber->extent.as<int>();
			auto currentCoord = get_current_tile_coord();
			math::irect to_place_region{tags::from_extent, currentCoord, extent};
			auto world_region = get_region_at(to_place_region);

			if(auto idxp = grid.coord_to_index(currentCoord); idxp && grid.is_building_placeable_at(*idxp, *current_chamber)){
				draw::line::rect_ortho(acquirer, world_region, 4, colors::pale_green.copy().set_a(.5f));
			}else{
				draw::line::rect_ortho(acquirer, world_region, 4, colors::red_dusted.copy().set_a(.5f));
			}
		}

		if(last_click_ && get_scene()->get_input_mode() & core::ctrl::mode::ctrl_shift){
			math::rect_ortho_trivial rect{get_select_box(last_click_)};

			if(current_chamber && get_scene()->is_mouse_pressed(core::ctrl::mouse::LMB)){
				placement_drawer(current_chamber->extent, rect, [this](math::upoint2 idx){
					return grid.is_building_placeable_at(idx, *current_chamber);
				});
			}

			if(get_scene()->is_mouse_pressed(core::ctrl::mouse::RMB)){
				remove_drawer(rect, [this](math::upoint2 idx){
					return grid[idx].building != nullptr;
				});
			}
		}

	}


	drawEnd:

	if(selected_building){
		auto b = get_region_at(selected_building->get_indexed_region());
		draw::fancy::select_rect(acquirer, b, 8, theme::colors::accent, ecs::chamber::tile_size / 1.5f, true, 1.f);
	}


	viewport_end();

	static constexpr auto shadow = colors::black.copy().set_a(.85f);
	auto drawShadow = [&](const ui::rect bound){
		acquirer.proj.mode_flag = draw::mode_flags::sdf;
		acquirer.proj.set_layer(ui::draw_layers::light);
		draw::nine_patch(acquirer, theme::shapes::base, bound, shadow);

		acquirer.proj.set_layer(ui::draw_layers::base);
		draw::nine_patch(acquirer, theme::shapes::base, bound, shadow);
	};

	drawShadow(grid_info_bar_->get_bound());
	grid_info_bar_->draw(clipSpace);

	if(selected_building != nullptr){
		drawShadow(current_selected_building_info_pane_->get_bound());
		current_selected_building_info_pane_->draw(clipSpace);
	}
}

mo_yanxi::math::frect mo_yanxi::game::ui::grid_editor_viewport::get_region_at(math::irect region_in_world) const noexcept{
	region_in_world.scl(ecs::chamber::tile_size_integral, ecs::chamber::tile_size_integral);
	return region_in_world.as<float>();
}

mo_yanxi::math::point2 mo_yanxi::game::ui::grid_editor_viewport::get_world_pos_to_tile_coord(math::vec2 coord) noexcept{
	return coord.div(ecs::chamber::tile_size).floor().as<int>();
}


mo_yanxi::math::irect mo_yanxi::game::ui::grid_editor_viewport::get_selected_place_region(
	math::usize2 extent,
	math::rect_ortho_trivial<float> rect) const noexcept{
	const auto chamberSize = (extent * ecs::chamber::tile_size_integral).as<float>();
	auto src = get_tile_coord(rect.src, extent);
	const auto src_in_world = (src * ecs::chamber::tile_size_integral).as<float>();

	auto ext = rect.get_abs_extent();

	if(rect.extent.x >= 0){
		ext.x += rect.src.x - src_in_world.x;
	}else{
		ext.x += src_in_world.x + chamberSize.x - rect.src.x;
	}

	if(rect.extent.y >= 0){
		ext.y += rect.src.y - src_in_world.y;
	}else{
		ext.y += src_in_world.y + chamberSize.y - rect.src.y;
	}

	math::isize2 size = (ext / (extent * ecs::chamber::tile_size_integral).as<float>()).trunc().as<int>().add(1)
			* extent.as<int>()
			* rect.extent.as<int>().sign();

	if(size.x < 0){
		src.x += extent.x;
	}

	if(size.y < 0){
		src.y += extent.y;
	}

	return math::irect{tags::from_extent, src, size};
}

mo_yanxi::math::point2 mo_yanxi::game::ui::grid_editor_viewport::get_current_tile_coord() const noexcept{
	math::vec2 off{};

	if(current_chamber){
		off = ((current_chamber->extent * ecs::chamber::tile_size_integral).as<float>() - math::vec2{}.set(ecs::chamber::tile_size)) * 0.5f;
	}

	return get_world_pos_to_tile_coord(get_transferred_cursor_pos() - off);
}

mo_yanxi::math::point2 mo_yanxi::game::ui::grid_editor_viewport::get_tile_coord(math::vec2 world_pos,
	math::usize2 extent) const noexcept{
	math::vec2 off{};

	off = ((extent * ecs::chamber::tile_size_integral).as<float>() - math::vec2{}.set(ecs::chamber::tile_size)) * 0.5f;

	return get_world_pos_to_tile_coord(world_pos - off);
}

void mo_yanxi::game::ui::grid_editor::chamber_info_elem::build(){
	{
		auto head = function_init([this](label& label){
			label.set_style();
			label.set_fit();
			label.set_text(chamber->name);
			label.text_entire_align = align::pos::center;
		});

		head.cell().set_size({400, 35});
	}

	auto canvas = end_line().create(creation::general_canvas{[&c = *chamber](const elem& e, rect clip){
		auto ext = c.extent;


		auto useable_region = align::embed_to(align::scale::fit, ext.as<float>(), e.content_size());
		auto srcOff = align::get_offset_of(align::pos::center, useable_region, e.prop().content_bound_absolute());
		auto unit_size = useable_region / ext.as<float>();

		auto acq = mo_yanxi::ui::get_draw_acquirer(e.get_renderer());
		using namespace graphic;
		acq.acquire(ext.area());

		for(int x = 0; x < ext.x; ++x){
			for(int y = 0; y < ext.y; ++y){
				math::vec2 pos = srcOff + unit_size.copy().mul(x, y);
				math::frect rect{tags::from_extent, pos, unit_size};
				rect.shrink(3, 3);
				draw::fill::rect_ortho(acq.get(), rect, colors::gray.copy().set_a(.3f));
			}
		}

		if(auto trs = c.get_part_offset()){
			auto pos = *trs * useable_region + srcOff;
		   draw::line::square(acq, {pos, math::pi_half / 2}, 16, 4, theme::colors::accent);
		}

	}});

	canvas.elem().set_style();
}

void mo_yanxi::game::ui::grid_editor::load_chambers(){
	content::chambers.each_content<meta::chamber::chamber_types>([this]<typename C>(C& c){
		chambers.at<C>().push_back(std::addressof(c));
	});

}

void mo_yanxi::game::ui::grid_editor::build_menu(){
	{
		menu_->get_default_elem().set_style();

		auto hdl = menu_->get_default_elem().function_init([](label& label){
			label.set_style();
			label.set_scale(2);
			label.set_fit();
			label.prop().graphic_data.inherent_opacity = .5f;
			label.set_text("Chamber Edit");
		});

		hdl.cell().margin.set(32);
	}

	{
		auto b = side_menu->end_line().emplace<button<label>>();

		b->set_style(theme::styles::side_bar_whisper);
		b->set_fit();
		b->set_text("from mesh");
		b->set_button_callback(button_tags::general, [this]{
			auto& selector = creation::create_file_selector(creation::file_selector_create_info{
				*this, [](const creation::file_selector& s, const grid_editor&){
					return s.get_current_main_select().has_value();
				}, [](const creation::file_selector& s, grid_editor& self){
					self.viewport->set_reference(load_hitbox_from(s.get_current_main_select().value()), true);
					return true;
				}});
			selector.set_cared_suffix({".hbox"});
		});
	}

	{
		auto b = side_menu->end_line().emplace<button<label>>();

		b->set_style(theme::styles::side_bar_whisper);
		b->set_fit();
		b->set_text("set mesh");
		b->set_button_callback(button_tags::general, [this]{
			auto& selector = creation::create_file_selector(creation::file_selector_create_info{
				*this, [](const creation::file_selector& s, const grid_editor&){
					return s.get_current_main_select().has_value();
				}, [](const creation::file_selector& s, grid_editor& self){
					self.viewport->set_reference(load_hitbox_from(s.get_current_main_select().value()), false);
					return true;
				}});
			selector.set_cared_suffix({".hbox"});
		});
	}

	{
		auto b = side_menu->end_line().emplace<button<label>>();

		b->set_style(theme::styles::side_bar_whisper);
		b->set_fit();
		b->set_text("save grid");
		b->set_button_callback(button_tags::general, [this]{
			auto& selector = creation::create_file_selector(creation::file_selector_create_info{
				.requester = *this,
				.checker = [](const creation::file_selector& s, const grid_editor&){
					return s.get_current_main_select().has_value();
				},
				.yielder = [](const creation::file_selector& s, grid_editor& self){
					std::ofstream ofs(s.get_current_main_select().value(), std::ios::binary | std::ios::out);
					auto hdl = meta::srl::write_grid(ofs, self.viewport->grid);
					hdl.resume();
					return true;
				},
				.add_file_create_button = true
			});
			selector.set_cared_suffix({".metagrid"});
		});
	}

	{
		auto b = side_menu->end_line().emplace<button<label>>();

		b->set_style(theme::styles::side_bar_whisper);
		b->set_fit();
		b->set_text("load grid");
		b->set_button_callback(button_tags::general, [this]{
			auto& selector = creation::create_file_selector(creation::file_selector_create_info{
				*this, [](const creation::file_selector& s, const grid_editor&){
					return s.get_current_main_select().has_value();
				}, [this](const creation::file_selector& s, grid_editor& self){
					std::ifstream ifs(s.get_current_main_select().value(), std::ios::binary | std::ios::in);
					meta::srl::read_grid(ifs, viewport->grid);
					return true;
				}});
			selector.set_cared_suffix({".metagrid"});
		});
	}

	{
		{
			auto sep = side_menu->end_line().create(ui::creation::general_seperator_line{
					.stroke = 20,
					.palette = ui::theme::style_pal::front_white.copy().mul_alpha(.25f)
				});
			sep.cell().pad.set_vert(4);
			sep.cell().margin.set_hori(8);
		}
	}


	{
		auto b = side_menu->end_line().emplace<button<label>>();

		b->set_style(theme::styles::side_bar_whisper);
		b->set_fit();
		b->set_text("edit grid");
		b->set_button_callback(button_tags::general, [this]{
			viewport->focus_on_grid = !viewport->focus_on_grid;
		});
		b->checkers.setActivatedProv([this]{
			return viewport->focus_on_grid;
		});
	}

	{
		for(int i = 0; i < std::to_underlying(meta::chamber::category::LAST); ++i){
			auto rst = menu_->add_elem<label, scroll_pane>();
			rst.button->set_fit();
			rst.button->set_text([i](){
				std::string str{meta::chamber::category_names[i]};
				str.front() = std::toupper(str.front());
				return str;
			}());
			rst.button->text_entire_align = align::pos::center_left;
			rst.button->property.size.set_minimum_size({400, 0});
			rst.button->set_style(theme::styles::no_edge);

			rst.elem.set_layout_policy(layout_policy::vert_major);
			rst.elem.set_style();

			rst.elem.set_elem([&, this](table& table){
				table.set_style();
				chambers.each([&, this]<typename T>(std::in_place_type_t<T> tag, const decltype(chambers)::value_type& values){
					for (auto basic_chamber : values){
						if(basic_chamber->category != meta::chamber::category{i})continue;
						if(table.has_children()){
							table.get_last_cell().set_pad({.right = 8});
						}

						create_handle<chamber_info_elem, cell_type> hdl = table.emplace<chamber_info_elem>(*this, *basic_chamber);
						hdl.cell().set_external({true, false});
					}
				});

			});
		}

	}
}
