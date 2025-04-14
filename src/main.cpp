#include <vulkan/vulkan.h>
#include <cassert>

// #define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include <freetype/freetype.h>

#include <gch/small_vector.hpp>

import std;

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

import mo_yanxi.graphic.renderer;
import mo_yanxi.graphic.renderer.world;
import mo_yanxi.graphic.renderer.ui;
import mo_yanxi.graphic.renderer.merger;
import mo_yanxi.graphic.draw;
import mo_yanxi.graphic.draw.func;
import mo_yanxi.graphic.draw.multi_region;
import mo_yanxi.graphic.image_atlas;
import mo_yanxi.graphic.image_nine_region;

import mo_yanxi.font;
import mo_yanxi.font.manager;
import mo_yanxi.font.typesetting;

import mo_yanxi.graphic.msdf;

import mo_yanxi.ui.root;
import mo_yanxi.ui.scene;
import mo_yanxi.ui.loose_group;
import mo_yanxi.ui.manual_table;
import mo_yanxi.ui.table;
import mo_yanxi.ui.elem.text_elem;
import mo_yanxi.ui.scroll_pane;
import mo_yanxi.ui.elem.text_input_area;
import mo_yanxi.ui.elem.slider;
import mo_yanxi.ui.elem.image_frame;
import mo_yanxi.ui.elem.progress_bar;
import mo_yanxi.ui.elem.collapser;
import mo_yanxi.ui.elem.button;
import mo_yanxi.ui.elem.check_box;
import mo_yanxi.ui.elem.nested_scene;

import mo_yanxi.game.graphic.effect;
import mo_yanxi.game.world.graphic;

import mo_yanxi.game.ecs.component_manager;
import mo_yanxi.game.ecs.task_graph;
import mo_yanxi.game.ecs.dependency_generator;

import mo_yanxi.game.ecs.component_operation_task_graph;
import mo_yanxi.game.ecs.component.manifold;
import mo_yanxi.game.ecs.component.hitbox;
import mo_yanxi.game.ecs.component.physical_property;
import mo_yanxi.game.ecs.system.collision;
import mo_yanxi.game.ecs.system.motion_system;


import test;
import hive;

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

	auto& svg_up = ui_page.register_named_region(
		"svg_up"s,
		graphic::msdf::msdf_generator{R"(D:\projects\mo_yanxi\prop\assets\svg\up.svg)"}, {100, 100}).first;
	auto& svg_down = ui_page.register_named_region(
		"svg_down"s,
		graphic::msdf::msdf_generator{R"(D:\projects\mo_yanxi\prop\assets\svg\down.svg)"}, {100, 100}).first;


	auto& test_svg = ui_page.register_named_region(
		"test"s,
		graphic::msdf::msdf_generator{R"(D:\projects\mo_yanxi\prop\assets\svg\blender_icon_select_subtract.svg)"}, {100, 100}).first;
	auto& line_svg = ui_page.register_named_region(
		"line"s,
		graphic::msdf::msdf_generator{R"(D:\projects\mo_yanxi\prop\assets\svg\line.svg)"}, {40u, 24u}).first;


	ui_page.mark_protected("line");

	graphic::image_caped_region region{line_svg, line_svg.get_region(), 12, 12, graphic::msdf::sdf_image_boarder};

	root.property.set_empty_drawer();
	root.skip_inbound_capture = true;

	ui::elem_ptr e{root.get_scene(), &root, std::in_place_type<ui::manual_table>};
	e->property.set_empty_drawer();
	e->property.fill_parent = {true, true};
	e->skip_inbound_capture = true;
	auto& bed = static_cast<ui::manual_table&>(root.add_children(std::move(e)));

	// auto spane = bed.emplace<ui::scroll_pane>();
	// spane.cell().region_scale = {math::vec2{}, math::vec2{.5, .8}};
	// spane.cell().align = align::pos::top_left;
	// spane->set_layout_policy(ui::layout_policy::hori_major);

	auto nscene = bed.emplace<ui::nested_scene>();
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
				img.set_drawable<ui::image_drawable>(0, graphic::draw::mode_flags::sdf, svg_up);
				img.set_drawable<ui::image_drawable>(1, graphic::draw::mode_flags::sdf, svg_down);
				img.add_collapser_image_swapper();
			}).cell().set_width(60);
			collapser->head().function_init([&](ui::basic_text_elem& txt){
				txt.property.set_empty_drawer();
				txt.set_text("#<[segui>Collapser Pane");
				txt.set_policy(font::typesetting::layout_policy::auto_feed_line);
			}).cell().set_external({false, true}).pad.left = 12;

			collapser->content().function_init([&](ui::check_box& img){
				img.set_drawable<ui::image_drawable>(0, graphic::draw::mode_flags::sdf, test_svg);
				img.set_drawable<ui::image_drawable>(1, graphic::draw::mode_flags::sdf, svg_up);
				img.set_drawable<ui::image_drawable>(2, graphic::draw::mode_flags::sdf, svg_down);
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
			button->set_drawable<ui::image_drawable>(graphic::draw::mode_flags::sdf, test_svg);
			button.cell().set_size(60);

			auto bar = table.emplace<ui::progress_bar>();
			bar->reach_speed = .05f;
			bar.cell().set_height(60);


			bar->set_progress_prov([&e = slider.elem()] -> ui::progress_bar_progress {
				return {e.get_progress().x};
			});
		}

		table.set_edge_pad(0);

	}
}

constexpr mo_yanxi::game::fx::line_splash f{
	.count = 150,
	.range = {260, 620},
	.stroke = {{2, 0}, {4, 0}},
	.length = {{30, 20}, {30, 100, mo_yanxi::math::interp::slope}}
};

void main_loop(){
	using namespace mo_yanxi;

	auto& context = core::global::graphic::context;

	graphic::image_atlas atlas{context};
	graphic::image_page& main_page = atlas.create_image_page("main");

	font::font_manager font_manager{atlas};
	{
		auto p = font_manager.page().register_named_region("white", graphic::bitmap{R"(D:\projects\mo_yanxi\prop\assets\texture\white.png)"});
		p.first.uv.shrink(64);
		font_manager.page().mark_protected("white");

		graphic::draw::white_region = p.first;

		auto& face_tele = font_manager.register_face("tele", R"(D:\projects\mo_yanxi\prop\assets\fonts\telegrama.otf)");
		auto& face_srch = font_manager.register_face("srch", R"(D:\projects\mo_yanxi\prop\assets\fonts\SourceHanSerifSC-SemiBold.otf)");
		auto& face_timesi = font_manager.register_face("timesi", R"(D:\projects\mo_yanxi\prop\assets\fonts\timesi.ttf)");
		auto& face_ui = font_manager.register_face("segui", R"(D:\projects\mo_yanxi\prop\assets\fonts\seguisym.ttf)");

		face_tele.fallback = &face_srch;
		face_srch.fallback = &face_ui;
		font::typesetting::default_font_manager = &font_manager;
		font::typesetting::default_font = &face_tele;
	}

	test::load_tex(atlas);

	font::typesetting::parser parser{};


	core::global::graphic::post_init();

	auto& renderer_world = core::global::graphic::world;
	auto& renderer_ui = core::global::graphic::ui;
	auto& merger = core::global::graphic::merger;

	game::world::graphic_context graphic_context{renderer_world};

	core::global::ui::root->add_scene(ui::scene{"main", new ui::loose_group{nullptr, nullptr}, &renderer_ui}, true);
	core::global::ui::root->resize(math::frect{math::vector2{context.get_extent().width, context.get_extent().height}.as<float>()});
	init_ui(core::global::ui::root->root_of<ui::loose_group>("main"), atlas);

	game::ecs::component_manager component_manager{};

	auto& wgfx_input =
		core::global::input.register_sub_input<
			math::vec2,
			game::world::graphic_context&,
			game::ecs::component_manager&
				>("gfx");
	wgfx_input.set_context(math::vec2{}, std::ref(graphic_context), std::ref(component_manager));

	{
		using namespace core::ctrl;
		using namespace game;
		wgfx_input.register_bind(mouse::_1, act::press, +[](packed_key_t, math::vec2 pos, world::graphic_context& ctx, game::ecs::component_manager&){
			const auto wpos = ctx.renderer().camera.get_screen_to_world(pos, {}, true);

			ctx.create_efx().set_data({
				.style = fx::line_splash{
					.count = 50,
					.distribute_angle = 15,
					.range = {260, 820},
					.stroke = {{2, 0}, {6, 0}},
					.length = {{30, 20}, {30, 480}},

					.palette = {
						{
							graphic::colors::white.create_lerp(graphic::colors::ORANGE, .65f).to_light().set_a(.5),
							graphic::colors::white.to_light().set_a(0),
							math::interp::linear_map<0., .55f>
						}, {
							graphic::colors::AQUA.to_light().set_a(0.95f),
							graphic::colors::white.to_light().set_a(0),
							math::interp::pow3In | math::interp::reverse
						}
					}
				},
				.trans = {wpos, 45},
				.depth = 0,
				.duration = {30}
				});

		// 	ctx.create_efx().set_data({
		// 		.style = fx::poly_outlined_out{
		// 			.radius = {30, 550, math::interp::pow3Out},
		// 			.stroke = {4},
		// 			.palette = {
		// 				{graphic::colors::AQUA.to_light()},
		// 				{graphic::colors::AQUA.to_light()},
		// 				{graphic::colors::clear},
		// 				{
		// 					graphic::colors::AQUA.create_lerp(graphic::colors::white, .5f).to_light().set_a(.5f),
		// 					graphic::colors::clear,
		// 				}
		// 			}
		// 		},
		// 		.trans = wpos,
		// 		.depth = 0,
		// 		.duration = {40}
		// 	});
		});
	}


	{
		context.register_post_resize("test", [&](window_instance::resize_event event){
		   core::global::ui::root->resize(
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

	graphic::borrowed_image_region light_region = main_page.register_named_region("pester.light", graphic::bitmap{R"(D:\projects\mo_yanxi\prop\assets\texture\pester.light.png)"}).first;
	graphic::borrowed_image_region base_region = main_page.register_named_region("pester", graphic::bitmap{R"(D:\projects\mo_yanxi\prop\assets\texture\pester.png)"}).first;
	graphic::borrowed_image_region base_region2 = main_page.register_named_region("pesterasd", graphic::bitmap{R"(D:\projects\mo_yanxi\prop\CustomUVChecker_byValle_1K.png)"}).first;


	game::ecs::collision_system collision_system{};
	game::ecs::motion_system motion_system{};

	using entity_desc = std::tuple<game::ecs::mech_motion, game::ecs::manifold, game::ecs::physical_rigid>;
	component_manager.add_archetype<entity_desc>();
	{
		math::rand rand{};
		for(int i = 0; i < 1000; ++i){
			using namespace game::ecs;

			manifold mf{};
			math::trans2 trs = {{rand(10000.f), rand(10000.f)}, rand(180.f)};
			mf.hitbox = game::hitbox{game::hitbox_comp{.box = {math::vec2{rand(0, 300.f), rand(0, 300.f)}}}, trs};
			component_manager.create_entity_deferred<entity_desc>(mech_motion{.trans = trs}, std::move(mf));
		}
	}

	wgfx_input.register_bind(core::ctrl::key::F, core::ctrl::act::press, +[](core::ctrl::packed_key_t, math::vec2 pos, game::world::graphic_context& ctx, game::ecs::component_manager& component_manager){
		using namespace game::ecs;

		math::rand rand{};

		auto dir =
			(pos.scl(1, -1).add_y(ctx.renderer().camera.get_screen_size().y) - ctx.renderer().camera.get_screen_center())
			.set_length(rand(80, 800));
		manifold mf{};
		math::trans2 trs = {ctx.renderer().camera.get_stable_center(), dir.angle()};
		mf.hitbox = game::hitbox{game::hitbox_comp{.box = {math::vec2{200, 40}}}, trs};
		component_manager.create_entity_deferred<entity_desc>(mech_motion{.trans = trs, .vel = dir}, std::move(mf));
	});

	core::global::timer.reset_time();
	while(!context.window().should_close()){
		context.window().poll_events();
		core::global::timer.fetch_time();
		core::global::input.update(core::global::timer.global_delta_tick());

		component_manager.update_delta = core::global::timer.global_delta_tick();
		graphic_context.update(core::global::timer.global_delta_tick());
		renderer_ui.set_time(core::global::timer.global_time());

		wgfx_input.set_context(core::global::ui::root->focus->cursor_pos);


		core::global::ui::root->update(core::global::timer.global_delta_tick());
		core::global::ui::root->layout();
		core::global::ui::root->draw();

		graphic::draw::world_acquirer acquirer{renderer_world.batch, static_cast<const graphic::combined_image_region<graphic::uniformed_rect_uv>&>(base_region)};
		{


			math::rect_box_posed rect1{math::vec2{200, 300}, {0, 30}};
			math::rect_box_posed rect2{math::vec2{200, 300}};

			{
				//
				for(int i = 0; i < 10; ++i){
					acquirer << base_region;
					acquirer.proj.depth = 1 - (static_cast<float>(i + 1) / 10.f - 0.04f);

					auto off = math::vector2{i * 500, i * 400}.as<float>();

					using namespace graphic::colors;

					graphic::draw::fill::quad(
						acquirer.get(),
						off - math::vec2{100, 500} + math::vec2{0, 0}.rotate(45.f),
						off - math::vec2{100, 500} + math::vec2{500 + i * 200.f, 0}.rotate(45.f),
						off - math::vec2{100, 500} + math::vec2{500 + i * 200.f, 5 + i * 100.f}.rotate(45.f),
						off - math::vec2{100, 500} + math::vec2{0, 5 + i * 100.f}.rotate(45.f),
						white.copy()
					);

					acquirer << light_region;
					acquirer.proj.slightly_decr_depth();
					graphic::draw::fill::quad(
						acquirer.get(),
						off - math::vec2{100, 500} + math::vec2{0, 0}.rotate(45.f),
						off - math::vec2{100, 500} + math::vec2{500 + i * 200.f, 0}.rotate(45.f),
						off - math::vec2{100, 500} + math::vec2{500 + i * 200.f, 5 + i * 100.f}.rotate(45.f),
						off - math::vec2{100, 500} + math::vec2{0, 5 + i * 100.f}.rotate(45.f),
						(white.copy() * 2).set_a((i + 3) / 10.f)
					);
				}
			}

			rect2.update({
				renderer_world.camera.get_screen_to_world(core::global::ui::root->focus->cursor_pos, {}, true),
				45});

			acquirer.proj.depth = 0.35f;

			graphic::draw::fill::rect_ortho(
					acquirer.get(),
					{-1000, -1000, 200, 200},
					graphic::colors::AQUA.copy().set_a(.5)
				);

			acquirer << graphic::draw::white_region;
			graphic::draw::fill::rect_ortho(
					acquirer.get(),
					{math::vec2{}, 40000},
					graphic::colors::black
				);

			bool is_intersected = rect1.overlap_exact(rect2);

			graphic::draw::fill::quad(
					acquirer.get(),
					rect1.view_as_quad(),
					is_intersected ? graphic::colors::PALE_GREEN : graphic::colors::white
				);

			graphic::draw::fill::quad(
					acquirer.get(),
					rect2.view_as_quad(),
					is_intersected ? graphic::colors::PALE_GREEN : graphic::colors::white
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

			if(is_intersected){
				auto vec_depart_vector = rect2.depart_vector_to_on_vel(rect1, math::vec2::from_polar(0., 1));
				auto min_depart_vector = rect2.depart_vector_to(rect1);
				auto p = rect2.avg();

				graphic::draw::line::line(
					acquirer.get(),
					p,
					p + vec_depart_vector,
					3,
					graphic::colors::AQUA,
					graphic::colors::AQUA
				);
				graphic::draw::line::line(
					acquirer.get(),
					p,
					p + min_depart_vector,
					3,
					graphic::colors::CRIMSON,
					graphic::colors::CRIMSON
				);
			}

			acquirer.proj.slightly_decr_depth();
			graphic::draw::fill::rect_ortho(
					acquirer.get(),
					{math::vec2{}, 30},
					graphic::colors::AQUA_SKY.to_light()
				);
		}

		component_manager.do_deferred();
		component_manager.sliced_each([&](
			game::ecs::component<game::ecs::manifold>& manifold,
			game::ecs::component<game::ecs::mech_motion>& motion
			){

			manifold->hitbox.update_hitbox_with_ccd(motion->trans);

			if(!renderer_world.camera.get_viewport().overlap_exclusive(manifold->hitbox.max_wrap_bound()))return;

			using namespace graphic;
			for (const auto & comp : manifold->hitbox.get_comps()){
				draw::line::quad(acquirer, comp.box, 2, colors::white.to_light());
				draw::line::rect_ortho(acquirer, comp.box.get_bound(), 1, colors::CRIMSON);
			}
			draw::line::quad(acquirer, manifold->hitbox.ccd_wrap_box(), 1, colors::PALE_GREEN);
		});

		collision_system.insert_all(component_manager);
		collision_system.run(component_manager);
		motion_system.run(component_manager);

		graphic_context.render_efx();

		renderer_world.batch.batch.consume_all();
		// renderer_world.batch.consume_all_transparent();
		renderer_world.post_process();

		renderer_ui.batch.batch.consume_all();
		renderer_ui.batch.blit();
		renderer_ui.post_process();

		merger.submit();


		context.flush();
	}

	vkDeviceWaitIdle(context.get_device());


}

void foo();

int main(){
	using namespace mo_yanxi;
	using namespace mo_yanxi::game;



	// foo();

	init_assets();
	compile_shaders();

	core::glfw::init();
	core::global::graphic::init();
	core::global::ui::init();
	assets::graphic::load(core::global::graphic::context);

	main_loop();

	assets::graphic::dispose();
	core::global::ui::dispose();
	core::global::graphic::dispose();
	core::glfw::terminate();
}

void foo(){
	using namespace mo_yanxi;
	using namespace mo_yanxi::game;

	ecs::component_manager cpmg{};
	ecs::component_operation_task_graph task_graph{cpmg};

	using entity_tuple_1 = std::tuple<math::trans2, math::vec2>;
	using entity_tuple_2 = std::tuple<math::trans2, math::vec2, int>;
	using entity_tuple_3 = std::tuple<math::vec2>;
	using entity_tuple_4 = std::tuple<int>;

	cpmg.add_archetype<
		entity_tuple_1,
		entity_tuple_2,
		entity_tuple_3,
		entity_tuple_4
	>();


	cpmg.create_entity_deferred<entity_tuple_1>(math::vec2{2, 2});

	cpmg.do_deferred();

	cpmg.sliced_each([](const ecs::component<math::vec2>& vecs){
		std::print("{} ", vecs.val());
	});
	// cpmg.create_entity<entity_tuple_1>();
	// cpmg.create_entity<entity_tuple_1>();
	// cpmg.create_entity<entity_tuple_1>();
	// cpmg.create_entity<entity_tuple_1>();
	// cpmg.create_entity<entity_tuple_2>();
	// cpmg.create_entity<entity_tuple_2>();
	// cpmg.create_entity<entity_tuple_2>();
	// cpmg.create_entity<entity_tuple_2>();
	// cpmg.create_entity<entity_tuple_2>();
	// cpmg.create_entity<entity_tuple_3>();
	// cpmg.create_entity<entity_tuple_3>();
	// auto ent = cpmg.create_entity<entity_tuple_2>();
	// auto en2t = cpmg.create_entity<entity_tuple_2>();
	// cpmg.create_entity<entity_tuple_4>();
	// cpmg.create_entity<entity_tuple_4>();
	// cpmg.create_entity<entity_tuple_4>();
	//
	// if(auto data = cpmg.get_entity_partial_chunk<math::vec2>(ent)){
	// 	data->val().set_polar(45.f, math::sqrt2);
	// }
	//
	// std::println();
	//
	// enum task_id{
	// 	t1, t2, t3,
	//
	// 	max
	// };
	//
	// task_graph.reserve_event_id(task_id::max);
	//
	// auto m0 = task_graph.run({}, {}, [](ecs::component<math::vec2>& comp){
	// 	comp->add(1, 2);
	// });
	//
	// task_graph.run(t1, {m0}, [](ecs::component<math::vec2>& comp){
	// 	comp->add(1, 2);
	// });
	//
	// task_graph.run(t2, {t1}, [](ecs::component<math::vec2>& comp){
	// 	comp->mul(1, 2);
	// });
	//
	// task_graph.run(t3, {t2}, [](const ecs::component<math::vec2>& comp){
	// 	std::print("{} ", comp.val());
	// });

	// auto beg = std::chrono::high_resolution_clock::now();
	//
	// for(int i = 0; i < 10'000'000; ++i){
	// 	auto& a = ent->unchecked_get<entity_tuple_2, math::vec2>();
	// 	// auto* a = cpmg.get_entity_partial_chunk<math::vec2>(ent);
	//
	// 	a->add(1, 2);
	// }
	// // task_graph.generate_dependencies();
	// // auto group = task_graph.get_sorted_task_group();
	// // auto instance = group.create_instance();
	//
	// auto end = std::chrono::high_resolution_clock::now();
	// //
	// std::println("Task Generate Cost: {}us", std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count());
}