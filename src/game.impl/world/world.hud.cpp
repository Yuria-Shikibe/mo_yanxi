module;

module mo_yanxi.game.world.hud;

import mo_yanxi.core.global.ui;
import mo_yanxi.core.global.graphic;

import mo_yanxi.ui.elem.button;
import mo_yanxi.ui.elem.scroll_pane;
import mo_yanxi.ui.elem.check_box;
import mo_yanxi.ui.creation.file_selector;
import mo_yanxi.ui.graphic;
import mo_yanxi.ui.assets;

import mo_yanxi.game.ecs.component.chamber;
import mo_yanxi.game.ecs.component.manifold;
import mo_yanxi.game.ecs.component.command;
import mo_yanxi.game.ecs.component.ui.builder;

import mo_yanxi.basic_util;

namespace mo_yanxi::game{
	void draw_path(
		graphic::renderer_ui_ref renderer,
		math::trans2 cur_pos,
		std::optional<math::trans2> next_pos,
		const path_seq& current_path,
		double time_in_sec,
		graphic::color path_color = graphic::colors::pale_green.to_neutralize_light_def().set_a(.75f)
		){
		if(current_path.empty())return;
		auto draw_acquirer = ui::get_draw_acquirer(renderer);
		using namespace graphic;


		constexpr float line_stroke = 4;

		if(next_pos){
			draw::line::line(draw_acquirer.get(), cur_pos.vec, next_pos->vec, line_stroke, {}, path_color);
		}

		for (const auto & [l1, l2] : current_path.to_span() | std::views::adjacent<2>){
			draw::line::line(draw_acquirer.get(), l1.vec, l2.vec,
			                 line_stroke, path_color, path_color);
		}

		auto time = time_in_sec;
		constexpr auto cycle = [](double time, double period, double valid_period, double margin) static noexcept {
			auto t = std::fmod(time, period);
			return (t - margin) / valid_period;
		};

		constexpr double offset = .5;
		constexpr double cycles = 2.75f;
		constexpr double valid_cycles = 1.5;
		constexpr double margin = offset;

		const auto t1_cycle = math::clamp(cycle(time, cycles, valid_cycles, margin));
		const auto t2_cycle = math::clamp(cycle(time + offset, cycles, valid_cycles, margin));

		if(t2_cycle >= t1_cycle){
			if(next_pos){
				draw::line::line(
					draw_acquirer.get(),
					math::lerp(cur_pos.vec, next_pos->vec, t1_cycle),
					math::lerp(cur_pos.vec, next_pos->vec, t2_cycle),
					line_stroke, colors::white_clear, colors::white);
			}

			for (const auto & [l1, l2] : current_path.to_span() | std::views::adjacent<2>){
				draw::line::line(
					draw_acquirer.get(),
					math::lerp(l1.vec, l2.vec, t1_cycle),
					math::lerp(l1.vec, l2.vec, t2_cycle),
					line_stroke, colors::white_clear, colors::white);
			}
		}

		for (const auto & p : current_path.to_span()){
			draw::fill::rect_ortho(draw_acquirer.get(), math::frect{p.vec, line_stroke * 4}, path_color);
		}

		if(const auto cur = current_path.next()){
			draw::line::square(draw_acquirer, {cur->vec, 45 * math::deg_to_rad}, 32, line_stroke * 2, ui::theme::colors::accent);
		}
	}
}

void mo_yanxi::game::world::hud::path_editor::active(const ecs::entity_ref& target) noexcept{
	this->target = target;
	if(!target)return;

	const auto mcmd = target->try_get<ecs::move_command>();
	if(!mcmd) return;

	if(auto p = mcmd->route.get_if<path_seq>()){
		current_path = *p;
	}

}

void mo_yanxi::game::world::hud::path_editor::apply() noexcept{
	if(!*this)return;
	if(auto cmd = target->try_get<ecs::move_command>()){
		current_path.set_current_index(1);
		cmd->route = std::move(current_path);
		confirm_animation_progress_ = 0;
	}
}

void mo_yanxi::game::world::hud::path_editor::on_key_input(core::ctrl::key_pack key_pack) noexcept{
	using namespace core::ctrl;
	const auto [k, a, m] = key_pack.unpack();
	switch(m){


	case mode::none:
		if(a == act::press || a == act::repeat){
			switch(k){
			case key::Left: current_path.to_prev(); break;
			case key::Right: current_path.to_next(); break;
			default: break;
			}
		}

		if(a == act::release){
			switch(k){
			case key::Delete: current_path.erase_current(); break;
			case key::Backspace: current_path.erase_current_and_to_prev(); break;
			case key::Enter: apply(); break;
			default: break;
			}
		}

	case mode::shift:
	default: break;
	}
}

void mo_yanxi::game::world::hud::path_editor::on_click(ui::input_event::click click){
	const auto mcmd = target->try_get<ecs::move_command>();
	if(!mcmd) return;

	using namespace core::ctrl;
    if(click.code.matches(rmb_click_no_mode)){

    	current_path.insert_after_current({click.pos});
    	current_path.to_next();
    }else if(click.code.matches({mouse::RMB, act::release, mode::ctrl})){
	    if(auto pos = current_path.current()){
	    	pos->vec = click.pos;
	    }
    }
}

void mo_yanxi::game::world::hud::path_editor::draw(const ui::elem& elem, graphic::renderer_ui_ref renderer) const{


	auto draw_acquirer = ui::get_draw_acquirer(renderer);
	using namespace graphic;

	constexpr float line_stroke = 4;
	constexpr auto path_color = colors::pale_green.to_neutralize_light_def().set_a(.75f);

	auto mm = target->try_get<ecs::mech_motion>();
	if(!mm)return;
	auto entTrans = mm->trans;

	if(!current_path.empty())draw_path(renderer, entTrans, current_path[0], current_path, elem.get_scene()->get_global_time());

	if(auto mcmd = target->try_get<ecs::move_command>(); mcmd->route.holds<path_seq>()){
		auto& path = *mcmd->route.get_if<path_seq>();
		if(confirm_animation_progress_ > 0){
			if(!path.empty()){
				auto prog = math::curve(confirm_animation_progress_, 0.f, confirm_animation_progress_duration * 0.5f);
				auto alpha = 1 - math::curve(confirm_animation_progress_,
				                                   confirm_animation_progress_duration * 0.4f,
				                                   confirm_animation_progress_duration);
				auto color = colors::pale_green.copy().set_a(alpha);
				draw::line::line(
					draw_acquirer.get(),
					entTrans.vec,
					math::lerp(entTrans.vec, path[0].vec, prog),
					line_stroke, colors::white_clear.create_lerp(color, prog), color);

				for(const auto& [l1, l2] : path.to_span() | std::views::adjacent<2>){
					draw::line::line(
						draw_acquirer.get(),
						l1.vec,
						math::lerp(l1.vec, l2.vec, prog),
						line_stroke, colors::white_clear.create_lerp(color, prog), color);
				}
			}
		}

		auto prog = confirm_animation_progress_ > 0 ? math::curve(confirm_animation_progress_,
												   confirm_animation_progress_duration * 0.4f,
												   confirm_animation_progress_duration) : 1;

		std::optional<mo_yanxi::math::trans2> trs{};
		if(auto p = path.get_current())trs.emplace(*p);
		draw_path(renderer, entTrans, trs, path, elem.get_scene()->get_global_time(), graphic::colors::aqua.to_neutralize_light_def().set_a(prog * .75f));


	}
}

void mo_yanxi::game::world::hud::hud_viewport::build_hud(){
	auto& bed = *this;

	{
		auto pane = bed.emplace<ui::scroll_pane>();
		side_bar = &pane->set_elem([](ui::table& t){
			t.set_style();
			t.interactivity = ui::interactivity::enabled;

			t.template_cell.pad.bottom = 8;
			t.template_cell.set_external({false, true});
		});
		pane->set_style(ui::theme::styles::general_static);
		pane.cell().region_scale = {tags::from_extent, {}, {0.2, 1.}};
		pane.cell().margin.set(4);
		pane.cell().align = align::pos::center_left;
	}
	{
		auto pane = bed.emplace<ui::scroll_pane>();
		pane->set_style(ui::theme::styles::general_static);
		pane->set_layout_policy(ui::layout_policy::vert_major);
		pane.cell().region_scale = {tags::from_extent, {}, {0.3, .2}};
		pane.cell().margin.set(4);
		pane.cell().align = align::pos::bottom_right;

		button_bar = &pane->set_elem([](ui::table& t){
			t.set_style();
			t.interactivity = ui::interactivity::enabled;

			auto box = t.emplace<ui::check_box>();
			box->set_drawable<ui::icon_drawable>(0, ui::theme::icons::close);
			box->set_drawable<ui::icon_drawable>(1, ui::theme::icons::up);
			// box->add_multi_select_tooltip({
			//
			// });
			// box.cell().set_width_from_scale();
			box.cell().set_width(80);
		});
	}

}

void mo_yanxi::game::world::hud::hud_viewport::build_entity_info(){
	side_bar->clear_children();

	if(main_selected){
		if(const auto b = main_selected->try_get<ecs::ui_builder>()){
			b->build_hud(side_bar->end_line(), main_selected);
		}

		if(const auto move_cmd = main_selected->try_get<ecs::move_command>()){
			path_editor_.active(main_selected);
		}
	}
}

void mo_yanxi::game::world::hud::hud_viewport::draw_content(const ui::rect clipSpace) const{
	using namespace graphic;
	auto draw_acquirer = ui::get_draw_acquirer(get_renderer());

	draw::line::square(draw_acquirer, {get_scene()->get_cursor_pos(), 45.f * math::deg_to_rad}, 32, 2, ui::theme::colors::theme);

	auto& renderer = renderer_from_erased(get_renderer());

	renderer.batch.push_projection(context.graphic_context->renderer().camera.get_world_to_uniformed());

	if(main_selected){

		if(auto building = main_selected->try_get<ecs::chamber::chamber_manifold>()){
			building->draw_hud(renderer);
		}

		if(auto manifold = main_selected->try_get<ecs::manifold>()){
			draw::line::rect_ortho(draw_acquirer, manifold->hitbox.max_wrap_bound(), 4, ui::theme::colors::theme);
		}

		if(path_editor_){
			path_editor_.draw(*this, get_renderer());
		}
	}

	renderer.batch.pop_projection();
	renderer.batch.blit_viewport(prop().content_bound_absolute());


	manual_table::draw_content(clipSpace);
}

mo_yanxi::ui::input_event::click_result mo_yanxi::game::world::hud::hud_viewport::on_click(
	const ui::input_event::click click_event){

	auto transed = get_transed(click_event.pos);
	using namespace core::ctrl;
	if(click_event.code.matches({mouse::LMB, act::press, mode::ctrl})){

		ecs::entity_id hit{};
		context.collision_system->quad_tree()->intersect_then(transed, [&, this](math::vec2 pos, const ecs::collision_object& obj){
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

	if(path_editor_){
		auto e = click_event;
		e.pos = transed;
		path_editor_.on_click(e);
	}

	return ui::input_event::click_result::intercepted;
}

void mo_yanxi::game::world::hud::hud_viewport::on_key_input(const core::ctrl::key_code_t key,
	const core::ctrl::key_code_t action, const core::ctrl::key_code_t mode){
	if(path_editor_){
		path_editor_.on_key_input({key, action, mode});
	}
}

mo_yanxi::math::vec2 mo_yanxi::game::world::hud::hud_viewport::get_transed(math::vec2 screen_pos) const noexcept{
	return context.graphic_context->renderer().camera.get_screen_to_world(screen_pos, {}, true);
}

mo_yanxi::game::world::hud::hud(){
	auto [add_scene, root] = core::global::ui::root->add_scene<ui::loose_group>(
		"hud",
		math::frect{math::vector2{core::global::graphic::context.get_extent().width, core::global::graphic::context.get_extent().height}.as<float>()},
		core::global::graphic::ui);

	this->scene = std::addressof(add_scene);

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
