module;


#include "../src/srl/srl.hpp"
#include <srl/game.meta.chamber.pb.h>

module mo_yanxi.game.meta.chamber;

import mo_yanxi.ui.graphic;
import mo_yanxi.ui.elem.label;
import mo_yanxi.ui.elem.canvas;
import mo_yanxi.ui.creation.field_edit;
import mo_yanxi.ui.creation.generic;
import mo_yanxi.ui.assets;
import mo_yanxi.math;
import mo_yanxi.math.fancy;


import mo_yanxi.game.ecs.component.chamber;
import mo_yanxi.game.ecs.component.chamber.radar;
import mo_yanxi.game.ecs.component.chamber.turret;
import mo_yanxi.game.ecs.component.chamber.power_generator;


namespace mo_yanxi::io{
	template <>
	struct loader_impl<game::meta::chamber::radar::radar_instance_data> : loader_base<pb::game::meta::chamber::radar_instance, game::meta::chamber::radar::radar_instance_data>{
		static void store(buffer_type& buf, const value_type& data){
			buf.set_rotation(data.rotation);
		}

		static void load(const buffer_type& buf, value_type& data){
			data.rotation = buf.rotation();
		}
	};
}

namespace mo_yanxi::game{
	template <std::derived_from<ecs::chamber::building> T, std::derived_from<meta::chamber::basic_chamber> C>
	T& add_build(
		const C& self,
		ecs::chamber::manifold_ref grid,
		math::point2 where,
		math::usize2 extent){
		using namespace mo_yanxi;
		using namespace game;
		auto& g = ecs::chamber::to_manifold(grid);
		auto& chunk = g.add_building<T>(ecs::chamber::tile_region{tags::from_extent, where, extent.as<int>()});
		ecs::chamber::building_data& data = chunk.template get<ecs::chamber::building_data>();
		if(std::derived_from<C, ecs::chamber::energy_status>){
			auto& s = static_cast<const ecs::chamber::energy_status&>(self);
			data.set_energy_status(s);
		}
		return chunk;
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
	auto& building = to_building(build_ref);

	building.data().hit_point.reset(hit_point);
	if(auto cons = get_energy_consumption()){
		building.data().energy_status = *cons;
	}
}

void mo_yanxi::game::meta::chamber::basic_chamber::build_ui(ui::table& table) const{
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
					const auto [from, to] = hit_point.capability_range / hit_point.max;
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
						hit_point.capability_range.from, hit_point.capability_range.to)
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
	ecs::chamber::manifold_ref grid, math::point2 where) const{
	auto& g = ecs::chamber::to_manifold(grid);
	g.add_building<std::tuple<>>(ecs::chamber::tile_region{tags::from_extent, where, extent.as<int>()});
	return nullptr;
}


mo_yanxi::game::meta::chamber::ui_build_handle mo_yanxi::game::meta::chamber::radar::radar_instance_data::build_ui(ui::table& table, ui_edit_context& context){
	co_yield true;

	table.end_line().function_init([this](ui::named_field_editor& l){
		l.get_editor().set_edit_target(ui::edit_target_range_constrained{ui::edit_target{&rotation, math::deg_to_rad, math::pi_2}, {0, math::pi_2}});
		l.get_unit_label().set_text("#<[#8FD7C0>Deg");
		l.get_name_label().set_text("Rotation");
		l.set_unit_label_size({60});
		l.set_editor_height(50);
		l.set_name_height(35);

		l.set_style(ui::theme::styles::side_bar_whisper);
	}).cell().set_external({false, true});
}

mo_yanxi::game::srl::chunk_serialize_handle mo_yanxi::game::meta::chamber::radar::radar_instance_data::write(
	std::ostream& stream) const{
	const auto msg = io::pack(*this);
	co_yield msg.ByteSizeLong();
	msg.SerializeToOstream(&stream);
}

void mo_yanxi::game::meta::chamber::radar::radar_instance_data::read(std::ispanstream& bounded_stream){
	chamber_instance_data::read(bounded_stream);
	io::loader<radar_instance_data>::parse_from(bounded_stream, *this);
}


mo_yanxi::game::ecs::chamber::build_ptr mo_yanxi::game::meta::chamber::energy_generator::create_instance_chamber(
	ecs::chamber::manifold_ref grid, math::point2 where) const{
	return add_build<ecs::chamber::power_generator_build>(*this, grid, where, extent);
}

mo_yanxi::game::ecs::chamber::build_ptr mo_yanxi::game::meta::chamber::turret_base::create_instance_chamber(
	ecs::chamber::manifold_ref grid, math::point2 where) const{
	return add_build<ecs::chamber::turret_build>(*this, grid, where, extent);
}

void mo_yanxi::game::meta::chamber::radar::draw(math::frect region, graphic::renderer_ui& renderer_ui, const graphic::camera2& camera) const{
	basic_chamber::draw(region, renderer_ui, camera);

}

mo_yanxi::game::ecs::chamber::build_ptr mo_yanxi::game::meta::chamber::radar::create_instance_chamber(
	ecs::chamber::manifold_ref grid, math::point2 where) const{
	return add_build<ecs::chamber::radar_build>(*this, grid, where, extent);
}

void mo_yanxi::game::meta::chamber::radar::install(ecs::chamber::build_ref build_ref) const{
	basic_chamber::install(build_ref);

	using namespace ecs::chamber;
	auto& building = to_building<radar_build>(build_ref);

	building.meta.reload_duration = reload_duration;
}

void mo_yanxi::game::meta::chamber::radar::radar_instance_data::install(ecs::chamber::build_ref build_ref){
	using namespace ecs::chamber;
	auto& building = to_building<radar_build>(build_ref);
}

void mo_yanxi::game::meta::chamber::radar::radar_instance_data::draw(const basic_chamber& meta, math::frect region,
                                                                     graphic::renderer_ui& renderer_ui, const graphic::camera2& camera) const{
	chamber_instance_data::draw(meta, region, renderer_ui, camera);

	auto& m = static_cast<const radar&>(meta);

	auto acq = ui::get_draw_acquirer(renderer_ui);

	using namespace graphic;

	math::trans2 pos = {region.src + m.transform.vec * region.size(), rotation};

	static constexpr color from_color = colors::dark_gray.copy().set_a(.5f);
	static constexpr color to_color = colors::pale_green;
	static constexpr color src_color = colors::red_dusted;

	float stk = 3 / math::max(camera.get_scale(), 0.1f);

	float ratio = m.targeting_range_angular.length() / math::pi_2;
	if(math::equal(ratio, 1.f)){
		draw::line::circle(acq, pos.vec, m.targeting_range_radius.from, stk, from_color, from_color);
		draw::line::circle(acq, pos.vec, m.targeting_range_radius.to, stk, to_color, to_color);
	}else{
		auto p1 = pos;
		p1.rot += m.targeting_range_angular.from;

		auto p2 = pos;
		p2.rot += m.targeting_range_angular.to;

		draw::line::line<false>(acq.get(), math::vec2{m.targeting_range_radius.from} | p1, math::vec2{m.targeting_range_radius.to} | p1, stk, from_color, to_color);
		draw::line::line<false>(acq.get(), math::vec2{m.targeting_range_radius.from} | p2, math::vec2{m.targeting_range_radius.to} | p2, stk, from_color, to_color);
		draw::line::circle_partial(acq, p1, m.targeting_range_radius.from, ratio, stk, from_color);
		draw::line::circle_partial(acq, p1, m.targeting_range_radius.to, ratio, stk, to_color);
		draw::line::line<false>(acq.get(), math::vec2{m.targeting_range_radius.from} | p1, pos.vec, stk, from_color, src_color);
		draw::line::line<false>(acq.get(), math::vec2{m.targeting_range_radius.from} | p2, pos.vec, stk, from_color, src_color);
	}
}
