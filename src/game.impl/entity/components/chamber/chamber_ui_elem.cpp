module mo_yanxi.game.ui.chamber_ui_elem;

import mo_yanxi.ui.graphic;

void mo_yanxi::game::ui::build_tile_status_drawer::draw(
	ui::rect region, float opacity,
	graphic::renderer_ui_ref renderer, const ecs::chamber::building_data& data) {
	static constexpr float shrinkion = 2;
	region.expand(shrinkion);

	auto acq = mo_yanxi::ui::get_draw_acquirer(renderer);
	// acq.proj.set_layer(ui::draw_layers::base);
	using namespace graphic;
	auto sz = data.region().extent();
	bool swap = sz.x < sz.y;
	if(swap)sz.swap_xy();

	const auto unit_size = get_unit_size(region.extent(), sz);
	const auto unit_rect = math::frect{0.f, 0.f, unit_size, unit_size}.shrink(shrinkion);

	auto building_max_individual = data.get_tile_individual_max_hitpoint();

	acq.proj.mode_flag = draw::mode_flags::slide_line;
	for(int y = 0; y < sz.y; ++y){
		for(int x = 0; x < sz.x; ++x){
			auto& status = data.tile_states[swap ? (x * sz.y + y) : (y * sz.x + x)];

			auto rect = unit_rect.copy().scl_size(status.valid_structure_hit_point / building_max_individual, 1);
			draw::fill::rect_ortho(acq.get(), rect.move(region.src + math::vector2{x, y}.as<float>() * unit_size),
								   (status.valid_hit_point < building_max_individual *
									ecs::chamber::tile_status::threshold_factor
										? colors::danger
										: colors::light_gray)
										.to_neutralize_light().set_a(opacity));
		}
	}

	acq.proj.mode_flag = draw::mode_flags::none;
	float build_total_health_factor = data.hit_point.get_capability_factor();
	for(int y = 0; y < sz.y; ++y){
		for(int x = 0; x < sz.x; ++x){
			auto& status = data.tile_states[swap ? (x * sz.y + y) : (y * sz.x + x)];

			auto rect = unit_rect.copy().scl_size(status.valid_hit_point / building_max_individual, 1);
			draw::fill::rect_ortho(acq.get(), rect.move(region.src + math::vector2{x, y}.as<float>() * unit_size),
				(colors::red_dusted.create_lerp(colors::pale_green, build_total_health_factor)).to_neutralize_light().set_a(opacity));
		}
	}
}

void mo_yanxi::game::ui::chamber_ui_elem::build(){
	clear_children();
	hitpoint_bar = nullptr;

	if(!chamber_entity)return;

	template_cell.set_external({false, true});

	{
		auto elem = end_line().emplace<ui::stalled_bar>();
		elem->set_style();
		elem.cell().set_height(30);
		elem.cell().margin.set(2);
		elem->approach_speed = 0.125f;

		hitpoint_bar = std::to_address(elem);
	}
	{
		auto elem = end_line().emplace<build_tile_status_elem>();
		elem->set_style();
		elem->entity = chamber_entity;

		status_ = std::to_address(elem);
	}
}

void mo_yanxi::game::ui::chamber_ui_elem::update(float delta_in_ticks){
	table::update(delta_in_ticks);

	if(!chamber_entity)return;
	auto& data = chamber_entity->at<ecs::chamber::building_data>();

	if(hitpoint_bar){
		hitpoint_bar->set_value(data.hit_point.factor());
		hitpoint_bar->normalized_valid_range = data.hit_point.capability_range / data.hit_point.max;
	}
}
