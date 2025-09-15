module;

#include "srl/srl.game.hpp"
#include "srl/srl.hpp"
#include <magic_enum/magic_enum.hpp>

module mo_yanxi.game.ui.grid_editor;

import mo_yanxi.ui.graphic;
import mo_yanxi.ui.elem.image_frame;
import mo_yanxi.graphic.draw.multi_region;
import mo_yanxi.graphic.layers.ui.grid_drawer;

import mo_yanxi.game.meta.grid.srl;
import mo_yanxi.game.content;
import mo_yanxi.game.meta.hitbox;
import mo_yanxi.math.rand;

import std;


namespace mo_yanxi::game::meta::chamber{
	struct default_color_prov{
		std::optional<graphic::color> operator()(auto&&) const noexcept{
			return std::nullopt;
		}
	};

	static_assert(std::invocable<default_color_prov, const grid_building&>);
	static_assert(std::invocable<default_color_prov, const grid_tile&>);

	template <
		std::invocable<const grid_building&> BuildingColorProvFn = default_color_prov,
		std::invocable<const grid_tile&> TileColorProvFn = default_color_prov
	>
	void draw_grid(
		const grid& grid,
		graphic::renderer_ui_ref renderer,
		const graphic::camera2& camera,
		math::point2 offset = {},
		float opacity_scl = 1.f,
		BuildingColorProvFn fn = {},
		TileColorProvFn tfn = {}
		){
		auto acquirer = ui::get_draw_acquirer(renderer);
		using namespace graphic;

		acquirer.proj.set_layer(ui::draw_layers::base);
		const auto [w, h] = grid.get_extent();

		{
			const auto region = grid.get_grid_region().move(offset).as<float>().scl(tile_size, tile_size);

			const auto color = colors::gray.copy().set_a(.5f * opacity_scl);

			for(unsigned x = 0; x <= w; ++x){
				static constexpr math::vec2 off{tile_size, 0};
				draw::line::line_ortho(acquirer.get(), region.vert_00() + off * x, region.vert_01() + off * x, 4, color, color);
			}

			for(unsigned y = 0; y <= h; ++y){
				static constexpr math::vec2 off{0, tile_size};
				draw::line::line_ortho(acquirer.get(), region.vert_00() + off * y, region.vert_10() + off * y, 4, color, color);
			}
		}

		acquirer.proj.set_layer(ui::draw_layers::def);
		acquirer.proj.mode_flag = draw::mode_flags::slide_line;
		const auto off = grid.get_origin_offset() + offset;
		for(unsigned y = 0; y < h; ++y){
			for(unsigned x = 0; x < w; ++x){
				auto& info = grid[x, y];
				std::optional<color> opt_grid_color = std::invoke(tfn, info);
				if(info.is_idle() || opt_grid_color){
					color tileC;
					if(!opt_grid_color){
						tileC = grid.is_within_structural(math::upoint2{x, y}) ? colors::pale_yellow : colors::aqua;
					}else{
						tileC = *opt_grid_color;
					}
					tileC.mul_a(.25f * opacity_scl);

					auto sz = math::irect{tags::from_extent, off.copy().add(x, y), 1, 1}.as<float>().scl(tile_size, tile_size);
					draw::fill::rect_ortho(acquirer.get(), sz, tileC);
				}

				if (info.is_building_identity({x, y})){
					acquirer.proj.mode_flag = {};

					auto rg = info.building->get_indexed_region().as<int>().move(off).as<float>().scl(tile_size, tile_size).shrink(2);
					draw::line::rect_ortho(acquirer, rg, 4, std::invoke_r<std::optional<color>>(fn, *info.building).value_or(info.building->get_meta_info().get_edit_outline_color()).set_a(opacity_scl));
					info.building->get_meta_info().draw(rg, renderer, camera);
					if(auto ist = info.building->get_instance_data()){
						ist->draw(info.building->get_meta_info(), rg, renderer, camera.get_scale());
					}
					acquirer.proj.mode_flag = draw::mode_flags::slide_line;
				}
			}
		}
	}
}

mo_yanxi::game::meta::hitbox_transed load_hitbox_from(const std::filesystem::path& path){
	using namespace mo_yanxi;
	using namespace game;
	meta::hitbox_transed meta{};
	std::ifstream stream(path, std::ios::in | std::ios::binary);
	io::loader<meta::hitbox_transed>::parse_from(stream, meta);

	return meta;
}

void mo_yanxi::game::ui::grid_info_bar::draw_content(const ui::rect clipSpace) const{
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

mo_yanxi::game::ui::grid_editor_viewport& mo_yanxi::game::ui::grid_detail_pane::get_viewport() const noexcept{
	//button table -> scroll pane -> detail pane
	return *get_parent<grid_editor_viewport>();
}

void mo_yanxi::game::ui::grid_detail_pane::clear_viewport_relevant_state() const noexcept{
	get_viewport().subpanel_drawer = nullptr;
}

void mo_yanxi::game::ui::grid_detail_pane::build(){
	set_style();
	template_cell.margin.set(4);
	{
		auto pane = emplace<scroll_pane>();
		pane.cell().region_scale = {tags::from_extent, math::vec2{}, math::vec2{.3f, 1.f}};
		pane.cell().align = align::pos::top_right;


		pane->set_style(ui::theme::styles::general_static);
		pane->function_init([](ui::table& button_table){
			button_table.name = "button_table";
			button_table.set_style();

			button_table.template_cell.set_height_from_scale();
			button_table.template_cell.set_pad(4);

			struct button final : ui::single_image_frame<ui::icon_drawable>{
				[[nodiscard]] button(
					scene* scene, group* group,
					detail_pane_mode index)
					: single_image_frame(scene, group, theme::icons::close), index(index){

					set_tooltip_state({
						.layout_info = tooltip_layout_info{
							.follow = tooltip_follow::cursor,
							.align_owner = {},
							.align_tooltip = align::pos::top_right,
						},
						.min_hover_time = 5
					}, [index](ui::table& l){
						l.interactivity = interactivity::disabled;
						l.set_style();
						l.template_cell.set_external();

						l.function_init([index](ui::label& label){
							label.set_scale(.5f);
							label.set_text(magic_enum::enum_name(index));
						});
					});
				}

				detail_pane_mode index{0};

				grid_detail_pane& get_pane() const noexcept{
					//button table -> scroll pane -> detail pane
					return *get_parent()->get_parent()->get_parent<grid_detail_pane>();
				}

				void update(float delta_in_ticks) override{
					auto& p = get_pane();
					// disabled = p.get_viewport().grid.e
					activated = p.current_mode_ == index;

					single_image_frame::update(delta_in_ticks);
				}

				input_event::click_result on_click(const input_event::click click_event) override{
					if(click_event.code.matches(core::ctrl::lmb_click)){
						auto& p = get_pane();
						p.set_mode(p.current_mode_ == index ? detail_pane_mode::monostate : index);
					}

					return input_event::click_result::intercepted;
				}
			};

			for(auto i = 0u; i < static_cast<unsigned>(detail_pane_mode::monostate); ++i){
				button_table.emplace<button>(detail_pane_mode{i})->set_style(ui::theme::styles::no_edge);
				if(i & 1u){
					button_table.end_line();
				}
			}

			button_table.set_edge_pad(0);
		});
	}

	auto pane = emplace<scroll_pane>();
	pane.cell().region_scale = {tags::from_extent, math::vec2{}, math::vec2{.7f, 1.f}};
	pane.cell().align = align::pos::top_left;
}

mo_yanxi::ui::elem_ptr mo_yanxi::game::ui::grid_detail_pane::prov(){
	switch(current_mode_){
	//
	// case detail_pane_mode::statistic :{
	// 	break;
	// }
	case detail_pane_mode::power_state : return {get_scene(), this, std::in_place_type<power_state_panel>};
	case detail_pane_mode::structural_state : return {get_scene(), this, std::in_place_type<grid_structural_panel>};
	case detail_pane_mode::maneuvering : return {get_scene(), this, std::in_place_type<maneuvering_panel>};
	// 	break;
	// }
	// case detail_pane_mode::corridor :{
	// 	break;
	// }
	// case detail_pane_mode::defense_active :{
	// 	break;
	// }
	// case detail_pane_mode::defense_passive :{
	// 	break;
	// }
	case detail_pane_mode::monostate: return {};
	default : return elem_ptr{get_scene(), this, [this](ui::label& l){
			l.set_text(magic_enum::enum_name(current_mode_));
			l.set_fit();
		}};
	}
}

mo_yanxi::game::ui::grid_editor_viewport& mo_yanxi::game::ui::grid_info_bar::get_editor() const{
	return *static_cast<grid_editor_viewport*>(get_parent());
}

void mo_yanxi::game::ui::grid_editor_viewport::draw_content(const rect clipSpace) const{
	viewport_begin();
	using namespace graphic;
	auto& r = renderer_from_erased(get_renderer());
	auto acquirer{mo_yanxi::ui::get_draw_acquirer(r)};

	{
		//draw grid
		batch_layer_guard guard(r.batch, std::in_place_type<layers::grid_drawer>);

		draw::fill::rect_ortho(acquirer.get(), camera.get_viewport(), colors::black.copy().set_a(.35));
	}

	{
		//draw hitbox reference
		acquirer.proj.set_layer(ui::draw_layers::base);

		for(const auto& comp : reference){
			draw::line::quad_expanded(acquirer, comp.crop().view_as_quad(), -4, colors::light_gray.copy().set_a(.35));
		}

		acquirer.proj.mode_flag = {};
		acquirer.proj.set_layer(ui::draw_layers::def);
	}


	auto mirrow_axis_drawer = [&, this](mirrow_axis_vec2 vec, float opacity = 1.f){
		using namespace ecs::chamber;
		math::vec2 center{};
		if(vec.x){
			center.y = vec.x->coord * tile_size + (tile_size / 2) * vec.x->at_mid;
		}

		if(vec.y){
			center.x = vec.y->coord * tile_size + (tile_size / 2) * vec.y->at_mid;
		}

		if(vec.x){
			draw::line::line_angle_center(acquirer.get(), {center, 0}, 1E5, 4, colors::RED.copy_set_a(opacity));
		}

		if(vec.y){
			draw::line::line_angle_center(acquirer.get(), {center, math::pi_half}, 1E5, 4, colors::ACID.copy_set_a(opacity));
		}
	};


	{
		auto placement_drawer = [this, &acquirer](math::usize2 chamber_extent, auto pred){
			this->selection_each(chamber_extent, [&](math::upoint2 idx, math::point2 pos){
				if(pred(idx)){
					draw::fill::rect_ortho(acquirer.get(), get_region_at({tags::from_extent, pos, chamber_extent.as<int>()}).shrink(4), colors::pale_green.copy().set_a(.5f));
				}
			}, [&](math::irect place_region){
				acquirer.proj.mode_flag = {};
				draw::line::rect_ortho(acquirer, get_region_at(place_region), 4, colors::pale_green);
				acquirer.proj.mode_flag = draw::mode_flags::slide_line;
			});
			acquirer.proj.mode_flag = {};
		};

		auto remove_drawer = [this, &acquirer](auto pred){
			this->selection_each({1, 1}, [&](math::upoint2 idx, math::point2 pos){
				if(pred(idx)){
					draw::fill::rect_ortho(acquirer.get(), get_region_at({pos, 1, 1}).shrink(4), colors::red_dusted.copy().set_a(.5f));
				}
			}, [&](const math::irect& place_region){
				acquirer.proj.mode_flag = {};
				draw::line::rect_ortho(acquirer, get_region_at(place_region), 4, colors::red_dusted);
				acquirer.proj.mode_flag = draw::mode_flags::slide_line;
			});
			acquirer.proj.mode_flag = {};
		};

		static constexpr auto prov = [](const meta::chamber::grid_building&){
			return std::optional{colors::gray.copy().set_a(.65)};
		};

		if(is_focused_on_grid()){
			if(grid_op_ == operation::move){
				auto mov = get_grid_edit_offset(get_scene()->get_cursor_pos());
				meta::chamber::draw_grid(grid, r, camera, {}, .5f, prov);
				meta::chamber::draw_grid(grid, r, camera, mov, 1.f, prov);
				mirrow_axis_drawer(get_moved_axis(mirrow_channel.get_current_axis(), mov), .75f);
			}else{
				meta::chamber::draw_grid(grid, r, camera, {}, 1.f, prov);

				if(last_click_ && get_scene()->get_input_mode() & core::ctrl::mode::ctrl_shift){
					if(get_scene()->is_mouse_pressed(core::ctrl::mouse::LMB)){
						placement_drawer({1, 1}, [this](math::upoint2 idx){
							return !grid[idx].placeable;
						});
					}else if(get_scene()->is_mouse_pressed(core::ctrl::mouse::RMB)){
						remove_drawer([this](math::upoint2 idx){
							return grid[idx].placeable && grid[idx].building == nullptr;
						});
					}
				}

				if (!last_click_ || get_scene()->get_input_mode() == 0){
					for (const auto & mirrowed_region : get_mirrowed_region(
						mirrow_channel.get_current_axis(),
						math::irect{
							tags::from_extent,
							get_world_pos_to_tile_coord(get_transferred_cursor_pos()),
							1, 1})){
						if((grid.coord_to_index(mirrowed_region.vert_00()))){
							draw::line::rect_ortho(acquirer, get_region_at(mirrowed_region), 4, colors::pale_green.copy().set_a(.5f));
						}
					}
				}
			}
		}else if(grid_detail_pane_->is_focused_on(detail_pane_mode::corridor)){
			meta::chamber::draw_grid(
				grid, r, camera, {}, 1.f, prov,
				[this](const meta::chamber::grid_tile& tile) -> std::optional<color>{
					if(tile.is_corridor){
						if(tile.building){
							const auto cid = grid.get_tile_corridor_id(tile.corridor_group_id);
							const auto hue = math::rand{cid}.next<float>();

							return colors::pale_green.copy().shift_hue(math::lerp(-1.f, 1.f, hue) * 60).set_a(3.f);

						}else{
							return colors::red_dusted.copy().set_a(tile.placeable ? 1.f : .5f);
						}
					} else{
						if(!tile.placeable) return std::nullopt;
						return tile.building
							       ? colors::gray
							       : colors::dark_gray.copy().set_a(.75);
					}
				}
			);

			if(last_click_ && get_scene()->get_input_mode() & core::ctrl::mode::ctrl_shift){
				if(get_scene()->is_mouse_pressed(core::ctrl::mouse::LMB)){
					placement_drawer({1, 1}, [this](math::upoint2 idx){
						return !grid[idx].is_corridor;
					});
				}else if(get_scene()->is_mouse_pressed(core::ctrl::mouse::RMB)){
					remove_drawer([this](math::upoint2 idx){
						return grid[idx].is_corridor;
					});
				}
			}

			if (!last_click_ || get_scene()->get_input_mode() == 0){
				for (const auto & mirrowed_region : get_mirrowed_region(
					mirrow_channel.get_current_axis(),
					math::irect{
						tags::from_extent,
						get_world_pos_to_tile_coord(get_transferred_cursor_pos()),
						1, 1})){
					if((grid.coord_to_index(mirrowed_region.vert_00()))){
						draw::line::rect_ortho(acquirer, get_region_at(mirrowed_region), 4, colors::pale_green.copy().set_a(.5f));
					}
				}
			}

			if(path_dst && current_request->try_wait_done()){
				auto pth = current_request->get_path();
				for (auto && seq : pth){
					math::irect rect{tags::from_extent, seq.as<int>() + grid.get_origin_offset(), 1, 1};
					draw::fill::rect_ortho(acquirer.get(), get_region_at(rect), colors::PURPLE.copy().set_a(.3f));
				}
			}

			if(path_src){
				math::irect rect{tags::from_extent, *path_src, 1, 1};
				draw::line::rect_ortho(acquirer, get_region_at(rect), 6, colors::CRIMSON.copy().set_a(.5f));
			}

			if(path_dst){
				math::irect rect{tags::from_extent, *path_dst, 1, 1};
				draw::line::rect_ortho(acquirer, get_region_at(rect), 6, colors::ORANGE.copy().set_a(.5f));
			}



		}else{
			meta::chamber::draw_grid(grid, r, camera, {}, 1.f, std::bind_front(&grid_detail_pane::get_override_color, grid_detail_pane_));

			if (current_chamber && (!last_click_ || get_scene()->get_input_mode() == 0)){
				auto extent = current_chamber->extent.as<int>();
				auto currentCoord = get_current_tile_coord();
				math::irect to_place_region{tags::from_extent, currentCoord, extent};

				for (const auto & mirrowed_region : get_mirrowed_region(
						mirrow_channel.get_current_axis(), to_place_region)){
					auto world_region = get_region_at(mirrowed_region);

					if(auto idxp = grid.coord_to_index(mirrowed_region.vert_00()); idxp && grid.is_building_placeable_at(*idxp, *current_chamber)){
						draw::line::rect_ortho(acquirer, world_region, 4, colors::pale_green.copy().set_a(.5f));
					}else{
						draw::line::rect_ortho(acquirer, world_region, 4, colors::red_dusted.copy().set_a(.5f));
					}
				}

			}

			if(last_click_ && get_scene()->get_input_mode() & core::ctrl::mode::ctrl_shift){

				if(current_chamber && get_scene()->is_mouse_pressed(core::ctrl::mouse::LMB)){
					placement_drawer(current_chamber->extent, [this](math::upoint2 idx){
						return grid.is_building_placeable_at(idx, *current_chamber);
					});
				}

				if(get_scene()->is_mouse_pressed(core::ctrl::mouse::RMB)){
					remove_drawer([this](math::upoint2 idx){
						return grid[idx].building != nullptr;
					});
				}
			}

		}
	}

	drawEnd:

	if(selected_building){
		auto b = get_region_at(selected_building->get_indexed_region());
		draw::fancy::select_rect(acquirer, b, 8, theme::colors::accent, ecs::chamber::tile_size / 1.5f, true, 1.f);
	}

	{
		if(mirrow_channel.is_editing()){
			mirrow_axis_drawer(mirrow_channel.get_temp_axis(get_world_pos_to_tile_coord(get_transferred_cursor_pos())));
			mirrow_axis_drawer(mirrow_channel.get_current_axis(), .3f);
		}else{
			mirrow_axis_drawer(mirrow_channel.get_current_axis());
		}
	}

	if(subpanel_drawer){
		subpanel_drawer(*this);
	}

	viewport_end();

	//add ui shadows

	static constexpr auto shadow = colors::black.copy().set_a(.85f);
	auto drawShadow = [&](const ui::rect bound){
		acquirer.proj.mode_flag = draw::mode_flags::sdf;
		acquirer.proj.set_layer(ui::draw_layers::light);
		draw::nine_patch(acquirer, theme::shapes::base, bound, shadow);

		acquirer.proj.set_layer(ui::draw_layers::base);
		draw::nine_patch(acquirer, theme::shapes::base, bound, shadow);
	};

	drawShadow(grid_detail_pane_->get_shadow_region());
	grid_detail_pane_->draw(clipSpace);

	if(selected_building != nullptr){
		drawShadow(current_selected_building_info_pane_->get_bound());
		current_selected_building_info_pane_->draw(clipSpace);
	}
}

mo_yanxi::math::frect mo_yanxi::game::ui::grid_editor_viewport::get_region_at(math::irect region_in_world) const noexcept{
	region_in_world.scl(ecs::chamber::tile_size_integral, ecs::chamber::tile_size_integral);
	return region_in_world.as<float>();
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
	math::usize2 extent) noexcept{
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
					self.viewport->reset_editor();

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
				}, [](const creation::file_selector& s, grid_editor& self){
					std::ifstream ifs(s.get_current_main_select().value(), std::ios::binary | std::ios::in);
					meta::srl::read_grid(ifs, self.viewport->grid);
					self.viewport->reset_editor();
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

			rst.elem.function_init([&, this](table& table){
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
