module;

#include <vulkan/vulkan.h>

export module test;

import mo_yanxi.assets.directories;
import mo_yanxi.assets.ctrl;

import mo_yanxi.vk.context;
import mo_yanxi.core.window;


import mo_yanxi.ui.primitives;
import mo_yanxi.ui.style;
import mo_yanxi.ui.assets;
import mo_yanxi.graphic.image_manage;
import mo_yanxi.graphic.msdf;
import mo_yanxi.graphic.color;
import mo_yanxi.graphic.image_multi_region;

import mo_yanxi.graphic.shaderc;

import mo_yanxi.game.meta.projectile;

import std;

namespace test{
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
	void set_swapchain_flusher(){
		auto& context = core::global::graphic::context;
		auto& renderer_world = core::global::graphic::world;
		auto& renderer_ui = core::global::graphic::ui;
		auto& merger = core::global::graphic::merger;

		context.register_post_resize("test", [&](window_instance::resize_event event){
			//TODO should it resize all?
			//TODO resize registry?
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
