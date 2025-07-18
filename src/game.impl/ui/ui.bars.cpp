module mo_yanxi.game.ui.bars;

import mo_yanxi.ui.assets;
import mo_yanxi.ui.graphic;
import mo_yanxi.math.rect_ortho;
import mo_yanxi.graphic.draw;

void mo_yanxi::game::ui::stalled_hp_bar_drawer::draw(math::frect region, float opacity, graphic::renderer_ui_ref renderer_ref) const{
	auto acq = mo_yanxi::ui::get_draw_acquirer(renderer_ref);
	using namespace graphic;

	const auto src = region.src;
	const auto [w, h] = region.extent();

	if(stall){
		float scale = (stall->value - current_value);
		if(scale > 0)draw::fill::rect_ortho(acq.get(), math::frect{tags::from_extent, src.copy().add_x(current_value * w), {w * scale, h}}, colors::dark_gray.copy().set_a(opacity));
	}

	get_chunked_progress_region({valid_range.from, valid_range.to}, current_value, [&](auto idx, float f, float t){
		float dst = t - f;
		float width = dst * w;
		if(width > std::numeric_limits<float>::epsilon()){
			color color;
			switch(idx){
			case 0: color = colors::light_gray.to_neutralize_light(); acq.proj.mode_flag = draw::mode_flags::slide_line; break;
			case 1: color = colors::pale_green.to_neutralize_light(); acq.proj.mode_flag = draw::mode_flags{}; break;
			case 2: color = theme::colors::accent.to_neutralize_light(); acq.proj.mode_flag = draw::mode_flags{}; break;
			default: std::unreachable();
			}

			color.set_a(opacity);

			draw::fill::rect_ortho(acq.get(), math::frect{tags::from_extent, src.copy().add_x(f * w), {width, h}}, color);
		}
	}, [](auto, float f, float t){
	});
}

void mo_yanxi::game::ui::stalled_bar::draw_content(const mo_yanxi::ui::rect clipSpace) const{
	auto acq = mo_yanxi::ui::get_draw_acquirer(get_renderer());
	using namespace graphic;

	const auto src = elem::content_src_pos();
	const auto [w, h] = content_size();

	const auto opacity = gprop().get_opacity();
	if(stall){
		float scale = (stall->value - current_value);
		if(scale > 0)draw::fill::rect_ortho(acq.get(), math::frect{tags::from_extent, src.copy().add_x(current_value * w), {w * scale, h}}, colors::dark_gray.copy().set_a(opacity));
	}


	get_chunked_progress_region({normalized_valid_range.from, normalized_valid_range.to}, current_value, [&](auto idx, float f, float t){
		float dst = t - f;
		float width = dst * w;
		if(width > std::numeric_limits<float>::epsilon()){
			color color;
			switch(idx){
			case 0: color = colors::light_gray.to_neutralize_light(); acq.proj.mode_flag = draw::mode_flags::slide_line; break;
			case 1: color = colors::pale_green.to_neutralize_light(); acq.proj.mode_flag = draw::mode_flags{}; break;
			case 2: color = theme::colors::accent.to_neutralize_light(); acq.proj.mode_flag = draw::mode_flags{}; break;
			default: std::unreachable();
			}

			color.set_a(opacity);

			draw::fill::rect_ortho(acq.get(), math::frect{tags::from_extent, src.copy().add_x(f * w), {width, h}}, color);
		}
	}, [](auto, float f, float t){
	});
}

void mo_yanxi::game::ui::reload_bar_drawer::draw(math::frect region, float opacity,
	graphic::renderer_ui_ref renderer_ref) const{

	auto acq = mo_yanxi::ui::get_draw_acquirer(renderer_ref);
	using namespace graphic;

	const auto src = region.src;
	const auto [w, h] = region.extent();

	{
		auto gradient = reload_color;
		gradient.from.set_a(opacity);
		gradient.to.set_a(opacity);
		const math::frect reload_progress{tags::from_extent, src, {w * current_reload_value, h * (1 - efficiency_bar_scale)}};

		draw::fill::fill(acq.get(),
			reload_progress.vert_00(), reload_progress.vert_01(),  reload_progress.vert_11(),  reload_progress.vert_10(),
			gradient.from, gradient.from,
			gradient[current_reload_value], gradient[current_reload_value]
		);
	}

	auto gradient = efficiency_color;
	gradient.from.set_a(opacity);
	gradient.to.set_a(opacity);
	const math::vec2 sz{w * current_efficiency, h * efficiency_bar_scale};
	const auto off = align::get_offset_of(align::pos::bottom_left, sz, region);
	const math::frect efficiency_region{tags::from_extent, off, sz};
	draw::fill::fill(acq.get(),
		efficiency_region.vert_00(), efficiency_region.vert_01(),  efficiency_region.vert_11(),  efficiency_region.vert_10(),
		gradient.from, gradient.from,
			gradient[current_efficiency], gradient[current_efficiency]
	);
}

void mo_yanxi::game::ui::reload_bar::draw_content(mo_yanxi::ui::rect clipSpace) const{
	draw_background();

	auto acq = mo_yanxi::ui::get_draw_acquirer(get_renderer());
	using namespace graphic;

	const auto src = elem::content_src_pos();
	const auto [w, h] = prop().content_size();


	const auto opacity = gprop().get_opacity();
	{
		auto gradient = reload_color;
		gradient.from.set_a(opacity);
		gradient.to.set_a(opacity);
		const math::frect reload_progress{tags::from_extent, src, {w * current_reload_value, h * (1 - efficiency_bar_scale)}};

		draw::fill::fill(acq.get(),
			reload_progress.vert_00(), reload_progress.vert_01(),  reload_progress.vert_11(),  reload_progress.vert_10(),
			gradient.from, gradient.from,
			gradient[current_reload_value], gradient[current_reload_value]
		);
	}

	auto gradient = efficiency_color;
	gradient.from.set_a(opacity);
	gradient.to.set_a(opacity);
	const math::vec2 sz{w * current_efficiency, h * efficiency_bar_scale};
	const auto off = align::get_offset_of(align::pos::bottom_left, sz, prop().content_bound_absolute());
	const math::frect efficiency_region{tags::from_extent, off, sz};
	draw::fill::fill(acq.get(),
		efficiency_region.vert_00(), efficiency_region.vert_01(),  efficiency_region.vert_11(),  efficiency_region.vert_10(),
		gradient.from, gradient.from,
			gradient[current_efficiency], gradient[current_efficiency]
	);
}

void mo_yanxi::game::ui::energy_bar_drawer::draw(math::frect region, float opacity, graphic::renderer_ui_ref renderer_ref) const{
	region.expand(2, 2);

	int abs_power_total = math::abs(state_.power_total);
	if(abs_power_total == 0)return;
	int abs_current_power = math::abs(state_.power_current);

	const math::vec2 bar_unit_extent = region.extent() / math::vector2{abs_power_total, 1}.as<float>();

	auto acq = mo_yanxi::ui::get_draw_acquirer(renderer_ref);
	using namespace graphic;

	const auto valid_color = (state_.power_total > 0 ? colors::pale_green : colors::aqua).copy().set_a(opacity);
	const auto charging_color = colors::power.copy().set_a(opacity);
	const auto invalid_color = colors::power.copy().mul(0.5f).mul_a(opacity);

	int i = 0;

	auto get_unused_color = [&, this]{
		return state_.power_total > 0 || i < state_.power_assigned ? invalid_color : colors::dark_gray;
	};

	for(;i < abs_current_power; ++i){

		draw::fill::rect_ortho(
			acq.get(),
			math::frect{tags::from_extent, region.src + bar_unit_extent.copy().mul(i, 0),
				bar_unit_extent}.shrink(2), valid_color
		);
	}

	if(i == abs_power_total){
		return;
	}

	auto charging_region = math::frect{tags::from_extent, region.src + bar_unit_extent.copy().scl(i, 0),
				bar_unit_extent.copy().mul(charge_smooth_, 1)};

	draw::fill::rect_ortho(
			acq.get(),
			charging_region.copy().shrink(1, 2).move_x(0.5),
			charging_color
		);

	draw::fill::rect_ortho(
			acq.get(),
			math::frect{charging_region.vert_10(), region.src + bar_unit_extent.copy().scl(i, 0) + bar_unit_extent}.shrink(1, 2).move_x(-0.5),
			get_unused_color()
		);

	++i;
	for(;i < abs_power_total; ++i){
		draw::fill::rect_ortho(
			acq.get(),
			math::frect{tags::from_extent, region.src + bar_unit_extent.copy().mul(i, 0),
				bar_unit_extent}.shrink(2),
			get_unused_color()
		);
	}
}
