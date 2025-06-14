module mo_yanxi.game.meta.chamber;

import mo_yanxi.ui.graphic;
import mo_yanxi.ui.elem.text_elem;
import mo_yanxi.ui.creation.field_edit;
import mo_yanxi.ui.assets;

void mo_yanxi::game::meta::chamber::basic_chamber::build_ui(ui::table& table) const{
	table.end_line().function_init([this](ui::label& l){
		l.set_fit();
		l.set_style();
		l.set_text(name);
		l.text_entire_align = align::pos::center;
	}).cell().set_height(40);


	for(int i = 0; i < 10; ++i){
		table.end_line().function_init([&, this](ui::label& l){
			l.set_fit();
			l.set_style();
			l.set_text(std::format("test {}", i));
		}).cell().set_height(30).pad.set_vert(4);

	}
}


mo_yanxi::game::meta::chamber::ui_build_handle mo_yanxi::game::meta::chamber::radar::radar_instance_data::build_ui(ui::table& table){
	co_yield true;

	table.end_line().function_init([this](ui::named_field_editor& l){
		l.get_editor().set_edit_target(ui::edit_target_range_constrained{ui::edit_target{&rotation, math::deg_to_rad, math::pi_2}, {0, math::pi_2}});
		l.get_unit_label().set_text("#<[#8FD7C0>Deg");
		l.get_name_label().set_text("Rotation");
		l.set_unit_label_size({60});
		l.set_editor_height(50);
		l.set_name_height(35);

		l.set_style(ui::theme::styles::side_bar_general);
	}).cell().set_external({false, true});
}


void mo_yanxi::game::meta::chamber::radar::draw(math::frect region, graphic::renderer_ui& renderer_ui, const graphic::camera2& camera) const{
	basic_chamber::draw(region, renderer_ui, camera);

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
	if(math::equal(ratio, 1)){
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
