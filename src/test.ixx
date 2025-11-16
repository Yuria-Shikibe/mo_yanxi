module;

#include <vulkan/vulkan.h>

export module test;

import mo_yanxi.assets.directories;
import mo_yanxi.assets.ctrl;

import mo_yanxi.vk;
import mo_yanxi.vk.util;
import mo_yanxi.core.window;


import mo_yanxi.ui.primitives;
import mo_yanxi.ui.style;
import mo_yanxi.ui.assets;
import mo_yanxi.graphic.image_atlas;
import mo_yanxi.graphic.msdf;
import mo_yanxi.graphic.color;
import mo_yanxi.graphic.image_multi_region;

import mo_yanxi.graphic.shaderc;

import mo_yanxi.game.meta.projectile;
import mo_yanxi.game.meta.turret;
import mo_yanxi.core.global.assets;

import mo_yanxi.gui.infrastructure;
import mo_yanxi.gui.infrastructure.group;
import mo_yanxi.gui.elem.manual_table;
import mo_yanxi.gui.global;

import std;

namespace test{
using namespace mo_yanxi;

using namespace mo_yanxi::game::meta::turret;

export
void build_main_ui(gui::scene& scene, gui::loose_group& root);

export turret_drawer drawer;

void load(){
	drawer = {
			.base_component = {
				.base_image = core::global::assets::atlas["main"].register_named_region(
					"lsr",
					graphic::bitmap_path_load{
						R"(D:\projects\mo_yanxi\prop\assets\texture\test\turret\beam-laser-turret.png)"
					}).region
			}
		};


	drawer.base_component.set_extent_by_scale({0.02f, 0.02f});
}

void dispose(){
	drawer = {};
}
}

namespace test{
export void check_validation_layer_activation(){
	if(!DEBUG_CHECK){
		vk::enable_validation_layers = false;
	} else if(auto ptr = std::getenv("NSIGHT"); ptr != nullptr && std::strcmp(ptr, "1") == 0){
		vk::enable_validation_layers = false;
	}
}

export void foot(){
	// using namespace mo_yanxi;
	// math::vec2 v{114, 514};
	// math::vec2 v2;
	// io::pb::math::vector2 buf;
	// io::store(buf, v);
	// io::load(buf, v2);
	// std::println("{} -> {}", v, v2);
}

using namespace mo_yanxi;

export void load_tex(graphic::image_atlas& atlas){
	ui::theme::load(&atlas);
}

export void init_assets(){
	mo_yanxi::assets::load_dir(R"(D:\projects\mo_yanxi\prop)");
	mo_yanxi::assets::ctrl::load();
}

export void compile_shaders(){
	using namespace mo_yanxi;

	graphic::shader_runtime_compiler compiler{};
	graphic::shader_wrapper wrapper{compiler, assets::dir::shader_spv.path()};

	assets::dir::shader_src.for_all_subs([&](io::file&& file){
		wrapper.compile(file);
	});
}

export
void set_up_flush_setter(auto append, auto prov){
	core::global::graphic::context.register_post_resize("test", [=](window_instance::resize_event event){
		core::global::ui::root->resize_all(math::frect{math::vector2{event.size.width, event.size.height}.as<float>()});

		core::global::graphic::world.resize(event.size);
		core::global::graphic::ui.resize(event.size);

		append();
		core::global::graphic::context.set_staging_image(
			{
				.image = prov(),
				.extent = core::global::graphic::context.get_extent(),
				.clear = false,
				.owner_queue_family = core::global::graphic::context.compute_family(),
				.src_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				.src_access = VK_ACCESS_2_SHADER_WRITE_BIT,
				.dst_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				.dst_access = VK_ACCESS_2_SHADER_WRITE_BIT,
				.src_layout = VK_IMAGE_LAYOUT_GENERAL,
				.dst_layout = VK_IMAGE_LAYOUT_GENERAL
			}, false);
	});

	append();
	core::global::graphic::context.set_staging_image({
			.image = prov(),
			.extent = core::global::graphic::context.get_extent(),
			.clear = false,
			.owner_queue_family = core::global::graphic::context.compute_family(),
			.src_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			.src_access = VK_ACCESS_2_SHADER_WRITE_BIT,
			.dst_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			.dst_access = VK_ACCESS_2_SHADER_WRITE_BIT,
			.src_layout = VK_IMAGE_LAYOUT_GENERAL,
			.dst_layout = VK_IMAGE_LAYOUT_GENERAL
		});
}

export
void load_content(){
	test::load();
}

export
void dispose_content(){
	test::dispose();
}

export
mo_yanxi::game::meta::projectile projectile_meta = [](){
	using namespace mo_yanxi;

	return game::meta::projectile{
			.hitbox = {{game::meta::hitbox::comp{.box = {math::vec2{80, 10} * -0.5f, {80, 10}}}}},
			.rigid = {
				.drag = 0.001f
			},
			.damage = {.material_damage = 1000},
			.lifetime = 150,
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
}
