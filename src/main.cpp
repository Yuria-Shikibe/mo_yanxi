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

import mo_yanxi.graphic.renderer;
import mo_yanxi.graphic.renderer.world;
import mo_yanxi.graphic.renderer.ui;
import mo_yanxi.graphic.draw;
import mo_yanxi.graphic.draw.func;
import mo_yanxi.graphic.draw.nine_patch;
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

import test;

void draw_glyph_layout(
	mo_yanxi::graphic::renderer_world& renderer,
	const mo_yanxi::font::typesetting::glyph_layout& layout,
	const mo_yanxi::math::vec2 offset,
	bool toLight,
	float opacityScl = 1.f){
	using namespace mo_yanxi;
	using namespace mo_yanxi::graphic;
	color tempColor{};



	draw::default_transparent_acquirer acquirer{renderer.batch, {}};

	const font::typesetting::glyph_elem* last_elem{};

	for (const auto& row : layout.rows()){
		const auto lineOff = row.src + offset;

		// acquirer << draw::white_region;
		//
		// draw::line::rect_ortho(acquirer, row.bound.to_region(lineOff));
		// draw::line::line_ortho(acquirer.get(), lineOff, lineOff.copy().add_x(row.bound.width));

		for (auto && glyph : row.glyphs){
			if(!glyph.glyph)continue;
			last_elem = &glyph;

			acquirer << glyph.glyph.get_cache();

			tempColor = glyph.color;

			if(opacityScl != 1.f){
				tempColor.a *= opacityScl;
			}

			if(glyph.code.code == U'\0'){
				tempColor.mul_rgb(.65f);
			}


			draw::fill::quad(
				acquirer.get(),
				lineOff + glyph.region.vert_00(),
				lineOff + glyph.region.vert_10(),
				lineOff + glyph.region.vert_11(),
				lineOff + glyph.region.vert_01(),
				tempColor.to_light_color_copy(toLight)
			);

			// acquirer << draw::white_region;
			// draw::line::rect_ortho(acquirer, glyph.region.copy().move(lineOff));

		}


	}


	if(layout.is_clipped() && last_elem){
		const auto lineOff = layout.rows().back().src + offset;

		draw::fill::fill(
			acquirer[-1],
			lineOff + last_elem->region.vert_00(),
			lineOff + last_elem->region.vert_10(),
			lineOff + last_elem->region.vert_11(),
			lineOff + last_elem->region.vert_01(),
			last_elem->color.copy().mulA(opacityScl).to_light_color_copy(toLight),
			{},
			{},
			last_elem->color.copy().mulA(opacityScl).to_light_color_copy(toLight)
		);
	}

	acquirer << draw::white_region;
	//
	draw::line::rect_ortho(acquirer, mo_yanxi::math::frect{mo_yanxi::tags::from_extent, offset, layout.extent()});
}


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

void init_ui(mo_yanxi::ui::loose_group& root){
	using namespace mo_yanxi;
	ui::elem_ptr e{root.get_scene(), &root, std::in_place_type<ui::manual_table>};
	e->property.fill_parent = {true, true};
	auto& bed = static_cast<ui::manual_table&>(root.addChildren(std::move(e)));

	auto spane = bed.emplace<ui::scroll_pane>();
	spane.cell().region_scale = {math::vec2{}, math::vec2{.5, .8}};
	spane.cell().align = align::pos::top_left;

	spane->set_layout_policy(ui::layout_policy::horizontal);
	auto& table = spane->emplace<ui::table>();

	table.template_cell.pad.set(2);
	{
		auto ehdl = table.emplace<ui::elem>();
		ehdl.cell().set_width(60);
		// ehdl.cell().set_height_from_scale(1.);
	}

	{
		auto text = table.emplace<ui::basic_text_elem>();
		text.cell().set_external({true, true});
		text->set_text("#<[32>dinasfas;lhfasdaahfasfa\n\n\ngdongji");
		text->set_policy(font::typesetting::layout_policy::auto_feed_line);
	}

	{
		auto ehdl = table.end_line().emplace<ui::elem>();
		// ehdl.cell().set_width(60);
		// ehdl.cell().set_height_from_scale(1.);
	}
	{
		auto text = table.emplace<ui::basic_text_elem>();
		text.cell().set_external({true, true});
		text->set_text("#<[32>dinasfas;bbbbbbbb#<[#ffff00bb>bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
		text->set_policy(font::typesetting::layout_policy::auto_feed_line);
	}

	{
		auto ehdl = table.end_line().emplace<ui::elem>();
		// ehdl.cell().set_width(80);
		// ehdl.cell().set_height_from_scale(1.);
	}
	{
		auto text = table.emplace<ui::basic_text_elem>();
		text.cell().set_external({true, true});
		text->set_text("#<[55>;bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
		text->set_policy(font::typesetting::layout_policy::auto_feed_line);
	}

	{
		auto ehdl = table.end_line().emplace<ui::elem>();
		// ehdl.cell().set_width(80);
		// ehdl.cell().set_height_from_scale(1.);
	}

	{
		auto text = table.emplace<ui::table>();
		text.cell().set_external({false, true});
		text->function_init([](ui::basic_text_elem& text){
			text.set_text("楼上的下来搞核算");
			text.set_policy(font::typesetting::layout_policy::auto_feed_line);
		}).cell().set_external({false, true});
		text->end_line().function_init([](ui::basic_text_elem& text){
			text.set_text("叮咚鸡叮咚鸡");
			text.set_policy(font::typesetting::layout_policy::auto_feed_line);
		}).cell().set_external({false, true});

		text->end_line().function_init([](ui::basic_text_elem& text){
			text.set_text("大狗大狗叫叫叫");
			text.set_policy(font::typesetting::layout_policy::auto_feed_line);
		}).cell().set_external({false, true});
	}

	// {
	// 	auto hdl = bed.function_init([](ui::scroll_pane& t){
	// 		t.set_layout_policy(ui::layout_policy::horizontal);
	// 		t.set_elem([](ui::table& table){
	//
	// 			auto ehdl = table.emplace<ui::basic_text_elem>();
	//
	//
	// 		});
	// 	});
	// 	hdl.cell().region_scale = {math::vec2{}, math::vec2{.5, .8}};
	// 	hdl.cell().align = align::pos::top_left;
	// }
	//
	// {
	// 	auto hdl = bed.emplace<ui::table>();
	// 	hdl.cell().region_scale = {math::vec2{.1, .1}, math::vec2{.5, .8}};
	// 	hdl.cell().align = align::pos::bottom_right;
	// 	{
	// 		auto ehdl = hdl.elem().emplace<ui::basic_text_elem>();
	// 		ehdl.cell().set_external({1, 1});
	// 		ehdl.elem().set_text("#<[32>dinasfas;lhfjahfasfa\n\n\ngdongji");
	// 		ehdl.elem().set_policy(font::typesetting::layout_policy::auto_feed_line);
	// 	}
	//
	// 	{
	// 		auto ehdl = hdl.elem().emplace<ui::elem>();
	// 		ehdl.cell().set_width(60);
	// 		ehdl.cell().set_height_from_scale(3.);
	// 	}
	//
	// 	hdl.elem().end_line();
	//
	// 	{
	// 		auto ehdl = hdl.elem().emplace<ui::elem>();
	// 		ehdl.cell().pad.right = 12;
	// 	}
	//
	// 	{
	// 		auto ehdl = hdl.elem().emplace<ui::elem>();
	// 		ehdl.cell().margin.set(4);
	// 	}
	// }

}

void main_loop(){
	using namespace mo_yanxi;

	vk::context context{vk::ApplicationInfo};
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

	graphic::allocated_image_region& p = font_manager.page().register_named_region("boarder", graphic::msdf::create_boarder(12.f)).first;
	graphic::image_nine_region nine_region{
		p,
		align::padding<std::uint32_t>{}.set(16).expand(graphic::msdf::sdf_image_boarder),
		graphic::msdf::sdf_image_boarder
	};

	font::typesetting::parser parser{};
	font::typesetting::apd_default_modifiers(parser);
	// auto rst = parser(
	// 	"，楼上的\n#<[#ffff00bb><[123>下来 搞核算 叮咚鸡叮咚鸡，#<[tele>Dgdgjjj\ndsjydyddyd",
	// 	font::typesetting::layout_policy::auto_feed_line | font::typesetting::layout_policy::reserve,
	// 	{1600, 200});

	vk::load_ext(context.get_instance());
	assets::graphic::load(context);

	graphic::renderer_export exports;
	graphic::renderer_world renderer_world{context, exports};
	graphic::renderer_ui renderer_ui{context, exports};

	core::global::ui::root->add_scene(ui::scene{"main", new ui::loose_group{nullptr, nullptr}, &renderer_ui}, true);
	core::global::ui::root->resize(math::frect{math::vector2{context.get_extent().width, context.get_extent().height}.as<float>()}.shrink(8));
	init_ui(core::global::ui::root->root_of<ui::loose_group>("main"));

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

		// draw_glyph_layout(renderer_world, rst, {}, false);
		// graphic::draw::default_transparent_acquirer acquirer{renderer_world.batch, {}};
		graphic::draw::ui_acquirer acquirer{renderer_ui.batch, graphic::draw::white_region};

		// acquirer.proj.mode_flag = vk::vertices::mode_flag_bits::sdf;
		// graphic::draw::nine_patch(acquirer, nine_region, {tags::from_extent, {100, 100}, {500, 500}}, graphic::colors::AQUA.to_light_color_copy());

		// acquirer.proj.mode_flag = vk::vertices::mode_flag_bits::none;
		// acquirer << base_region2;
		// graphic::draw::fill::rect_ortho(acquirer.get(), math::frect{20, 30, 70, 90}, graphic::colors::white.to_light_color_copy());

		// renderer_ui.batch.push_scissor({{200, 200, 400, 400}, 32});
		//
		// graphic::draw::fill::rect_ortho(acquirer.get(), {0, 0, 800, 800});

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

	assets::graphic::dispose();
	core::global::ui::dispose();

}

int main(){
	using namespace mo_yanxi;
	font::font_face face{R"(D:\projects\mo_yanxi\prop\assets\fonts\telegrama.otf)"};

	// graphic::msdf::ff();
	// auto mp = graphic::msdf::load_svg(R"(D:\projects\mo_yanxi\prop\assets\svg\test.svg)", 10, 10);
	// mp.write(R"(D:\projects\mo_yanxi\prop\assets\svg\test.png)", true);
	// graphic::test(reinterpret_cast<::msdfgen::FontHandle*>(face.handle()));
	init_assets();
	compile_shaders();

	core::glfw::init();
	core::global::ui::init();

	main_loop();

	core::glfw::terminate();
}
