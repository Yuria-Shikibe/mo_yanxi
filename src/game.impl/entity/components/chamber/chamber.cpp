module;

#include <cassert>

module mo_yanxi.game.ecs.component.chamber;

import mo_yanxi.ui.elem.label;
import mo_yanxi.ui.assets;
import mo_yanxi.ui.graphic;
import mo_yanxi.ui.creation.generic;
import mo_yanxi.math.fancy;


import mo_yanxi.game.ui.bars;
import mo_yanxi.strided_span;

namespace mo_yanxi::game::ecs::chamber{
	std::vector<tile> chamber_grid_fields::get_dst_sorted_tiles(this const chamber_grid_fields& grid,
		const math::frect_box& hitter_box_in_global, math::vec2 basepoint_in_global, math::nor_vec2 normal_in_global){
		const auto localBox = grid.box_to_local(hitter_box_in_global);

		const auto trans = grid.get_transform();
		math::vec2 pos{trans.apply_inv_to(basepoint_in_global)};
		const math::vec2 vel{normal_in_global.rotate_rad(-static_cast<float>(trans.rot))};



		std::vector<tile> under_hit;

		localBox.each_tile_within(tile_size, [&](tile_coord coord){
			if(auto tile = grid.local_grid.tiles.find(coord); tile && tile->building){
				auto dot = (tile->get_real_pos() - pos).dot(vel);
				if(dot < 0.0f){
					pos += vel * dot;
				}
				under_hit.push_back(*tile);
			}
		});

		std::ranges::sort(under_hit, pierce_comp{pos, vel}, &tile::get_real_pos);
		return under_hit;
	}

	chamber_manifold::chamber_manifold(const meta::chamber::grid& grid): chamber_grid_fields(grid){
		grid.dump(*this);

		using params = std::tuple<building_data, power_consumer_building_tag>;
		using span_tuple = unary_apply_to_tuple_t<strided_span, params>;

		manager.slice_and_then<params>([this](const span_tuple& p){
			energy_allocator.insert_acquisition(std::get<0>(p) | std::views::transform([](building_data& data){
				return std::addressof<building_data>(data);
			}));
		});
	}
}

void energy_bar_drawer(const mo_yanxi::ui::elem& e, const mo_yanxi::ui::rect& r, int consumption){
	using namespace mo_yanxi;
	auto acq = ui::get_draw_acquirer(e.get_renderer());
	using namespace graphic;

	// static constexpr float icon_width = 35.f;
	static constexpr float icon_pad = 6;

	const auto sz = e.content_size();
	const auto icon_region = align::embed_to(align::scale::fit, {1, 1}, sz);
	acq.proj.mode_flag = draw::mode_flags::sdf;

	acq << static_cast<combined_image_region<uniformed_rect_uv>>(consumption < 0 ? ui::theme::icons::minus : ui::theme::icons::plus);

	draw::fill::rect_ortho(acq.get(), ui::rect{tags::from_extent, e.content_src_pos(), icon_region}.expand(8), consumption < 0 ? colors::red_dusted : colors::pale_green);

	acq << draw::white_region;
	acq.proj.mode_flag = consumption < 0 ? draw::mode_flags::slide_line : draw::mode_flags{};

	const auto c = math::abs(consumption);
	const auto stride = math::clamp_positive(sz.x - icon_pad - icon_region.x) / c;
	static constexpr auto x_shrink = 2.f;
	const auto width = stride - x_shrink * 2;
	if(width < 8){
		if(stride > 0){
			draw::fill::rect_ortho(acq.get(),
			ui::rect{tags::from_extent, e.content_src_pos().add_x(icon_pad + icon_region.x), stride, sz.y},
			colors::power);
		}
	}else{
		for(unsigned i = 0; i < c; ++i){
			draw::fill::rect_ortho(acq.get(),
				ui::rect{tags::from_extent, e.content_src_pos().add_x(i * stride + x_shrink + icon_pad + icon_region.x), width, sz.y},
				colors::power);
		}
	}
}


void mo_yanxi::game::meta::chamber::basic_chamber::install(ecs::chamber::build_ref build_ref) const{
	using namespace ecs::chamber;
	auto& building = building_cast(build_ref);

	building.data().hit_point.reset(hit_point);
	if(auto cons = get_energy_consumption()){
		building.data().energy_status = *cons;
	}
}

void mo_yanxi::game::meta::chamber::basic_chamber::build_editor_ui(ui::table& table) const{
	table.end_line().function_init([this](ui::label& l){
		l.set_fit();
		l.set_style();
		l.set_text(name);
		l.text_entire_align = align::pos::center;
	}).cell().set_height(60);

	table.end_line().function_init([this](ui::label& l){
		l.set_fit();
		l.set_style();
		l.set_text(std::format("Hitpoint: {}", hit_point.max));
		l.text_entire_align = align::pos::center_left;
	}).cell().set_height(40);

	{
		auto hpbar = table.end_line().create(ui::creation::general_canvas{
				[this](const ui::elem& e, auto){
					const auto [from, to] = hit_point.capability_range;
					const auto src = e.content_src_pos();
					const auto [w, h] = e.content_size();
					const auto opacity = e.gprop().get_opacity();

					auto acq = ui::get_draw_acquirer(e.get_renderer());
					math::get_chunked_progress_region(
						{from, to},
						[&](auto idx, float f, float t){
							using namespace ui;
							using namespace graphic;
							float dst = t - f;
							float width = dst * w;
							if(width > std::numeric_limits<float>::epsilon()){

								color color = (colors::seq::gray_green_yellow | std::views::transform(&color::to_neutralize_light_def))[idx];
								color.set_a(opacity);

								draw::fill::rect_ortho(acq.get(), math::frect{
									                       tags::from_extent, src.copy().add_x(f * w),
									                       {width, h}
								                       }, color);
							}
						});
				}
			});
		hpbar->set_style();
		hpbar->interactivity = ui::interactivity::enabled;
		hpbar->set_tooltip_state(
			{
				.layout_info = {ui::tooltip_follow::cursor},
			}, [this](ui::table& t){
				t.set_style(ui::theme::styles::side_bar_standard);
				t.function_init([&](ui::label& l){
					l.set_style();
					l.set_scale(.7f);

					l.set_text(
						std::format(
							"[{:.0f}, {:.0f}]#<[_><[#8888899ff>[Min Enable, Max Enable]",
						hit_point.capability_range.from * hit_point.max, hit_point.capability_range.to * hit_point.max)
					);
				}).cell().set_external({true, true}).pad.set_vert(6);
			});
		hpbar.cell().set_height(24);
	}

	if(int consume = get_energy_usage(); consume != 0){
		auto canvas = table.end_line().create(ui::creation::general_canvas{std::bind_back(energy_bar_drawer, consume)});
		canvas->set_style();
		canvas.cell().set_height(40);
		canvas.cell().pad.top = 8;
		canvas->interactivity = ui::interactivity::enabled;
		canvas->set_tooltip_state({
			.layout_info = {ui::tooltip_follow::cursor},
		}, [consume](ui::table& t){
			t.set_style(ui::theme::styles::side_bar_general);
			t.property.graphic_data.style_color_scl = consume > 0 ? graphic::colors::pale_green : graphic::colors::red_dusted;
			t.function_init([&](ui::label& l){
				l.set_style();
				l.set_scale(.7f);

				l.set_text(
					std::format("#<{:[#a}>{}#<c> {}#<[_>Unit",
						consume > 0 ? graphic::colors::pale_green : graphic::colors::red_dusted,
						consume > 0 ? "Generate" : "Consume",
						math::abs(consume))
						);
			}).cell().set_external({true, true}).pad.set_vert(6);
		});
	}
}


mo_yanxi::game::ecs::chamber::build_ptr mo_yanxi::game::meta::chamber::basic_chamber::create_instance_chamber(
	ecs::chamber::chamber_manifold& grid, math::point2 where) const{
	grid.add_building<std::tuple<>>(ecs::chamber::tile_region{tags::from_extent, where, extent.as<int>()});
	return nullptr;
}




