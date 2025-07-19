#ifndef __AVX2__
#error "Requires __AVX2__"
#endif

#include <vulkan/vulkan.h>
// #include <cassert>

// #define VMA_IMPLEMENTATION
// #include <vk_mem_alloc.h>
// #include <freetype/freetype.h>

#include "../src/srl/srl.hpp"
#include "../src/srl/srl.game.hpp"
//
import mo_yanxi.graphic.bitmap;

import mo_yanxi.math;
import mo_yanxi.math.angle;
import mo_yanxi.math.vector2;
import mo_yanxi.math.vector4;
import mo_yanxi.math.rand;
import mo_yanxi.math.quad;
import mo_yanxi.math.constrained_system;

import mo_yanxi.vk.context;
import mo_yanxi.vk.batch;

import mo_yanxi.vk.vertex_info;
import mo_yanxi.vk.ext;

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
import mo_yanxi.graphic.renderer.merger;
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
import mo_yanxi.game.ecs.system.grid_system;
import mo_yanxi.game.ecs.system.renderer;
import mo_yanxi.game.ecs.system.projectile;
import mo_yanxi.game.ecs.system.chamber;

import mo_yanxi.game.ecs.entitiy_decleration;

import mo_yanxi.game.ecs.component.chamber.radar;
import mo_yanxi.game.ecs.component.chamber.turret;

import mo_yanxi.game.meta.instancing;
import mo_yanxi.game.content;
//

import mo_yanxi.game.ui.hitbox_editor;
import mo_yanxi.game.ui.grid_editor;
import mo_yanxi.game.content;
import mo_yanxi.game.meta.grid.srl;

import test;


import std;


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
		pane.cell().region_scale = {tags::from_extent, math::vec2{}, math::vec2{1.f, 0.75f}};
		pane.cell().align = align::pos::top_left;
		pane.cell().margin.set(4);
	}

	{
		auto pane = bed.emplace<ui::scroll_pane>();
		pane.cell().region_scale = {tags::from_extent, math::vec2{}, math::vec2{1.f, .25f}};
		pane.cell().align = align::pos::bottom_left;
		pane.cell().margin.set(4);

		pane->set_layout_policy(ui::layout_policy::vert_major);

		pane->set_elem([](ui::list& t){
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


void main_loop(){
	using namespace mo_yanxi;

	auto& context = core::global::graphic::context;

	graphic::image_atlas& atlas{core::global::assets::atlas};
	graphic::image_page& main_page = atlas.create_image_page("main");

	test::load_tex(atlas);
	core::global::graphic::init_renderers();

	auto& renderer_world = core::global::graphic::world;
	auto& renderer_ui = core::global::graphic::ui;
	auto& merger = core::global::graphic::merger;

	{
		auto& g = renderer_ui.batch.emplace_batch_layer<graphic::layers::grid_drawer>();
		g.data.current = graphic::layers::default_grid_style;
	}

	{
		core::global::ui::root->add_scene(ui::scene{"main", new ui::loose_group{nullptr, nullptr}, renderer_ui}, true);
		core::global::ui::root->resize(math::frect{math::vector2{context.get_extent().width, context.get_extent().height}.as<float>()});
		init_ui(core::global::ui::root->root_of<ui::loose_group>("main"), atlas);
	}

	game::world::hud hud{};
	hud.focus_hud();

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

	{
		context.register_post_resize("test", [&](window_instance::resize_event event){
		   core::global::ui::root->resize_all(
			   math::frect{math::vector2{event.size.width, event.size.height}.as<float>()});

		   renderer_world.resize(event.size);
		   renderer_ui.resize(event.size);
		   merger.resize(event.size);

		   context.set_staging_image(
			   {
				   .image = merger.get_result().image,
				   .extent = context.get_extent(),
				   .clear = false,
				   .owner_queue_family = context.compute_family(),
				   .src_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				   .src_access = VK_ACCESS_2_SHADER_WRITE_BIT,
				   .dst_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				   .dst_access = VK_ACCESS_2_SHADER_WRITE_BIT,
				   .src_layout = VK_IMAGE_LAYOUT_GENERAL,
				   .dst_layout = VK_IMAGE_LAYOUT_GENERAL
			   }, false);
	   });

		context.set_staging_image({
			.image = merger.get_result().image,
			.extent = context.get_extent(),
			.clear = false,
			.owner_queue_family = context.compute_family(),
			.src_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			.src_access = VK_ACCESS_2_SHADER_WRITE_BIT,
			.dst_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			.dst_access = VK_ACCESS_2_SHADER_WRITE_BIT,
			.src_layout = VK_IMAGE_LAYOUT_GENERAL,
			.dst_layout = VK_IMAGE_LAYOUT_GENERAL
		});
	}

	graphic::borrowed_image_region light_region = main_page.register_named_region("pester.light", graphic::path_load{R"(D:\projects\mo_yanxi\prop\assets\texture\pester.light.png)"}).first;
	graphic::borrowed_image_region base_region = main_page.register_named_region("pester", graphic::path_load{R"(D:\projects\mo_yanxi\prop\assets\texture\pester.png)"}).first;
	graphic::borrowed_image_region base_region2 = main_page.register_named_region("pesterasd", graphic::path_load{R"(D:\projects\mo_yanxi\prop\CustomUVChecker_byValle_1K.png)"}).first;


	using grided_entity_desc = game::ecs::desc::grid_entity;

	using projectile_entity_desc = game::ecs::desc::projectile;


	game::meta::chamber::grid grid{};
	game::meta::hitbox_transed hitbox_transed{};
	{
		std::ifstream stream{R"(D:\projects\mo_yanxi\build\windows\x64\debug\test_grid.metagrid)", std::ios::binary};
		game::meta::srl::read_grid(stream, grid);
	}

	{
		std::ifstream stream{R"(D:\projects\mo_yanxi\build\windows\x64\debug\tes.hbox)", std::ios::binary};
		io::loader<game::meta::hitbox_transed>::parse_from(stream, hitbox_transed);
		hitbox_transed.apply();
	}

	{



		{
			math::rand rand{};
			for(int i = 0; i < 1; ++i){
				using namespace game::ecs;

				manifold mf{};
				math::trans2 trs = {{rand(40.f), rand(20.f)}, rand(180.f)};
				mf.hitbox = game::hitbox{hitbox_transed};
				//game::hitbox{game::hitbox_comp{.box = {math::vec2{chamber::tile_size * 4, chamber::tile_size * 4}}}};

				faction_data faction_data{};
				if(!(i & 1)){
					faction_data.faction = game::faction_0;
				}else{
					faction_data.faction = game::faction_1;
				}

				chamber::chamber_manifold cmf{grid};

				world.component_manager.create_entity_deferred<grided_entity_desc>(mech_motion{.trans = trs},
					std::move(mf),
					std::move(cmf),
					std::move(faction_data),
					physical_rigid{
					.inertial_mass = 500000
				});
			}
		}

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


	std::size_t count{};
	float dt{};
	std::future<void> async_collide{};

	core::global::timer.reset_time();
	while(!context.window().should_close()){
		context.window().poll_events();
		core::global::timer.fetch_time();

		// count++;
		// dt += core::global::timer.global_delta();
		// if(dt > 1.f){
		// 	dt = 0.f;
		// 	std::println(std::cerr, "{}", count);
		// 	count = 0;
		// }

		core::global::input.update(core::global::timer.global_delta_tick());

		world.component_manager.update_delta = core::global::timer.update_delta_tick();
		world.graphic_context.update(core::global::timer.global_delta_tick(), core::global::timer.is_paused());
		renderer_ui.set_time(core::global::timer.global_time());

		wgfx_input.set_context(core::global::ui::root->get_current_focus().get_cursor_pos());


		core::global::ui::root->update(core::global::timer.global_delta_tick());
		core::global::ui::root->layout();
		core::global::ui::root->draw();


		graphic::draw::world_acquirer acquirer{renderer_world.batch, static_cast<const graphic::combined_image_region<graphic::uniformed_rect_uv>&>(base_region)};
		math::rect_box_posed rect2{math::vec2{200, 40}};

		auto cursor_in_world = renderer_world.camera.get_screen_to_world(core::global::ui::root->get_current_focus().get_cursor_pos(), {}, true);

		auto dst = cursor_in_world - renderer_world.camera.get_viewport_center();

		rect2.update({
			cursor_in_world,
			dst.angle_rad()});

		{
			/*{
				//
				for(int i = 0; i < 10; ++i){
					acquirer << base_region;
					acquirer.proj.depth = 1 - (static_cast<float>(i + 1) / 10.f - 0.04f);

					auto off = math::vector2{i * 500, i * 400}.as<float>();

					using namespace graphic::colors;

					graphic::draw::fill::quad(
						acquirer.get(),
						off - math::vec2{100, 500} + math::vec2{0, 0}.rotate_deg(45.f),
						off - math::vec2{100, 500} + math::vec2{500 + i * 200.f, 0}.rotate_deg(45.f),
						off - math::vec2{100, 500} + math::vec2{500 + i * 200.f, 5 + i * 100.f}.rotate_deg(45.f),
						off - math::vec2{100, 500} + math::vec2{0, 5 + i * 100.f}.rotate_deg(45.f),
						white.copy()
					);

					acquirer << light_region;
					acquirer.proj.slightly_decr_depth();
					graphic::draw::fill::quad(
						acquirer.get(),
						off - math::vec2{100, 500} + math::vec2{0, 0}.rotate_deg(45.f),
						off - math::vec2{100, 500} + math::vec2{500 + i * 200.f, 0}.rotate_deg(45.f),
						off - math::vec2{100, 500} + math::vec2{500 + i * 200.f, 5 + i * 100.f}.rotate_deg(45.f),
						off - math::vec2{100, 500} + math::vec2{0, 5 + i * 100.f}.rotate_deg(45.f),
						(white.copy() * 2).set_a((i + 3) / 10.f)
					);
				}
			}*/

			acquirer.proj.depth = 0.35f;

			acquirer << graphic::draw::white_region;
			graphic::draw::fill::rect_ortho(
					acquirer.get(),
					{math::vec2{}, 40000},
					graphic::colors::black
				);

			graphic::draw::line::quad(
					acquirer,
					rect2.view_as_quad(),
					2.f,
					graphic::colors::white
				);

			acquirer.proj.slightly_decr_depth();

			graphic::draw::line::line(acquirer.get(), cursor_in_world, renderer_world.camera.get_viewport_center(),
				3.,
				graphic::colors::aqua.to_light(),
				graphic::colors::white.to_light().set_a(0)
			);
		}

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
						draw::fill::rect_ortho(acquirer.get(), tile.get_bound(),
						                       (colors::red_dusted.create_lerp(colors::pale_green, tile.building.data().hit_point.get_capability_factor())).to_light(1.5f).set_a(
							                       tile.get_status().valid_hit_point / tile.building.data().get_tile_individual_max_hitpoint()));
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

					if(any){
						auto rst = grid.get_dst_sorted_tiles(rect2, (rect2[0] + rect2[3]) / 2, dst.normalize());
						for (const auto& [idx, tile] : rst | std::views::enumerate){
							draw::fill::rect_ortho(acquirer.get(), tile.get_bound(),
												   colors::aqua.create_lerp(colors::red_dusted, idx / static_cast<float>(rst.size())));
						}
					}

					renderer_world.batch.pop_proj();
				});

			if(async_collide.valid()){
				async_collide.get();
			}

			if(!core::global::timer.is_paused()){
				world.component_manager.do_deferred();

				world.component_manager.sliced_each([&](
					game::ecs::move_command& cmd,
					game::ecs::mech_motion& motion
					){

					constexpr float accelMax = .85f;
					auto overhead = std::max(motion.overhead(accelMax), 12.f);
					cmd.update(motion.trans, overhead * 1.65f, math::pi_2);
					if(auto next = cmd.get_next(motion.trans, 0)){
						const auto v_accel = math::constrain_resolve::smooth_approach(next->vec - motion.pos(), motion.vel.vec, accelMax, 32.f);
						const auto a_accel = math::constrain_resolve::smooth_approach(motion.pos().angle_to_rad(next->vec) - motion.trans.rot, motion.vel.rot, 0.001f, 0.f);
						motion.accel += {v_accel, a_accel};
					}else{
						motion.accel += motion.get_stop_accel(accelMax, 0.001f);
						//TODO make it stop
					}
					motion.vel.vec.limit_max_length(15);
				});
				motion_system.run(world.component_manager);
				world.collision_system.insert_all(world.component_manager);

				game::ecs::system::chamber{}.run(world);
				game::ecs::system::projectile{}.run(world.component_manager, world.graphic_context);

				async_collide = std::async(std::launch::async, [&]{
					world.collision_system.run_collision_test(world.component_manager);
				});
			}

			ecs_renderer.filter_screen_space(world.component_manager, renderer_world.camera.get_viewport());
			ecs_renderer.draw(world.graphic_context);

		}

		world.graphic_context.render_efx();

		renderer_world.batch.batch.consume_all();
		renderer_world.post_process();

		renderer_ui.post_process();

		merger.submit();
		renderer_ui.clear();

		context.flush();
	}
	if(async_collide.valid())async_collide.get();

	context.wait_on_device();


}

int main(){
	using namespace mo_yanxi;
	using namespace mo_yanxi::game;

	test::init_assets();
	// compile_shaders();

	core::glfw::init();
	core::global::graphic::init_vk();
	core::global::assets::init(&core::global::graphic::context);
	assets::graphic::load(core::global::graphic::context);
	core::global::ui::init();


	game::content::load();

	main_loop();

	core::global::ui::dispose();
	assets::graphic::dispose();
	core::global::assets::dispose();
	core::global::graphic::dispose();
	core::glfw::terminate();
}
