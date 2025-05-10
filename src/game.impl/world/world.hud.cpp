module;

#include <gch/small_vector.hpp>

module mo_yanxi.game.world.hud;

import mo_yanxi.core.global.ui;
import mo_yanxi.core.global.graphic;

import mo_yanxi.ui.elem.button;
import mo_yanxi.ui.elem.scroll_pane;
import mo_yanxi.ui.creation.file_selector;
import mo_yanxi.ui.graphic;
import mo_yanxi.ui.assets;

import mo_yanxi.game.ecs.component.manifold;

void mo_yanxi::game::world::hud::hud_viewport::build_hud(){
	auto& bed = *this;

	auto pane = bed.emplace<ui::scroll_pane>();
	side_bar = &pane->set_elem([](ui::table& t){
		t.set_style();
		t.template_cell.pad.bottom = 8;
		t.template_cell.set_external({false, true});
	});
	pane.cell().region_scale = {tags::from_extent, {}, {0.3, 1.}};
	pane.cell().margin.set(4);
	pane.cell().align = align::pos::center_left;

}

void mo_yanxi::game::world::hud::hud_viewport::build_entity_info(){
	side_bar->clear_children();

	if(main_selected)if(const auto b = main_selected->try_get<ecs::ui_builder>()){
		b->build_hud(side_bar->end_line(), main_selected);
	}
}

void mo_yanxi::game::world::hud::hud_viewport::draw_content(const ui::rect clipSpace) const{
	using namespace graphic;
	ui::draw_acquirer draw_acquirer{get_renderer().batch, draw::white_region};

	draw::line::square(draw_acquirer, {get_scene()->get_cursor_pos(), 45.f * math::deg_to_rad}, 32, 2, ui::theme::colors::theme);

	get_renderer().batch.push_projection(context.graphic_context->renderer().camera.get_world_to_uniformed());

	if(main_selected){
		if(auto manifold = main_selected->try_get<ecs::manifold>()){
			draw::line::rect_ortho(draw_acquirer, manifold->hitbox.max_wrap_bound(), 4, ui::theme::colors::theme);
		}
	}

	get_renderer().batch.pop_projection();
	get_renderer().batch.blit_viewport(get_scene()->region);


	manual_table::draw_content(clipSpace);
}

mo_yanxi::ui::events::click_result mo_yanxi::game::world::hud::hud_viewport::on_click(
	const ui::events::click click_event){

	if(click_event.code.action() == core::ctrl::act::press){
		auto transed = context.graphic_context->renderer().camera.get_screen_to_world(click_event.pos, {}, true);

		ecs::entity_id hit{};
		context.collision_system->quad_tree().intersect_then(transed, [&, this](math::vec2 pos, const ecs::collision_object& obj){
			hit = obj.id;
		});

		if(hit != main_selected){
			main_selected = hit;
			build_entity_info();
		}else{
			main_selected = nullptr;
			side_bar->clear_children();
		}
	}

	return ui::events::click_result::intercepted;
}

mo_yanxi::game::world::hud::hud(){
	scene = &core::global::ui::root->add_scene(ui::scene{"hud", new ui::loose_group{nullptr, nullptr}, &core::global::graphic::ui});
	core::global::ui::root->resize(math::frect{math::vector2{core::global::graphic::context.get_extent().width, core::global::graphic::context.get_extent().height}.as<float>()}, "hud");
	auto& root = core::global::ui::root->root_of<ui::loose_group>("hud");

	ui::elem_ptr e{root.get_scene(), &root, std::in_place_type<hud_viewport>};
	e->set_style();
	e->property.fill_parent = {true, true};
	auto& bed = static_cast<hud_viewport&>(root.add_children(std::move(e)));
	viewport = &bed;
	viewport->hud = this;
}

mo_yanxi::game::world::hud::~hud(){
	core::global::ui::root->erase_scene("hud");
}

void mo_yanxi::game::world::hud::focus_hud(){
	core::global::ui::root->switch_scene_to("hud");
}
