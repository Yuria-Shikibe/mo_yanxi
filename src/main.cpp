#include <vulkan/vulkan.h>
#include <cassert>

// #define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include <freetype/freetype.h>
#include <msdfgen/ext/import-font.h>

import std;

import mo_yanxi.graphic.bitmap;

import mo_yanxi.math;
import mo_yanxi.math.angle;
import mo_yanxi.math.vector2;
import mo_yanxi.math.vector4;

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
import mo_yanxi.graphic.draw;
import mo_yanxi.graphic.draw.func;
import mo_yanxi.graphic.draw.multi_region;
import mo_yanxi.graphic.draw.transparent;
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

import test;

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

	ui::elem_ptr e{root.get_scene(), &root, std::in_place_type<ui::manual_table>};
	e->property.fill_parent = {true, true};
	auto& bed = static_cast<ui::manual_table&>(root.add_children(std::move(e)));

	auto spane = bed.emplace<ui::scroll_pane>();
	spane.cell().region_scale = {math::vec2{}, math::vec2{.5, .8}};
	spane.cell().align = align::pos::top_left;
	spane->set_layout_policy(ui::layout_policy::hori_major);

	auto nscene = bed.emplace<ui::nested_scene>();
	nscene.cell().region_scale = {tags::from_extent, math::vec2{}, math::vec2{.5, 1}};
	nscene.cell().align = align::pos::center_right;
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

	vk::load_ext(context.get_instance());

	graphic::renderer_export exports;
	graphic::renderer_world renderer_world{context, exports};
	graphic::renderer_ui renderer_ui{context, exports};

	core::global::ui::root->add_scene(ui::scene{"main", new ui::loose_group{nullptr, nullptr}, &renderer_ui}, true);
	core::global::ui::root->resize(math::frect{math::vector2{context.get_extent().width, context.get_extent().height}.as<float>()}.shrink(8));
	init_ui(core::global::ui::root->root_of<ui::loose_group>("main"), atlas);

	context.register_post_resize("test", [&](window_instance::resize_event event){
		core::global::ui::root->resize(math::frect{math::vector2{event.size.width, event.size.height}.as<float>()}.shrink(8));

		core::global::camera.resize_screen(event.size.width, event.size.height);

		renderer_world.resize(event.size);
		renderer_ui.resize(event.size);

		context.set_staging_image({
			                          .image = exports.results[renderer_ui.get_name()].image,
			                          .extent = event.size,
			                          .clear = false,
			                          .owner_queue_family = context.compute_family(),
			                          .src_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			                          .src_access = VK_ACCESS_2_SHADER_WRITE_BIT,
			                          .dst_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			                          .dst_access = VK_ACCESS_2_SHADER_WRITE_BIT,
			                          .src_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			                          .dst_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		                          }, false);
	});

	core::global::camera.resize_screen(context.get_extent().width, context.get_extent().height);
	context.set_staging_image({
		.image = exports.results[renderer_ui.get_name()].image,
		.extent = context.get_extent(),
		.clear = false,
		.owner_queue_family = context.compute_family(),
		.src_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		.src_access = VK_ACCESS_2_SHADER_WRITE_BIT,
		.dst_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		.dst_access = VK_ACCESS_2_SHADER_WRITE_BIT,
		.src_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.dst_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	});

	graphic::borrowed_image_region light_region = main_page.register_named_region("pester.light", graphic::bitmap{R"(D:\projects\mo_yanxi\prop\assets\texture\pester.light.png)"}).first;
	graphic::borrowed_image_region base_region = main_page.register_named_region("pester", graphic::bitmap{R"(D:\projects\mo_yanxi\prop\assets\texture\pester.png)"}).first;
	graphic::borrowed_image_region base_region2 = main_page.register_named_region("pesterasd", graphic::bitmap{R"(D:\projects\mo_yanxi\prop\CustomUVChecker_byValle_1K.png)"}).first;

	while(!context.window().should_close()){
		context.window().poll_events();
		core::global::timer.fetch_time();
		core::global::input.update(core::global::timer.global_delta_tick());
		core::global::camera.update(core::global::timer.global_delta_tick());
		renderer_world.batch.frag_data.current.enable_depth = true;
		renderer_world.batch.update_proj({core::global::camera.get_world_to_uniformed_flip_y()});

		renderer_world.ssao.set_scale(core::global::camera.map_scale(0.15f, 2.5f) * 1.5f);
		renderer_world.bloom.set_scale(core::global::camera.map_scale(0.195f, 2.f));
		renderer_world.batch.frag_data.current.camera_scale = core::global::camera.get_scale();


		renderer_ui.set_time(core::global::timer.global_time());
		core::global::ui::root->update(core::global::timer.global_delta_tick());
		core::global::ui::root->layout();

		core::global::ui::root->draw();



		// renderer_ui.batch.pop_scissor();

		// {
		// 	graphic::draw::world_acquirer acquirer{renderer_world.batch, static_cast<const graphic::combined_image_region<graphic::uniformed_rect_uv>&>(base_region)};
		// 	//
		// 	for(int i = 0; i < 10; ++i){
		// 		acquirer.proj.depth = static_cast<float>(i + 1) / 20.f - 0.04f;
		//
		// 		auto off = math::vector2{i * 500, i * 400}.as<float>();
		//
		// 		graphic::draw::fill::quad(
		// 			acquirer.get(),
		// 			off - math::vec2{100, 500} + math::vec2{0, 0}.rotate(45.f),
		// 			off - math::vec2{100, 500} + math::vec2{500 + i * 200.f, 0}.rotate(45.f),
		// 			off - math::vec2{100, 500} + math::vec2{500 + i * 200.f, 5 + i * 100.f}.rotate(45.f),
		// 			off - math::vec2{100, 500} + math::vec2{0, 5 + i * 100.f}.rotate(45.f),
		// 			graphic::colors::white
		// 		);
		// 	}
		// }
		//
		//
		// renderer_world.batch.batch.consume_all();
		//
		// {
		// 	graphic::draw::world_acquirer<> acquirer{renderer_world.batch, static_cast<const graphic::combined_image_region<graphic::uniformed_rect_uv>&>(light_region)};
		// 	//
		// 	for(int i = 0; i < 10; ++i){
		// 		acquirer.proj.depth = static_cast<float>(i + 1) / 20.f - 0.04f;
		//
		// 		auto off = math::vector2{i * 500, i * 400}.as<float>();
		//
		// 		graphic::draw::fill::quad(
		// 			acquirer.get(),
		// 			off - math::vec2{100, 500} + math::vec2{0, 0}.rotate(45.f),
		// 			off - math::vec2{100, 500} + math::vec2{500 + i * 200.f, 0}.rotate(45.f),
		// 			off - math::vec2{100, 500} + math::vec2{500 + i * 200.f, 5 + i * 100.f}.rotate(45.f),
		// 			off - math::vec2{100, 500} + math::vec2{0, 5 + i * 100.f}.rotate(45.f),
		// 			graphic::colors::white.copy().to_light_color()
		// 		);
		// 	}
		// }

		renderer_world.batch.batch.consume_all();
		renderer_world.batch.consume_all_transparent();
		renderer_world.post_process();

		renderer_ui.batch.batch.consume_all();
		renderer_ui.batch.blit();
		renderer_ui.post_process();


		context.flush();
	}

	vkDeviceWaitIdle(context.get_device());


}

int main(){
	using namespace mo_yanxi;

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
