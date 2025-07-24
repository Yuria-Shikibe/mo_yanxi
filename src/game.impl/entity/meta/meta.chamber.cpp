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
	template <
		std::derived_from<ecs::chamber::building> T,
		tuple_spec Tup = std::tuple<>,
		std::derived_from<meta::chamber::basic_chamber> C>
	T& add_build(
		const C& self,
		ecs::chamber::chamber_manifold& grid,
		math::point2 where,
		math::usize2 extent){
		using namespace mo_yanxi;
		using namespace game;
		auto& chunk = grid.add_building<tuple_cat_t<std::tuple<T>, Tup>>(ecs::chamber::tile_region{tags::from_extent, where, extent.as<int>()});
		ecs::chamber::building_data& data = chunk.template get<ecs::chamber::building_data>();
		if(std::derived_from<C, ecs::chamber::energy_status>){
			auto& s = static_cast<const ecs::chamber::energy_status&>(self);
			data.set_energy_status(s);
		}
		return chunk;
	}
}

mo_yanxi::game::meta::chamber::ui_build_handle mo_yanxi::game::meta::chamber::radar::radar_instance_data::build_editor_ui(ui::table& table, ui_edit_context& context){
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
	ecs::chamber::chamber_manifold& grid, math::point2 where) const{
	return add_build<
		ecs::chamber::power_generator_build, std::tuple<ecs::chamber::power_generator_building_tag>
	>(*this, grid, where, extent);
}

mo_yanxi::game::ecs::chamber::build_ptr mo_yanxi::game::meta::chamber::turret_base::create_instance_chamber(
	ecs::chamber::chamber_manifold& grid, math::point2 where) const{
	return add_build<ecs::chamber::turret_build, std::tuple<ecs::chamber::power_consumer_building_tag>>(*this, grid, where, extent);
}

void mo_yanxi::game::meta::chamber::radar::draw(math::frect region, graphic::renderer_ui_ref renderer_ui, const graphic::camera2& camera) const{
	basic_chamber::draw(region, renderer_ui, camera);

}

mo_yanxi::game::ecs::chamber::build_ptr mo_yanxi::game::meta::chamber::radar::create_instance_chamber(
	ecs::chamber::chamber_manifold& grid, math::point2 where) const{
	return add_build<ecs::chamber::radar_build, std::tuple<ecs::chamber::power_consumer_building_tag>>(*this, grid, where, extent);
}

void mo_yanxi::game::meta::chamber::radar::install(ecs::chamber::build_ref build_ref) const{
	basic_chamber::install(build_ref);

	using namespace ecs::chamber;
	auto& building = building_cast<radar_build>(build_ref);

	building.meta.reload_duration = reload_duration;
	building.meta.targeting_range_radius = this->targeting_range_radius;
	building.meta.targeting_range_angular = this->targeting_range_angular;

	building.meta.transform.vec = this->transform.vec * extent.as<float>() * tile_size;
	building.meta.transform.z_offset = this->transform.z_offset;
}

void mo_yanxi::game::meta::chamber::radar::radar_instance_data::install(ecs::chamber::build_ref build_ref){
	using namespace ecs::chamber;
	auto& building = building_cast<radar_build>(build_ref);

	building.meta.transform.rot = this->rotation;
}

void mo_yanxi::game::meta::chamber::radar::radar_instance_data::draw(
	const basic_chamber& meta,
	math::frect region,
	graphic::renderer_ui_ref renderer_ui,
	const float camera_scale) const{
	chamber_instance_data::draw(meta, region, renderer_ui, camera_scale);

	auto& m = static_cast<const radar&>(meta);

	auto acq = ui::get_draw_acquirer(renderer_ui);

	using namespace graphic;

	math::trans2 pos = {region.src + m.transform.vec * region.extent(), rotation};

	static constexpr color from_color = colors::dark_gray.copy().set_a(.5f);
	static constexpr color to_color = colors::pale_green;
	static constexpr color src_color = colors::red_dusted;

	float stk = 3 / math::max(camera_scale, 0.1f);

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
