#include <vulkan/vulkan.h>
#include <cassert>

// #define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include <freetype/freetype.h>

#include "../src/srl/srl.hpp"
#include "../src/srl/srl.game.hpp"

import mo_yanxi.graphic.bitmap;

import mo_yanxi.math;
import mo_yanxi.math.angle;
import mo_yanxi.math.vector2;
import mo_yanxi.math.vector4;
import mo_yanxi.math.quad;

import mo_yanxi.vk.instance;
import mo_yanxi.vk.sync;
import mo_yanxi.vk.context;
import mo_yanxi.vk.command_pool;
import mo_yanxi.vk.command_buffer;
import mo_yanxi.vk.resources;

import mo_yanxi.vk.image_derives;
import mo_yanxi.vk.shader;
import mo_yanxi.vk.uniform_buffer;
import mo_yanxi.vk.descriptor_buffer;
import mo_yanxi.vk.pipeline.layout;
import mo_yanxi.vk.pipeline;
import mo_yanxi.vk.dynamic_rendering;
import mo_yanxi.vk.util;
import mo_yanxi.vk.util.uniform;
import mo_yanxi.vk.batch;
import mo_yanxi.vk.sampler;

import mo_yanxi.vk.vertex_info;
import mo_yanxi.vk.ext;

import mo_yanxi.assets.graphic;
import mo_yanxi.assets.directories;
import mo_yanxi.assets.ctrl;
import mo_yanxi.graphic.shaderc;
import mo_yanxi.graphic.color;
import mo_yanxi.graphic.post_processor.bloom;

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
import mo_yanxi.graphic.msdf;
import mo_yanxi.graphic.draw;
import mo_yanxi.graphic.draw.func;
import mo_yanxi.graphic.draw.multi_region;
import mo_yanxi.graphic.image_manage;
import mo_yanxi.graphic.image_multi_region;

import mo_yanxi.graphic.layers.ui.grid_drawer;

import mo_yanxi.font;
import mo_yanxi.font.manager;
import mo_yanxi.font.typesetting;

import mo_yanxi.graphic.msdf;

import mo_yanxi.ui.root;
import mo_yanxi.ui.basic;
import mo_yanxi.ui.manual_table;
import mo_yanxi.ui.table;
import mo_yanxi.ui.elem.text_elem;
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

import mo_yanxi.game.graphic.effect;
import mo_yanxi.game.world.graphic;
import mo_yanxi.game.world.hud;
import mo_yanxi.game.ecs.world.top;

import mo_yanxi.game.ecs.component.manager;
import mo_yanxi.game.ecs.task_graph;
import mo_yanxi.game.ecs.dependency_generator;

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


import mo_yanxi.game.ui.hitbox_editor;

import test;
import hive;


import std;

void compile_shaders(){
	using namespace mo_yanxi;

	graphic::shader_runtime_compiler compiler{};
	graphic::shader_wrapper wrapper{compiler, assets::dir::shader_spv.path()};

	assets::dir::shader_src.for_all_subs([&](io::file&& file){
		wrapper.compile(file);
	});
}

void init_assets(){
	mo_yanxi::assets::load_dir(R"(D:\projects\mo_yanxi\prop)");
	mo_yanxi::assets::ctrl::load();
}

void init_ui(mo_yanxi::ui::loose_group& root, mo_yanxi::graphic::image_atlas& atlas){
	using namespace std::literals;
	using namespace mo_yanxi;

	auto& ui_page = *atlas.find_page("ui");

	auto& line_svg = ui_page.register_named_region(
		"line"s,
		graphic::sdf_load{
			graphic::msdf::msdf_generator{R"(D:\projects\mo_yanxi\prop\assets\svg\line.svg)"}, {40u, 24u}
		}).first;

	ui_page.mark_protected("line");

	graphic::image_caped_region region{line_svg, line_svg.get_region(), 12, 12, graphic::msdf::sdf_image_boarder};

	root.property.set_empty_drawer();
	root.skip_inbound_capture = true;

	ui::elem_ptr e{root.get_scene(), &root, std::in_place_type<ui::manual_table>};
	e->property.set_empty_drawer();
	e->property.fill_parent = {true, true};
	e->skip_inbound_capture = true;
	auto& bed = static_cast<ui::manual_table&>(root.add_children(std::move(e)));

	{
		auto pane = bed.emplace<ui::button<>>();
		pane->set_button_callback(ui::button_tags::general, [](ui::elem& elem){
			using namespace ui;
			auto& selector = creation::create_file_selector(creation::file_selector_create_info{elem, [](const creation::file_selector& s, const ui::elem&){
				return s.get_current_main_select().has_value();
			}, [](const creation::file_selector& s, const ui::elem&){
				return true;
			}, true});
			selector.set_cared_suffix({".png"});
		});
		pane.cell().region_scale = {tags::from_extent, math::vec2{}, math::vec2{.1f, 1.f}};
		pane.cell().align = align::pos::center_left;
		pane.cell().margin.set(4);
	}

	/*{
		auto pane = bed.emplace<game::ui::hit_box_editor>();
		pane.cell().region_scale = {tags::from_extent, math::vec2{}, math::vec2{.9f, 1.f}};
		pane.cell().align = align::pos::center_right;
		pane.cell().margin.set(4);
	}*/



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

auto projectile_meta = [](){
	using namespace mo_yanxi;

	return game::meta::projectile{
			.hitbox = {{game::meta::hitbox::comp{.box = {math::vec2{80, 10} * -0.5f, {80, 10}}}}},
			.rigid = {
				.drag = 0.001f
			},
			.damage = {.material_damage = 1000},
			.lifetime = 75,
			.initial_speed = 250,

			.trail_style = {
				game::meta::trail_style{
					.radius = 7,
					.color = {graphic::colors::aqua.to_light(), graphic::colors::aqua.to_light()},
					.trans = {-40}
				},
				50
			}
		};
}();

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

	auto& g = renderer_ui.batch.emplace_batch_layer<graphic::layers::grid_drawer>();
	g.data.current.chunk_size.set(100);
	g.data.current.solid_spacing.set(5);
	g.data.current.line_width = 3;
	g.data.current.main_line_width = 6;
	g.data.current.line_spacing = 15;
	g.data.current.line_gap = 5;
	g.data.current.main_line_color = graphic::colors::pale_green.copy().mul_rgb(.66f);
	g.data.current.line_color = graphic::colors::dark_gray;

	core::global::ui::root->add_scene(ui::scene{"main", new ui::loose_group{nullptr, nullptr}, &renderer_ui}, true);
	core::global::ui::root->resize(math::frect{math::vector2{context.get_extent().width, context.get_extent().height}.as<float>()});
	init_ui(core::global::ui::root->root_of<ui::loose_group>("main"), atlas);

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


	using grided_entity_desc = game::ecs::decl::grided_entity_desc;

	using projectile_entity_desc = game::ecs::decl::projectile_entity_desc;

	{



		{
			math::rand rand{};
			for(int i = 0; i < 10; ++i){
				using namespace game::ecs;

				manifold mf{};
				math::trans2 trs = {{rand(4000.f), rand(2000.f)}, rand(180.f)};
				mf.hitbox = game::hitbox{game::hitbox_comp{.box = {math::vec2{chamber::tile_size * 17, chamber::tile_size * 17}}}};

				faction_data faction_data{};
				if(i & 1){
					faction_data.faction = game::faction_0;
				}else{
					faction_data.faction = game::faction_1;
				}

				using build_tuple = std::tuple<chamber::building>;

				chamber::chamber_manifold grid{};
				grid.add_building<std::tuple<chamber::turret_build>>(
					{tags::from_extent, chamber::tile_coord{-8, -8}, 4, 17},
					chamber::turret_build{chamber::turret_meta{
						.range = {0, 3000},
						// .rotation_speed = ,
						// .default_angle = ,
						// .shooting_field_angle = ,
						// .shoot_cone_tolerance = ,
						.mode = {
							.reload_duration = 200,
							.burst_count = 3,
							.burst_spacing = 15,
							.inaccuracy = 6 * math::deg_to_rad,
							.projectile_type = &projectile_meta
						}
					}});
				grid.add_building<std::tuple<chamber::radar_build>>(
					{tags::from_extent, chamber::tile_coord{5, -8}, 4, 17}, chamber::radar_build{chamber::radar_meta{
						.local_center = {20, 20},
						.targeting_range = {400, 2000},
						.reload_duration = 120
					}});

				world.component_manager.create_entity_deferred<grided_entity_desc>(mech_motion{.trans = trs},
					std::move(mf),
					std::move(grid),
					std::move(faction_data),
					physical_rigid{
					.inertial_mass = 500000
				});
			}
		}

		wgfx_input.register_bind(core::ctrl::key::Q, core::ctrl::act::press, +[](core::ctrl::packed_key_t, math::vec2 pos, game::world::graphic_context& ctx, game::ecs::component_manager& component_manager){
			using namespace game::ecs;



			auto hdl = game::meta::create(component_manager, projectile_meta);

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
							math::interp::linear_map<0., .55f>
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

	core::global::timer.reset_time();
	while(!context.window().should_close()){
		context.window().poll_events();
		core::global::timer.fetch_time();
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


			math::rect_box_posed rect1{math::vec2{200, 300}, {0, 30}};
			{
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
			}


			acquirer.proj.depth = 0.35f;

			acquirer << graphic::draw::white_region;
			graphic::draw::fill::rect_ortho(
					acquirer.get(),
					{math::vec2{}, 40000},
					graphic::colors::black
				);

			bool is_intersected = rect1.overlap_exact(rect2);

			graphic::draw::line::quad(
					acquirer,
					rect2.view_as_quad(),
					2.f,
					is_intersected ? graphic::colors::pale_green : graphic::colors::white
				);

			for(int i = 0; i < 4; ++i){
				auto v0 = rect2[i];
				auto v1 = rect2[i + 1];
				auto n = rect2.edge_normal_at(i);

				graphic::draw::line::line(
					acquirer.get(),
					(v0 + v1) / 2,
					(v0 + v1) / 2 + n * 16,
					3,
					graphic::colors::PURPLE,
					graphic::colors::PURPLE
				);
			}

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
						                       (colors::red_dusted.create_lerp(colors::pale_green, tile.building.data().hit_point.get_functionality_factor())).to_light(1.5f).set_a(
							                       tile.get_status().valid_hit_point / tile.building.data().get_tile_individual_max_hitpoint()));
					});

					grid.manager.sliced_each([&](const chamber::building_data& building_data){
						draw::line::rect_ortho(acquirer, building_data.get_bound(), 3, colors::CRIMSON.to_light());
					});

					auto local_rect = grid.box_to_local(static_cast<const math::rect_box<float>&>(rect2));

					bool any{};
					grid.local_grid.quad_tree()->intersect_then(
						local_rect, [](const math::rect_box<float>& rect, math::frect region){
							return rect.overlap_rough(region) && rect.overlap_exact(region);
						}, [&](const auto& rect, const chamber::tile& tile){
							any = true;
							// draw::fill::rect_ortho(acquirer.get(), tile.get_bound(),
							//                        colors::ACID.to_light().set_a(.3f));
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

			if(!core::global::timer.is_paused()){
				game::ecs::system::chamber{}.run(world);
				motion_system.run(world.component_manager);
				game::ecs::system::projectile{}.run(world.component_manager, world.graphic_context);
				world.collision_system.run_collision_test(world.component_manager);
			}

			ecs_renderer.filter_screen_space(world.component_manager, renderer_world.camera.get_viewport());
			ecs_renderer.draw(world.graphic_context);

			world.component_manager.do_deferred();
		}

		world.graphic_context.render_efx();

		renderer_world.batch.batch.consume_all();
		renderer_world.post_process();

		renderer_ui.post_process();

		merger.submit();
		renderer_ui.clear();

		context.flush();
	}

	context.wait_on_device();


}

int main(){
	using namespace mo_yanxi;
	using namespace mo_yanxi::game;


	// test::foot();
	//
	// math::rect_box_posed rect{math::vec2{300, 100}, {125, 44, 3.14 / 4}};
	// // auto pkg = io::loader<decltype(rect)>::pack(rect);
	// // auto rect2 = io::loader<decltype(rect)>::extract(pkg);
	//
	// game::hitbox_meta meta{
	// 	{
	// 		hitbox_meta::meta{rect.get_identity(), rect.deduce_transform()},
	// 		hitbox_meta::meta{{-10, -20, 20, 40}, {1, 2, 3}},
	// 	}
	// };
	//
	// auto pkg = io::pack(meta);
	//
	// auto meta2 = io::extract<hitbox_meta>(pkg);

	// std::vector<std::vector<math::vec2>> vec{{{0, 1,}, {2, 3}}, {{4, 5}}};

	// std::println("{}", vec);

	// foo2();



	init_assets();
	compile_shaders();

	core::glfw::init();
	core::global::graphic::init_vk();
	core::global::assets::init(&core::global::graphic::context);
	assets::graphic::load(core::global::graphic::context);
	core::global::ui::init();

	main_loop();

	core::global::ui::dispose();
	assets::graphic::dispose();
	core::global::assets::dispose();
	core::global::graphic::dispose();
	core::glfw::terminate();
}
