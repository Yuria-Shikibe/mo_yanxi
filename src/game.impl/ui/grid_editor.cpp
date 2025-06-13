module;

#include "srl/srl.game.hpp"
#include "srl/srl.hpp"

module mo_yanxi.game.ui.grid_editor;

import mo_yanxi.ui.graphic;
import mo_yanxi.graphic.draw.multi_region;
import mo_yanxi.graphic.layers.ui.grid_drawer;

import mo_yanxi.game.content;

mo_yanxi::game::meta::hitbox_transed mo_yanxi::game::load_hitbox_from(const std::filesystem::path& path){
	meta::hitbox_transed meta{};
	std::ifstream stream(path, std::ios::in | std::ios::binary);
	io::loader<meta::hitbox_transed>::parse_from(stream, meta);

	return meta;
}

void mo_yanxi::game::ui::grid_editor_viewport::draw_content(const rect clipSpace) const{
	viewport_begin();
	auto& r = get_renderer();
	using namespace graphic;
	auto acquirer{mo_yanxi::ui::get_draw_acquirer(r)};



	{
		batch_layer_guard guard(r.batch, std::in_place_type<layers::grid_drawer>);

		draw::fill::rect_ortho(acquirer.get(), camera.get_viewport(), colors::black.copy().set_a(.35));
	}

	acquirer.proj.set_layer(ui::draw_layers::base);

	for(const auto& comp : reference){
		draw::line::quad_expanded(acquirer, comp.crop().view_as_quad(), -4, colors::light_gray.copy().set_a(.35));
	}

	grid.draw(get_renderer(), camera);

	acquirer.proj.mode_flag = {};
	acquirer.proj.set_layer(ui::draw_layers::def);

	if (current_chamber && get_scene()->get_input_mode() == 0){
		auto extent = current_chamber->extent.as<int>();
		auto currentCoord = get_current_tile_coord();
		math::irect to_place_region{tags::from_extent, currentCoord, extent};
		auto world_region = get_region_at(to_place_region);

		if(auto idxp = grid.coord_to_index(currentCoord); idxp && grid.is_building_placeable_at(*idxp, current_chamber)){
			draw::line::rect_ortho(acquirer, world_region, 4, colors::pale_green.copy().set_a(.5f));
		}else{
			draw::line::rect_ortho(acquirer, world_region, 4, colors::red_dusted.copy().set_a(.5f));
		}

	}

	if(last_click_ && get_scene()->get_input_mode() & core::ctrl::mode::ctrl_shift){
		math::rect_ortho_trivial rect{get_select_box()};

		if(current_chamber && get_scene()->is_mouse_pressed(core::ctrl::mouse::LMB)){
			math::irect place_region = get_selected_place_region(current_chamber->extent, rect);
			draw::line::rect_ortho(acquirer, place_region.as<float>().scl(ecs::chamber::tile_size, ecs::chamber::tile_size), 4, colors::pale_green);

			auto chamber_sz = current_chamber->extent.as<int>();

			acquirer.proj.mode_flag = draw::mode_flags::slide_line;
			each_tile(place_region, chamber_sz, [&, this](math::point2 pos){
				if(auto idxp = grid.coord_to_index(pos)){
					if(grid.is_building_placeable_at(*idxp, current_chamber)){
						draw::fill::rect_ortho(acquirer.get(), get_region_at({tags::from_extent, pos, chamber_sz}).shrink(4), colors::pale_green.copy().set_a(.5f));
					}
				}
			});
			acquirer.proj.mode_flag = {};
		}

		if(get_scene()->is_mouse_pressed(core::ctrl::mouse::RMB)){
			math::irect place_region = get_selected_place_region({1, 1}, rect);
			draw::line::rect_ortho(acquirer, place_region.as<float>().scl(ecs::chamber::tile_size, ecs::chamber::tile_size), 4, colors::red_dusted);

			acquirer.proj.mode_flag = draw::mode_flags::slide_line;
			each_tile(place_region, {1, 1}, [&, this](math::point2 pos){
				if(auto idxp = grid.coord_to_index(pos)){
					if(grid[idxp.value()].building != nullptr){
						draw::fill::rect_ortho(acquirer.get(), get_region_at({pos, 1, 1}).shrink(4), colors::red_dusted.copy().set_a(.5f));
					}
				}
			});
			acquirer.proj.mode_flag = {};
		}
	}

	drawEnd:

	if(selected_building){
		auto b = get_region_at(selected_building->get_indexed_region());
		draw::fancy::select_rect(acquirer, b, 8, theme::colors::accent, ecs::chamber::tile_size / 1.5f, true, 1.f);
	}


	viewport_end();

	if(selected_building != nullptr){
		acquirer.proj.mode_flag = draw::mode_flags::sdf;
		draw::nine_patch(acquirer, theme::shapes::base, pane_->get_bound(), colors::black.copy().set_a(.85f));

		acquirer.proj.set_layer(ui::draw_layers::base);
		draw::nine_patch(acquirer, theme::shapes::base, pane_->get_bound(), colors::black.copy().set_a(.85f));
		viewport::draw_content(clipSpace);
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


		auto useable_region = align::embedTo(align::scale::fit, ext.as<float>(), e.content_size());
		auto srcOff = align::get_offset_of(align::pos::center, useable_region, e.prop().content_bound_absolute());
		auto unit_size = useable_region / ext.as<float>();

		auto acq = ui::get_draw_acquirer(e.get_renderer());
		using namespace graphic;
		acq.acquire(ext.area());
		for(int y = 0; y < ext.y; ++y){
			for(int x = 0; x < ext.x; ++x){
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
	content::content_manager.each_content<meta::chamber::chamber_types>([this]<typename C>(C& c){
		chambers.at<C>().push_back(std::addressof(c));
	});

}
