module;

#include "../src/srl/srl.game.hpp"

module mo_yanxi.game.ui.hitbox_editor;

import mo_yanxi.core.global.assets;
import mo_yanxi.basic_util;

void mo_yanxi::game::ui::hitbox_editor::write_to(const std::filesystem::path& path){
	auto metas = viewport->channel_hitbox.export_to_meta();
	std::ofstream stream(path, std::ios::out | std::ios::binary);
	io::loader<meta::hitbox_transed>::serialize_to(stream, metas);
}

void mo_yanxi::game::ui::hitbox_editor::load_from(const std::filesystem::path& path){
	meta::hitbox_transed meta{};
	std::ifstream stream(path, std::ios::in | std::ios::binary);
	io::loader<meta::hitbox_transed>::parse_from(stream, meta);
	viewport->channel_hitbox.clear();
	viewport->channel_hitbox.add_comp(meta.components | std::views::transform([](const meta::hitbox::comp& a){
		return box_wrapper{{a}};
	}), false);
	viewport->channel_hitbox.origin_trans = meta.trans;
}


void mo_yanxi::game::ui::hitbox_editor::build_menu(){
	menu->clear_children();
	menu->template_cell.set_external({false, true}).set_pad({.bottom = 8});

	{
		auto box = menu->end_line().emplace<button<icon_frame>>();
		box.cell().set_height(60);
		box->set_style(ui::theme::styles::no_edge);
		box->set_drawable(ui::theme::icons::blender_icon_pivot_cursor);
		box->set_tooltip_state(
			{
				.layout_info = tooltip_layout_info{
					.follow = tooltip_follow::owner,
					.align_owner = align::pos::top_right,
					.align_tooltip = align::pos::top_left,
				},
				.use_stagnate_time = false,
				.auto_release = false,
				.min_hover_time = tooltip_create_info::disable_auto_tooltip
			}, [this](ui::table& table){
				table.prop().size.set_minimum_size({400, 0});
				table.set_entire_align(align::pos::top_left);
				table.template_cell.set_external({true, true});
				table.template_cell.pad.top = 12;

				table.end_line().function_init([this](label& area){
					area.set_style();
					area.set_scale(.6f);
					area.set_text("x  ");
				});
				table.function_init([this](numeric_input_area& area){
					area.set_style();
					area.set_scale(.6f);
					area.set_target(viewport->channel_hitbox.origin_trans.vec.x);
				});

				table.end_line().function_init([this](label& area){
					area.set_style();
					area.set_scale(.6f);
					area.set_text("y  ");
				});
				table.function_init([this](numeric_input_area& area){
					area.set_style();
					area.set_scale(.6f);
					area.set_target(viewport->channel_hitbox.origin_trans.vec.y);
				});

				table.end_line().function_init([this](label& area){
					area.set_style();
					area.set_scale(.6f);
					area.set_text("rot");
				});
				table.function_init([this](numeric_input_area& area){
					area.set_style();
					area.set_scale(.6f);
					area.set_target(ui::edit_target{
							&viewport->channel_hitbox.origin_trans.rot, math::deg_to_rad_v<float>, math::pi_2
						});
				});


				table.end_line().function_init([this](label& area){
					area.set_style();
					area.set_scale(.6f);
					area.set_text("scl");
				});
				table.function_init([this](numeric_input_area_with_callback& area){
					area.set_style();
					area.set_scale(.6f);
					area.set_target(
						ui::edit_target_range_constrained<float>{
							{&viewport->channel_reference_image.scale},
							{std::numeric_limits<float>::epsilon(), 2}
						});

					area.set_callback([this]{
						viewport->channel_reference_image.update_scale(viewport->channel_reference_image.scale);
					});
				});


				table.set_edge_pad(0);
			});
		box->set_button_callback_build_tooltip();

		origin_point_modify = std::to_address(box);
	}

	{
		auto box = menu->end_line().emplace<ui::check_box>();
		box.cell().set_height(60);
		box->set_style(ui::theme::styles::no_edge);
		box->set_drawable<ui::icon_drawable>(0, ui::theme::icons::blender_icon_pivot_individual);
		box->set_drawable<ui::icon_drawable>(1, ui::theme::icons::blender_icon_pivot_median);
		box->set_drawable<ui::icon_drawable>(2, ui::theme::icons::blender_icon_pivot_active);
		box->add_multi_select_tooltip({
				.follow = tooltip_follow::owner,
				.align_owner = align::pos::top_right,
				.align_tooltip = align::pos::top_left,
			});

		checkbox = std::to_address(box);
	}


	{
		auto b = menu->end_line().emplace<ui::button<ui::label>>();

		b->set_style(ui::theme::styles::no_edge);
		b->set_scale(.6f);
		b->set_text("save");
		b->set_button_callback(ui::button_tags::general, [this]{
			auto& selector = creation::create_file_selector(creation::file_selector_create_info{
					.requester = *this,
					.checker = [](const creation::file_selector& s, const ui::hitbox_editor&){
						return s.get_current_main_select().has_value();
					},
					.yielder = [](const creation::file_selector& s, ui::hitbox_editor& self){
						self.write_to(s.get_current_main_select().value());
						return true;
					},
					.add_file_create_button = true
				});
			selector.set_cared_suffix({".hbox"});
		});
	}


	{
		auto b = menu->end_line().emplace<ui::button<ui::label>>();

		b->set_style(ui::theme::styles::no_edge);
		b->set_scale(.6f);
		b->set_text("load");
		b->set_button_callback(ui::button_tags::general, [this]{
			auto& selector = creation::create_file_selector(creation::file_selector_create_info{
				*this, [](const creation::file_selector& s, const ui::hitbox_editor&){
					return s.get_current_main_select().has_value();
				}, [](const creation::file_selector& s, ui::hitbox_editor& self){
					self.load_from(s.get_current_main_select().value());
					return true;
				}});
			selector.set_cared_suffix({".hbox"});
		});
	}

	{
		auto b = menu->end_line().emplace<ui::button<ui::label>>();

		b->set_style(ui::theme::styles::no_edge);
		b->set_scale(.6f);
		b->set_text("set ref");
		b->set_button_callback(ui::button_tags::general, [this]{
			auto& selector = creation::create_file_selector(creation::file_selector_create_info{
				*this, [](const creation::file_selector& s, const ui::hitbox_editor&){
					return s.get_current_main_select().has_value();
				}, [](const creation::file_selector& s, ui::hitbox_editor& self){
					self.set_image_ref(s.get_current_main_select().value());
					return true;
				}});
			selector.set_cared_suffix({".png"});
		});
	}
}

void mo_yanxi::game::ui::hitbox_editor::set_image_ref(const std::filesystem::path& path){
	auto region = core::global::assets::atlas[graphic::image_page::name_temp].async_allocate(
		{graphic::path_load{path.string()}}
	);

	viewport->channel_reference_image.set_region(std::move(region));
}
