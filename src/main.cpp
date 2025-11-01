#ifndef __AVX2__
#error "Requires __AVX2__"
#endif

#include <vulkan/vulkan.h>
// #include <cassert>

// #define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
// #include <freetype/freetype.h>

#include "../src/srl/srl.hpp"
#include "../src/srl/srl.game.hpp"
//
import mo_yanxi.graphic.bitmap;

import mo_yanxi.math;
import mo_yanxi.math.angle;
import mo_yanxi.math.matrix3;
import mo_yanxi.math.vector2;
import mo_yanxi.math.vector4;
import mo_yanxi.math.rand;
import mo_yanxi.math.quad;
import mo_yanxi.math.constrained_system;

import mo_yanxi.vk;

import mo_yanxi.assets.graphic;
import mo_yanxi.assets.directories;
import mo_yanxi.assets.ctrl;
import mo_yanxi.graphic.shaderc;
import mo_yanxi.graphic.color;

import mo_yanxi.core.window;
import mo_yanxi.allocator_2D;
import mo_yanxi.algo;

import mo_yanxi.graphic.camera;
import mo_yanxi.core.global;
import mo_yanxi.core.global.ui;
import mo_yanxi.core.global.graphic;
import mo_yanxi.core.global.assets;

import mo_yanxi.graphic.renderer;
import mo_yanxi.graphic.renderer.world;
import mo_yanxi.graphic.renderer.ui;
import mo_yanxi.graphic.draw.func;

import mo_yanxi.graphic.draw.multi_region;
import mo_yanxi.graphic.image_manage;
import mo_yanxi.graphic.image_multi_region;

import mo_yanxi.graphic.layers.ui.grid_drawer;

import mo_yanxi.font.manager;
//
import mo_yanxi.ui.root;
import mo_yanxi.ui.primitives;
import mo_yanxi.ui.elem.manual_table;
import mo_yanxi.ui.elem.table;
import mo_yanxi.ui.elem.list;
import mo_yanxi.ui.elem.label;
import mo_yanxi.ui.elem.scroll_pane;
import mo_yanxi.ui.elem.text_input_area;
import mo_yanxi.ui.elem.slider;
import mo_yanxi.ui.elem.image_frame;
import mo_yanxi.ui.elem.progress_bar;
import mo_yanxi.ui.elem.collapser;
import mo_yanxi.ui.elem.button;
import mo_yanxi.ui.elem.check_box;
import mo_yanxi.ui.elem.nested_scene;
import mo_yanxi.ui.creation.file_selector;
import mo_yanxi.ui.assets;
import mo_yanxi.ui.menu;
import mo_yanxi.ui.creation.field_edit;

import mo_yanxi.game.graphic.effect;
import mo_yanxi.game.world.graphic;
import mo_yanxi.game.world.hud;
import mo_yanxi.game.ecs.world.top;

import mo_yanxi.game.ecs.component.manage;

import mo_yanxi.game.ecs.component_operation_task_graph;
import mo_yanxi.game.ecs.component.manifold;
import mo_yanxi.game.ecs.component.hitbox;
import mo_yanxi.game.ecs.component.chamber;
import mo_yanxi.game.ecs.component.physical_property;
import mo_yanxi.game.ecs.component.projectile.manifold;

import mo_yanxi.game.ecs.system.collision;
import mo_yanxi.game.ecs.system.motion_system;
import mo_yanxi.game.ecs.system.renderer;
import mo_yanxi.game.ecs.system.projectile;
import mo_yanxi.game.ecs.system.chamber;

import mo_yanxi.game.ecs.entitiy_decleration;
import mo_yanxi.game.ecs.util;

import mo_yanxi.game.ecs.component.chamber.radar;
import mo_yanxi.game.ecs.component.chamber.turret;

import mo_yanxi.game.meta.instancing;
import mo_yanxi.game.content;
//

import mo_yanxi.game.ui.hitbox_editor;
import mo_yanxi.game.ui.grid_editor;
import mo_yanxi.game.content;
import mo_yanxi.game.meta.grid.srl;
import mo_yanxi.game.graphic.lightning;

import mo_yanxi.graphic.render_graph.manager;
import mo_yanxi.graphic.render_graph.fill;
import mo_yanxi.graphic.render_graph.volume;
import mo_yanxi.graphic.post_processor.oit_blender;
import mo_yanxi.game.graphic.render_graph_spec;

import mo_yanxi.graphic.draw.instruction;
import mo_yanxi.slide_window_buf;


import prototype.renderer.ui;
import mo_yanxi.gui.infrastructure;
import mo_yanxi.gui.infrastructure.group;
import mo_yanxi.gui.global;

import test;
import std;


#pragma region MAIN

#if 1
#pragma region UI_INIT
void init_ui(mo_yanxi::ui::loose_group& root, mo_yanxi::graphic::image_atlas& atlas){
	using namespace std::literals;
	using namespace mo_yanxi;

	root.property.set_empty_drawer();
	root.skip_inbound_capture = true;

	ui::elem_ptr e{root.get_scene(), &root, std::in_place_type<ui::manual_table>};
	e->property.set_empty_drawer();
	e->property.fill_parent = {true, true};
	e->skip_inbound_capture = true;
	auto& bed = static_cast<ui::manual_table&>(root.add_children(std::move(e)));

	using namespace math;

	// {
	// 	auto pane = bed.emplace<ui::button<>>();
	// 	pane->set_button_callback(ui::button_tags::general, [](ui::elem& elem){
	// 		using namespace ui;
	// 		auto& selector = creation::create_file_selector(creation::file_selector_create_info{elem, [](const creation::file_selector& s, const ui::elem&){
	// 			return s.get_current_main_select().has_value();
	// 		}, [](const creation::file_selector& s, const ui::elem&){
	// 			return true;
	// 		}, true});
	// 		selector.set_cared_suffix({".png"});
	// 	});
	// 	pane.cell().region_scale = {tags::from_extent, math::vec2{}, math::vec2{.1f, 1.f}};
	// 	pane.cell().align = align::pos::center_left;
	// 	pane.cell().margin.set(4);
	// }

	// {
	// 	auto pane = bed.emplace<ui::menu>();
	// 	pane.cell().region_scale = {tags::from_extent, math::vec2{}, math::vec2{.9f, 1.f}};
	// 	pane.cell().align = align::pos::center_right;
	// 	pane.cell().margin.set(4);
	//
	// 	for(int i = 0; i < 10; ++i){
	// 		auto rst = pane->add_elem<ui::label, ui::label>();
	// 		rst.button->set_scale(.5f);
	// 		rst.button->set_text(std::format("menu {}", i));
	// 		rst.button->text_entire_align = align::pos::center_left;
	// 		rst.button.cell().set_width(600);
	//
	// 		rst.elem.set_scale(.5f);
	// 		rst.elem.set_text(std::format("content: {}", i));
	// 		rst.elem.text_entire_align = align::pos::center;
	//
	// 	}
	//
	// }
	{
		auto pane = bed.emplace<game::ui::grid_editor>();
		pane.cell().region_scale = {tags::from_extent, math::vec2{}, math::vec2{1.f, 1.f}};
		pane.cell().align = align::pos::top_left;
		pane.cell().margin.set(4);
	}

	if(false){
		auto pane = bed.emplace<ui::scroll_pane>();
		pane.cell().region_scale = {tags::from_extent, math::vec2{}, math::vec2{1.f, .25f}};
		pane.cell().align = align::pos::bottom_left;
		pane.cell().margin.set(4);

		pane->set_layout_policy(ui::layout_policy::vert_major);

		pane->function_init([](ui::list& t){
			t.template_cell.set_size(120);
			t.template_cell.pad.set(5);
			t.set_layout_policy(ui::layout_policy::vert_major);

			{
				auto p = t.emplace<ui::elem>();
				p.cell().set_size(60);
			}

			t.emplace<ui::elem>();
			t.emplace<ui::elem>();
			t.function_init([](ui::label& label){
				label.set_text("#<[s.75>楼上的下来搞#<[s2>核算#<[s>\nsdasdasd");
			}).cell().set_external();
			t.emplace<ui::elem>();
			t.emplace<ui::elem>();
		});
	}


	// {
	// 	static int t = 0;
	// 	auto pane = bed.emplace<game::ui::field_editor>();
	// 	pane.cell().region_scale = {tags::from_extent, math::vec2{}, math::vec2{.9f, .1f}};
	// 	pane.cell().align = align::pos::center_right;
	// 	pane.cell().margin.set(4);
	// 	pane->set_edit_target(ui::edit_target_range_constrained{ui::edit_target{&t}, {200, 400}});
	//
	// }
	// {
	// 	static int t = 0;
	// 	auto pane = bed.emplace<game::ui::named_field_editor>();
	// 	pane.cell().region_scale = {tags::from_extent, math::vec2{}, math::vec2{.9f, .15f}};
	// 	pane.cell().align = align::pos::center_right;
	// 	pane.cell().margin.set(4);
	// 	pane->get_editor().set_edit_target(ui::edit_target_range_constrained{ui::edit_target{&t}, {200, 400}});
	//
	// 	pane->set_name_height(50);
	// 	pane->set_editor_height(80);
	// }
	//

	//

	// auto pane = bed.emplace<ui::creation::file_selector>();
	// pane.cell().region_scale = {tags::from_extent, math::vec2{}, math::vec2{.6f, .9f}};
	// pane.cell().align = align::pos::center_left;

	// auto spane = bed.emplace<ui::scroll_pane>();
	// spane.cell().region_scale = {math::vec2{}, math::vec2{.5, .8}};
	// spane.cell().align = align::pos::top_left;
	// spane->set_layout_policy(ui::layout_policy::hori_major);

	/*auto nscene = bed.emplace<ui::nested_scene>();
	nscene.cell().region_scale = {tags::from_extent, math::vec2{}, math::vec2{.3f, .5f}};
	nscene.cell().align = align::pos::bottom_left;
	//
	nscene->get_group().function_init([](ui::table& t){
		t.function_init([](ui::text_input_area& text){
			text.set_text("楼上的下来搞\nadasd\n\nasdasdas\n\n核算abcde啊啊啊.,。，；：");
			text.set_policy(font::typesetting::layout_policy::auto_feed_line);
		}).cell().set_external({true, true});
		t.context_size_restriction = ui::extent_by_external;
	});

	nscene->get_group().function_init([](ui::table& t){
		t.function_init([](ui::text_input_area& text){
			text.set_text("楼上的下来搞\nadasd\n\nasdasdas\n\n核算abcde啊啊啊.,。，；：");
			text.set_policy(font::typesetting::layout_policy::auto_feed_line);
		}).cell().set_external({true, true});
		t.context_size_restriction = ui::extent_by_external;
		t.property.relative_src = {-100, 500};
	});

	{
		auto& table = nscene->get_group().emplace<ui::table>();
		table.property.relative_src = {400, 500};

		table.template_cell.pad.set(4);

		table.emplace<ui::elem>();

		table.function_init([](ui::text_input_area& text){
			text.set_text(R"(楼上的下来搞核算abcde啊啊啊.,。，；："123"'t')");
			text.set_policy(font::typesetting::layout_policy::auto_feed_line);

			text.set_tooltip_state({
				.layout_info = ui::tooltip_layout_info{
					.follow = ui::tooltip_follow::dialog,
					.align_tooltip = align::pos::center,
				},
				.auto_release = false,
			}, [](ui::text_input_area& t){
				t.set_text(R"(#<[srch>楼上的下来搞核算abcde啊啊啊.,。，；："123"'t')");
				t.set_policy(font::typesetting::layout_policy::auto_feed_line);
				t.property.fill_parent = {true, true};
			});
		}).cell().set_external({true, true});

		{
			auto image_hdl = table.end_line().emplace<ui::image_frame>();
			image_hdl.cell().set_height(4);
			image_hdl.cell().pad.set(12);
			image_hdl.cell().margin.set_hori(12);
			image_hdl.cell().saturate = true;

			image_hdl->property.set_empty_drawer();
			image_hdl->default_style.scaling = align::scale::stretchX;
			image_hdl->set_drawable<ui::image_caped_region_drawable>(graphic::draw::mode_flags::sdf | graphic::draw::mode_flags::slide_line, region, .075f);
		}

		{

			auto slider = table.end_line().emplace<ui::slider>();
			// slider.cell().set_height(60);
			slider.cell().pad.top = 8.;
			slider->set_hori_only();


			auto collapser = table.emplace<ui::collapser>();
			collapser.cell().set_external({false, true});

			collapser->head().function_init([&](ui::image_frame& img){
				img.property.set_empty_drawer();
				img.set_drawable<ui::icon_drawable>(0, ui::assets::icons::up);
				img.set_drawable<ui::icon_drawable>(1, ui::assets::icons::down);
				img.add_collapser_image_swapper();
			}).cell().set_width(60);
			collapser->head().function_init([&](ui::basic_text_elem& txt){
				txt.property.set_empty_drawer();
				txt.set_text("#<[segui>Collapser Pane");
				txt.set_policy(font::typesetting::layout_policy::auto_feed_line);
			}).cell().set_external({false, true}).pad.left = 12;

			collapser->content().function_init([&](ui::check_box& img){
				img.set_drawable<ui::icon_drawable>(0, ui::assets::icons::left);
				img.set_drawable<ui::icon_drawable>(1, ui::assets::icons::up);
				img.set_drawable<ui::icon_drawable>(2, ui::assets::icons::down);
				img.add_multi_select_tooltip({
					// .follow = ui::tooltip_follow::owner,
					// .target_align = ,
					// .owner_align = ,
					// .offset =
				});
			}).cell().set_height(96);

			collapser->content().end_line().function_init([](ui::text_input_area& text){
				text.set_text("楼上的叮咚鸡\n\n\n下来算abcde啊啊啊.,。，；：");
				text.set_policy(font::typesetting::layout_policy::auto_feed_line);
			}).cell().set_external({false, true}).set_pad({.top = 8});




			auto button = table.end_line().emplace<ui::button<ui::image_frame>>();
			button->set_drawable<ui::icon_drawable>(ui::assets::icons::left);
			button.cell().set_size(60);

			auto bar = table.emplace<ui::progress_bar>();
			bar->reach_speed = .05f;
			bar.cell().set_height(60);


			bar->set_progress_prov([&e = slider.elem()] -> ui::progress_bar_progress {
				return {e.get_progress().x};
			});
		}

		table.set_edge_pad(0);

	}*/
}
#pragma endregion

void main_loop(){
	using namespace mo_yanxi;

	auto& context = core::global::graphic::context;

	graphic::image_atlas& atlas{core::global::assets::atlas};
	graphic::image_page& main_page = atlas.create_image_page("main");

	test::load_tex(atlas);
	core::global::graphic::init_renderers();

	auto& renderer_world = core::global::graphic::world;
	auto& renderer_ui = core::global::graphic::ui;

	renderer_world.camera.set_scale_range({0.2f, 2.f});

	{
		auto& g = renderer_ui.batch.emplace_batch_layer<graphic::layers::grid_drawer>();
		g.data.current = graphic::layers::default_grid_style;
	}

	{
		auto main_scene = core::global::ui::root->add_scene<ui::loose_group>("main", math::frect{math::vector2{context.get_extent().width, context.get_extent().height}.as<float>()}, renderer_ui);
		init_ui(main_scene.root_group, atlas);
		core::global::ui::root->switch_scene_to("main");
	}

	game::world::hud hud{};
	hud.focus_hud();

	core::global::input.main_binds.register_bind(core::ctrl::key::F2, core::ctrl::act::press, [](auto){
		core::global::ui::root->switch_scene_to("hud");
	});

	core::global::input.main_binds.register_bind(core::ctrl::key::F1, core::ctrl::act::press, [](auto){
		core::global::ui::root->switch_scene_to("main");
	});

	game::ecs::system::motion_system motion_system{};
	game::ecs::system::renderer ecs_renderer{};

	game::world::entity_top_world world{renderer_world};

	hud.bind_context({
		.component_manager = &world.component_manager,
		.collision_system = &world.collision_system,
		.graphic_context = &world.graphic_context
	});

	auto& wgfx_input =
		core::global::input.register_sub_input<
			math::vec2,
			game::world::graphic_context&,
			game::ecs::component_manager&
				>("gfx");
	wgfx_input.set_context(math::vec2{}, std::ref(world.graphic_context), std::ref(world.component_manager));

	// test::set_swapchain_flusher();

	graphic::borrowed_image_region light_region = main_page.register_named_region("pester.light", graphic::bitmap_path_load{R"(D:\projects\mo_yanxi\prop\assets\texture\pester.light.png)"}).first;
	graphic::borrowed_image_region base_region = main_page.register_named_region("pester", graphic::bitmap_path_load{R"(D:\projects\mo_yanxi\prop\assets\texture\pester.png)"}).first;
	graphic::borrowed_image_region base_region2 = main_page.register_named_region("pesterasd", graphic::bitmap_path_load{R"(D:\projects\mo_yanxi\prop\CustomUVChecker_byValle_1K.png)"}).first;


	using grided_entity_desc = game::ecs::desc::grid_entity;

	game::meta::chamber::grid grid{};
	game::meta::hitbox_transed hitbox_transed{};


	std::future<void> async_collide{};

	auto grid_loader = [&]{
		if(async_collide.valid()){
			async_collide.get();
		}

		world.reset();

		{
			std::ifstream stream{R"(D:\projects\mo_yanxi\build\windows\x64\debug\test_grid.metagrid)", std::ios::binary};
			game::meta::srl::read_grid(stream, grid);
		}

		{
			std::ifstream stream{R"(D:\projects\mo_yanxi\build\windows\x64\debug\tes.hbox)", std::ios::binary};
			io::loader<game::meta::hitbox_transed>::parse_from(stream, hitbox_transed);
			hitbox_transed.apply();
		}

		math::rand rand{};
		for(int i = 0; i < 8; ++i){
			using namespace game::ecs;

			manifold mf{};
			math::trans2 trs = {{rand(4000.f), rand(2000.f)}, rand(180.f)};
			mf.hitbox = game::hitbox{hitbox_transed};
			//game::hitbox{game::hitbox_comp{.box = {math::vec2{chamber::tile_size * 4, chamber::tile_size * 4}}}};

			faction_data faction_data{};
			if(!(i & 1)){
				faction_data.faction = game::faction_0;
			}else{
				faction_data.faction = game::faction_1;
			}

			chamber::chamber_manifold cmf{grid};
			game::meta::chamber::grid_structural_statistic stat{grid};
			cmf.hit_point = {stat.get_structural_hitpoint()};
			cmf.hit_point.cure();

			cmf.manager.sliced_each([](chamber::turret_build& b){
				using namespace math;

				auto& body = b.meta;
				body.shoot_type.projectile = &test::projectile_meta;
				body.shoot_type.burst = {.count = 1, .spacing = 5};
				body.shoot_type.salvo = {.count = 3};
				b.rotate_torque = 30;

				body.range.to = 6000;
				body.drawer = &test::drawer;
			});

			world.component_manager.create_entity_deferred<grided_entity_desc>(mech_motion{.trans = trs},
				std::move(mf),
				std::move(cmf),
				std::move(faction_data),
				physical_rigid{
				.inertial_mass = 500000
			});
		}
	};

	{
		static auto l = grid_loader;
		core::global::input.main_binds.register_bind(core::ctrl::key::R, core::ctrl::act::press, core::ctrl::mode::ctrl, [](auto){
			l();
		});

		wgfx_input.register_bind(core::ctrl::key::Q, core::ctrl::act::press, +[](core::ctrl::packed_key_t, math::vec2 pos, game::world::graphic_context& ctx, game::ecs::component_manager& component_manager){
			using namespace game::ecs;

			auto hdl = game::meta::create(component_manager, test::projectile_meta);

			math::rand rand{};
			auto dir =
				(pos.scl(1, -1).add_y(ctx.renderer().camera.get_screen_size().y) - ctx.renderer().camera.get_screen_center())
				.set_length(rand(200, 400));
			math::trans2 trs = {ctx.renderer().camera.get_stable_center(), dir.angle_rad()};

			hdl.set_initial_trans(trs);
			hdl.set_initial_vel({dir, 1});
			hdl.resume();

			ctx.create_efx().set_data({
				.style = game::fx::line_splash{
					.count = 45,
					.distribute_angle = 5 * math::deg_to_rad,
					.range = {260, 2420},
					.stroke = {{2, 0}, {6, 0}},
					.length = {{30, 20}, {90, 750, math::interp::slope}},

					.palette = {
						{
							graphic::colors::white.create_lerp(graphic::colors::ORANGE, .65f).to_light().set_a(.5),
							graphic::colors::white.to_light().set_a(0),
							math::interp::linear_map<0.f, .55f>
						}, {
							graphic::colors::aqua.to_light().set_a(0.95f),
							graphic::colors::white.to_light().set_a(0),
							math::interp::pow3In | math::interp::reverse
						}
					}
				},
				.trans = trs,
				.depth = 0,
				.duration = {38}
				});
		});
	}

	world.component_manager.do_deferred();

	game::fx::post_process_graph graph{core::global::graphic::context};

	graph.set_updator([&](game::fx::post_process_graph& graph){
		graph.ui_base.as_image().handle = renderer_ui.batch.blit_base;
		graph.ui_base.as_image().expected_layout = VK_IMAGE_LAYOUT_GENERAL;

		graph.ui_light.as_image().handle = renderer_ui.batch.blit_light;
		graph.ui_light.as_image().expected_layout = VK_IMAGE_LAYOUT_GENERAL;

		graph.world_blit_base.as_image().handle = renderer_world.batch.mid_attachments.color_base;
		graph.world_blit_base.as_image().expected_layout = VK_IMAGE_LAYOUT_GENERAL;

		graph.world_blit_light.as_image().handle = renderer_world.batch.mid_attachments.color_light;
		graph.world_blit_light.as_image().expected_layout = VK_IMAGE_LAYOUT_GENERAL;

		graph.world_draw_resolved_depth.as_image().handle = {renderer_world.batch.depth_resolved, renderer_world.batch.depth_resolved_view};
		graph.world_draw_resolved_depth.as_image().expected_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		graph.world_draw_resolved_base.as_image().handle = renderer_world.batch.draw_resolved_attachments.color_base;
		graph.world_draw_resolved_base.as_image().expected_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		graph.world_draw_resolved_light.as_image().handle = renderer_world.batch.draw_resolved_attachments.color_light;
		graph.world_draw_resolved_light.as_image().expected_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;



		graph.world_draw_depth.as_image().handle = {renderer_world.batch.depth, renderer_world.batch.depth_view};
		graph.world_draw_depth.as_image().expected_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		graph.world_draw_base.as_image().handle = renderer_world.batch.draw_attachments.color_base;
		graph.world_draw_base.as_image().expected_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		graph.world_draw_light.as_image().handle = renderer_world.batch.draw_attachments.color_light;
		graph.world_draw_light.as_image().expected_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;




		graph.world_draw_oit_head.as_image().handle = renderer_world.batch.oit_image_head;
		graph.world_draw_oit_head.as_image().expected_layout = VK_IMAGE_LAYOUT_GENERAL;

		graph.world_draw_oit_buffer.as_buffer().handle = renderer_world.batch.oit_buffer.borrow_after(sizeof(graphic::oit_statistic));
		graph.world_draw_oit_buffer_stat.as_buffer().handle = renderer_world.batch.oit_buffer.borrow(0, sizeof(graphic::oit_statistic));
	});

	test::set_up_flush_setter([&]{
		graph.update_resources();
	}, [&]{
		return graph.get_output_image().image;
	});


	graph.world_bloom().set_strength(1.05f, 0.95f);
	graph.ui_bloom().set_strength(.8f, .8f);

	core::global::timer.reset_time();
	while(!context.window().should_close()){
		context.window().poll_events();
		core::global::timer.fetch_time();
		core::global::input.update(core::global::timer.global_delta_tick());

		// count++;
		// dt += core::global::timer.global_delta();
		// if(dt > 1.f){
		// 	dt = 0.f;
		// 	std::println(std::cerr, "{}", count);
		// 	count = 0;
		// }


		world.component_manager.update_update_delta(core::global::timer.update_delta_tick());
		if(world.graphic_context.update(core::global::timer.global_delta_tick(), core::global::timer.is_paused())){
			graph.ssao_pass().set_scale(world.graphic_context.renderer().camera.map_scale(0.15f, 2.5f) * 1.5f);
			graph.world_bloom().set_scale(world.graphic_context.renderer().camera.map_scale(0.9f, 1.5f));
		}
		renderer_ui.set_time(core::global::timer.global_time());

		wgfx_input.set_context(core::global::ui::root->get_current_focus().get_cursor_pos());


		core::global::ui::root->update(core::global::timer.global_delta_tick());
		core::global::ui::root->layout();
		core::global::ui::root->draw();



		graphic::draw::world_acquirer<> acquirer{renderer_world.batch, static_cast<const graphic::combined_image_region<graphic::uniformed_rect_uv>&>(base_region)};
		math::rect_box_posed rect2{math::vec2{200, 40}};

		auto cursor_in_world = renderer_world.camera.get_screen_to_world(core::global::ui::root->get_current_focus().get_cursor_pos(), {}, true);

		auto dst = cursor_in_world - renderer_world.camera.get_viewport_center();


		rect2.update({
			cursor_in_world,
			dst.angle_rad()});

		{

			// acquirer.proj.depth = 1.f;

			acquirer << graphic::draw::white_region;
			// graphic::draw::fill::rect_ortho(
			// 		acquirer.get(),
			// 		math::frect{math::vec2{}, 40000},
			// 		graphic::colors::black
			// 	);

			acquirer.proj.depth = 0.25f;

			graphic::draw::line::quad(
					acquirer,
					rect2.view_as_quad(),
					2.f,
					graphic::colors::white.copy().set_a(.3f)
				);

			acquirer.proj.depth = 0.35f;
			acquirer.proj.slightly_decr_depth();

			graphic::draw::line::line(acquirer.get(), cursor_in_world, renderer_world.camera.get_viewport_center(),
				3.,
				graphic::colors::aqua.to_light(),
				graphic::colors::white.to_light().set_a(0)
			);
		}


		{
			{
				auto viewport = renderer_world.camera.get_viewport();

				world.component_manager.sliced_each([&](
					const game::ecs::chunk_meta& meta,
				   game::ecs::manifold& manifold,
				   game::ecs::mech_motion& motion
				){
				   if(!viewport.overlap_exclusive(manifold.hitbox.max_wrap_bound()))return;

				   using namespace graphic;
				   for (const auto & comp : manifold.hitbox.get_comps()){
					   draw::line::quad(acquirer, comp.box, 2, colors::white.to_light());
					   draw::line::rect_ortho(acquirer, comp.box.get_bound(), 1, colors::CRIMSON);
				   }
				   draw::line::quad(acquirer, manifold.hitbox.ccd_wrap_box(), 1, colors::pale_green);
				   draw::line::line_angle_center(acquirer.get(), motion.trans, 1000, 4);
				   draw::line::line_angle_center(acquirer.get(), {motion.trans.vec, motion.trans.rot + math::pi_half}, 1000, 4);
				   draw::line::line_angle(acquirer.get(), math::trans2{600} | motion.trans, 600, 8, colors::ORANGE);
			   });

				world.component_manager.sliced_each([&](
					game::ecs::chamber::chamber_manifold& grid,
					const game::ecs::mech_motion& motion
				){

						if(!grid.get_wrap_bound().overlap_rough(viewport) || !grid.get_wrap_bound().overlap_exact(viewport))return;

						using namespace game::ecs;
						using namespace graphic;

						math::mat3 mat3{};
						mat3.from_transform(grid.get_transform());

						renderer_world.batch.push_proj(mat3);

						draw::line::quad(acquirer, grid.local_grid.get_wrap_bound(), 3, colors::AQUA_SKY.to_light());

					grid.local_grid.each_tile([&](const chamber::tile& tile){
							if(!tile.building) return;

							draw::line::rect_ortho(acquirer, tile.get_bound(), 1, colors::dark_gray.to_light());
							draw::fill::rect_ortho(
								acquirer.get(),
								tile.get_bound(),

								(colors::red_dusted.create_lerp(tile.building.data().get_meta()->get_meta_info().is_structural() ? colors::pale_yellow : colors::pale_green, tile.building.data().hit_point.get_capability_factor()))
									.to_light(1.5f)
									.set_a(tile.get_status().valid_hit_point / tile.building.data().get_tile_individual_max_hitpoint()));
						});

						grid.manager.sliced_each([&](const chamber::building_data& building_data){
							draw::line::rect_ortho(acquirer, building_data.get_bound(), 3, colors::CRIMSON.to_light());
						});

						auto local_rect = grid.box_to_local(static_cast<const math::rect_box<float>&>(rect2));

						bool any{};
						grid.local_grid.quad_tree()->intersect_then(
							local_rect, [&](const math::rect_box<float>& rect, math::frect region){
								return !any && rect.overlap_rough(region) && rect.overlap_exact(region);
							}, [&](const auto& rect, const chamber::tile& tile){
								any = true;
							});

						if(true){
							auto rst = grid.get_dst_sorted_tiles(rect2, (rect2[0] + rect2[3]) / 2, dst.normalize());
							for (const auto& [idx, tile] : rst | std::views::enumerate){
								draw::fill::rect_ortho(
									acquirer.get(),
									tile.get_bound(),
									colors::aqua.create_lerp(colors::red_dusted, idx / static_cast<float>(rst.size())));
							}
						}


						grid.manager.sliced_each([&](const chamber::upper_building_drawable_tag&, const chamber::building& building){
							building.draw_upper_building(world.graphic_context);
						});


						renderer_world.batch.pop_proj();


					});
			}

			if(async_collide.valid()){
				async_collide.get();
			}

			if(!core::global::timer.is_paused()){
				world.collision_system.apply_collision();
				//collision do things the last frame do, so before defer

				world.component_manager.do_deferred();

				world.component_manager.sliced_each([&](
					game::ecs::move_command& cmd,
					game::ecs::mech_motion& motion,
					const game::ecs::physical_rigid& rigid,
					const game::ecs::chamber::chamber_manifold& chamber_manifold
					){
					const float a_accelMax = chamber_manifold.maneuver_subsystem.torque() / (rigid.rotational_inertia / 1E5);

					if(auto next = cmd.get_next(motion.trans, 0)){
						math::angle approach = motion.pos().angle_to_rad(next->vec);
						auto yaw_angle = approach - motion.trans.rot;

						float accelMax = chamber_manifold.maneuver_subsystem.get_force_at(yaw_angle) / rigid.inertial_mass;
						auto overhead = std::max(motion.overhead(accelMax), 12.f);
						cmd.update(motion.trans, overhead * 1.65f, math::pi_2);

						const auto v_accel = math::constrain_resolve::smooth_approach(next->vec - motion.pos(), motion.vel.vec, accelMax, 32.f);


						const auto a_accel = math::constrain_resolve::smooth_approach(
							motion.pos().angle_to_rad(next->vec) - motion.trans.rot,
							motion.vel.rot,
							a_accelMax, 0.0f

							);
						motion.accel += {v_accel, a_accel};
					}else{
						float accelMax = chamber_manifold.maneuver_subsystem.force_longitudinal() / rigid.inertial_mass;

						motion.accel += motion.get_stop_accel(accelMax, a_accelMax);
						//TODO make it stop
					}
					motion.vel.vec.limit_max_length(15);
				});
				motion_system.run(world.component_manager);
				world.collision_system.insert_all(world.component_manager);

				async_collide = std::async(std::launch::async, [&]{
					world.collision_system.run_collision_test(world.component_manager);
				});

				game::ecs::system::chamber{}.run(world);
				game::ecs::system::projectile{}.run(world.component_manager, world.graphic_context);

			}

			ecs_renderer.filter_screen_space(world.component_manager, renderer_world.camera.get_viewport());
			ecs_renderer.draw(world.graphic_context);
		}

		world.graphic_context.render_efx();
		renderer_world.batch.batch.consume_all();
		vk::cmd::submit_command(core::global::graphic::context.compute_queue(), {graph.get_main_command_buffer()});
		context.flush();
	}
	if(async_collide.valid())async_collide.get();

	context.wait_on_device();
}
#endif

#pragma endregion

void draw_main(){
	test::check_validation_layer_activation();


	using namespace mo_yanxi;
	using namespace mo_yanxi::game;
	using namespace mo_yanxi::graphic;

	test::init_assets();
	test::compile_shaders();
	//
	core::glfw::init();
	core::global::graphic::init_vk();
	core::global::assets::init(&core::global::graphic::context);
	assets::graphic::load(core::global::graphic::context);

	//
	// while(!core::global::graphic::context.window().should_close()){
	// 	core::global::graphic::context.window().poll_events();
	// }
	//
	// core::global::graphic::context.wait_on_device();
	// graph_small_test();
	// graphic::shader_reflection reflection{assets::graphic::shaders::comp::bloom.get_binary()};
	// reflection.foo();

	core::global::ui::init();
	game::content::load();
	test::load_content();

	main_loop();

	test::dispose_content();
	core::global::ui::dispose();

	assets::graphic::dispose();
	core::global::assets::dispose();
	core::global::graphic::dispose();
	core::glfw::terminate();
}


int main(){
	using namespace mo_yanxi;
	using namespace mo_yanxi::game;
	using namespace mo_yanxi::graphic;
	test::check_validation_layer_activation();

	test::init_assets();
	test::compile_shaders();
	//
	core::glfw::init();
	core::global::graphic::init_vk();
	core::global::assets::init(&core::global::graphic::context);
	assets::graphic::load(core::global::graphic::context);

	camera2 camera;
	math::vec2 cursor;
	assets::ctrl::spec_camera = &camera;

	core::global::ui::init();
	game::content::load();
	test::load_content();


	struct VertexUBO{
		vk::padded_mat3 proj{};
		vk::padded_mat3 view{};
		vk::padded_mat3 reverse{};

		[[nodiscard]] VertexUBO(math::matrix3 proj, math::matrix3 view) : proj(proj), view(view), reverse((view * proj).inv()){}
	};

	{

#pragma region Shader
		auto& ctx = core::global::graphic::context;

		vk::shader_module msh{ctx.get_device(), assets::dir::shader_spv / "ui.draw.basic.spv"};
		msh.set_no_deduced_stage();



		vk::shader_module highlight_pick{ctx.get_device(), assets::dir::shader_spv / "post_process.highlight_extract.spv", VK_SHADER_STAGE_COMPUTE_BIT};
		vk::shader_module hdr_to_sdr{ctx.get_device(), assets::dir::shader_spv / "post_process.hdr_to_sdr.spv", VK_SHADER_STAGE_COMPUTE_BIT};
		vk::shader_module volume_shader{ctx.get_device(), assets::dir::shader_spv / "post_process.volume_pass.spv", VK_SHADER_STAGE_COMPUTE_BIT};
		vk::shader_module bloom_merge_shader{ctx.get_device(), assets::dir::shader_spv / "post_process.bloom.merge.spv", VK_SHADER_STAGE_COMPUTE_BIT};
#pragma endregion

		mo_yanxi::gui::renderer renderer{ctx, assets::graphic::samplers::texture_sampler, msh};
		renderer.resize({0, 0, ctx.get_extent().width, ctx.get_extent().height});
		// renderer.get_top_viewport().push_scissor({{200, 200, 520, 420}});
		auto& batch = renderer.batch;

#pragma region VK_RES
		vk::uniform_buffer general_proj_ubo{ctx.get_allocator(), sizeof(VertexUBO), VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT};
		vk::descriptor_layout general_proj_descriptor_layout{ctx.get_device(), VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT, [](vk::descriptor_layout_builder& b){
			b.push_seq(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
		}};
		vk::descriptor_buffer general_proj_dbo{ctx.get_allocator(), general_proj_descriptor_layout, general_proj_descriptor_layout.binding_count()};
		(void)vk::descriptor_mapper{general_proj_dbo}.set_uniform_buffer(0, general_proj_ubo);

#pragma endregion

#pragma region render_graph
		render_graph::render_graph_manager manager{ctx};
		auto& res = manager.add_explicit_resource(render_graph::resource_desc::independent_resource{render_graph::resource_desc::independent_image{}});
		res.as_image().handle = renderer.get_base();
		res.as_image().expected_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		auto volume_pass = manager.add_stage<render_graph::volume_pass>(render_graph::post_process_meta{volume_shader, {
			{{0}, render_graph::no_slot, 0},
			{{1}, 0, render_graph::no_slot},
		}});
		volume_pass.meta.set_sampler_at_binding(1, assets::graphic::samplers::blit_sampler);

		{
			auto& d = manager.add_external_descriptor({general_proj_descriptor_layout, general_proj_dbo});
			volume_pass.meta.add_external_descriptor(d, 2);
			volume_pass.meta.batch.flush();
		}

		volume_pass.pass.add_input({render_graph::resource_desc::independent_resource_usage{res, 0}});



		auto highligh_pass = manager.add_stage<render_graph::post_process_stage>(render_graph::post_process_meta{highlight_pick, {
			{{0}, render_graph::no_slot, 0},
			{{1}, 0, render_graph::no_slot},
		}});

		highligh_pass.pass.add_dep({volume_pass.id()});


		const auto bloom_pass = manager.add_stage<render_graph::bloom_pass>(game::fx::get_bloom_default_meta());
		bloom_pass.meta.set_sampler_at_binding(0, assets::graphic::samplers::blit_sampler);
		bloom_pass.meta.set_sampler_at_binding(1, assets::graphic::samplers::blit_sampler);
		bloom_pass.pass.add_dep({highligh_pass.id()});
		bloom_pass.meta.meta().set_format_at_in(0, VK_FORMAT_R16G16B16A16_SFLOAT);


		const auto hdr_to_sdr_pass = manager.add_stage<render_graph::post_process_stage>(render_graph::post_process_meta{hdr_to_sdr, {
			{{0}, render_graph::no_slot, 0},
			{{1}, 0, render_graph::no_slot},
		}});

		auto bloom_merge_pass = manager.add_stage<render_graph::post_process_stage>(render_graph::post_process_meta{bloom_merge_shader, {
			{{0}, render_graph::no_slot, 0},
			{{1}, 0, render_graph::no_slot},
			{{2}, 1, render_graph::no_slot},
			{{3}, 2, render_graph::no_slot},
		}});

		bloom_merge_pass.pass.add_dep({volume_pass.id(), 0, 0});
		bloom_merge_pass.pass.add_dep({highligh_pass.id(), 0, 1});
		bloom_merge_pass.pass.add_dep({bloom_pass.id(), 0, 2});
		bloom_merge_pass.meta.meta().set_format_at_out(0, VK_FORMAT_R16G16B16A16_SFLOAT);

		auto smaa_final_pass = game::fx::add_smaa(manager, bloom_merge_pass.id());
		smaa_final_pass.meta.meta().set_format_at_out(0, VK_FORMAT_R16G16B16A16_SFLOAT);

		hdr_to_sdr_pass.pass.add_dep({smaa_final_pass.id()});

		hdr_to_sdr_pass.pass.add_output({
				{0, false},
			});

		{
			auto world_clear = manager.add_stage<render_graph::image_clear>(1);

			world_clear.pass.add_in_out({
				render_graph::resource_desc::independent_resource_usage{res, 0},
			});
			world_clear.pass.add_exec_dep(volume_pass.id());
		}

		manager.init_after_pass_initialized();
		manager.resize();
		manager.reset_pass_resources();
		manager.create_command();
#pragma endregion

#pragma region ResizeSet
		core::global::graphic::context.set_staging_image({
			.image = hdr_to_sdr_pass.pass.at_out(0).as_image().image.image,
			.extent = core::global::graphic::context.get_extent(),
			.clear = false,
			.owner_queue_family = core::global::graphic::context.compute_family(),
			.src_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			.src_access = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
			.dst_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			.dst_access = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
			.src_layout = VK_IMAGE_LAYOUT_GENERAL,
			.dst_layout = VK_IMAGE_LAYOUT_GENERAL
		});

		core::global::graphic::context.register_post_resize("test", [&](window_instance::resize_event event){
			core::global::ui::root->resize_all(math::frect{
					math::vector2{event.size.width, event.size.height}.as<float>()
				});

			renderer.resize({tags::from_extent, {}, event.size.width, event.size.height});
			gui::global::manager.resize(math::rect_ortho{tags::from_extent, {}, event.size.width, event.size.height}.as<float>());
			res.as_image().handle = renderer.get_base();
			res.as_image().expected_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			manager.resize(event.size);
			manager.reset_pass_resources();
			manager.create_command();
			core::global::graphic::context.set_staging_image(
				{
					.image = hdr_to_sdr_pass.pass.at_out(0).as_image().image.image,
					.extent = core::global::graphic::context.get_extent(),
					.clear = false,
					.owner_queue_family = core::global::graphic::context.compute_family(),
					.src_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					.src_access = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
					.dst_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					.dst_access = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
					.src_layout = VK_IMAGE_LAYOUT_GENERAL,
					.dst_layout = VK_IMAGE_LAYOUT_GENERAL
				}, false);
		});

		{
			math::vec2 window_extent = math::vector2{core::global::graphic::context.get_extent().width, core::global::graphic::context.get_extent().height}.as<float>();
			camera.resize_screen(window_extent.x, window_extent.y);

			auto rst = gui::global::manager.add_scene<gui::loose_group>("main", true, renderer);
			rst.scene.resize({{}, window_extent});
			test::build_main_ui(rst.scene, rst.root_group);

		}
#pragma endregion

		camera.set_scale_range({0.2f, 4.f});

		graphic::image_atlas& atlas{core::global::assets::atlas};
		graphic::image_page& main_page = atlas.create_image_page("main");

		graphic::borrowed_image_region light_region = main_page.register_named_region("pester.light", graphic::bitmap_path_load{R"(D:\projects\mo_yanxi\prop\CustomUVChecker_byValle_1K.png)"}).first;
		graphic::borrowed_image_region r1 = atlas.create_image_page("1").register_named_region("pester.light", graphic::bitmap_path_load{R"(D:\projects\mo_yanxi\prop\assets\texture\pester.png)"}).first;
		graphic::borrowed_image_region r2 = atlas.create_image_page("2").register_named_region("pester.light", graphic::bitmap_path_load{R"(D:\projects\mo_yanxi\prop\CustomUVChecker_byValle_1K.png)"}).first;
		graphic::borrowed_image_region r3 = atlas.create_image_page("3").register_named_region("pester.light", graphic::bitmap_path_load{R"(D:\projects\mo_yanxi\prop\assets\texture\pester.png)"}).first;
		graphic::borrowed_image_region r4 = atlas.create_image_page("4").register_named_region("pester.light", graphic::bitmap_path_load{R"(D:\projects\mo_yanxi\prop\CustomUVChecker_byValle_1K.png)"}).first;

		graphic::uniformed_trail trail{120, 0.5f};

#pragma region Inputset
		auto& wgfx_input =
		core::global::input.register_sub_input<
			math::vec2,
			draw::instruction::volume::batch&
				>("gfx");

		wgfx_input.set_context(math::vec2{}, std::ref(volume_pass.meta.batch));
		{
			using namespace core::ctrl;
			wgfx_input.register_bind(mouse::RIGHT, act::press, [](packed_key_t, math::vec2 pos, draw::instruction::volume::batch& b){
				math::rand rand{};
				b.push_instr(draw::instruction::volume::shockwave{
					.generic = {
						.lifetime = rand.random(60.f, 180.f)
					},
					.pos = pos,
					.radius = rand.random(100.f, 400.f),
					.stroke = rand.random(20.f, 60.f),
					.intensity = rand.random(15.f, 50),

					.color_from = colors::ORANGE.to_light(2.5f),
					.color_to = colors::clear,
				});
			});
		}
#pragma endregion

		static std::chrono::microseconds callbacks{};
		static std::chrono::microseconds coroutine{};
		static std::size_t count{};

		core::global::timer.reset_time();
		while(!ctx.window().should_close()){
			using namespace draw;

			ctx.window().poll_events();
			core::global::timer.fetch_time();
			core::global::input.update(core::global::timer.global_delta_tick());
			camera.update(core::global::timer.global_delta_tick());
			gui::global::manager.update(core::global::timer.global_delta_tick());
			gui::global::manager.layout();

			if(camera.check_changed()){
				const VertexUBO proj{
					math::mat3_idt,
					camera.get_world_to_uniformed()
				};
				// batch.update_ubo(proj);

				(void)vk::buffer_mapper{general_proj_ubo}.load(proj);

				// renderer.top_viewport().set_local_transform(camera.get_v2v_mat({}));
				auto screenP = renderer.top_viewport().get_element_to_root_screen() * camera.get_viewport_center();
				auto pos = renderer.get_screen_uniform_proj() * screenP;
				// bloom_pass.meta.set_scale(camera.map_scale(0.9f, 1.5f));
				renderer.notify_viewport_changed();

			}

			volume_pass.meta->update(core::global::timer.global_time() * 60.f);

			auto cursor_world = camera.get_screen_to_world(core::global::input.get_cursor_pos(), {}, true);
			wgfx_input.set_context(cursor_world);

			gui::global::manager.draw();
			renderer.wait_idle();

			// batch.reset();
			vk::cmd::submit_command(core::global::graphic::context.compute_queue(), {manager.get_main_command_buffer()});
			ctx.flush();

			++count;
		}


		std::println(std::cerr, "[SUM] Callback : {}", (callbacks / count).count());
		std::println(std::cerr, "[SUM] Coroutine: {}", (coroutine / count).count());


		ctx.wait_on_device();
	}
	// main_loop();

	test::dispose_content();
	core::global::ui::dispose();

	assets::graphic::dispose();
	core::global::assets::dispose();
	core::global::graphic::dispose();
	core::glfw::terminate();
	// draw_main();

	(void)0;
}
