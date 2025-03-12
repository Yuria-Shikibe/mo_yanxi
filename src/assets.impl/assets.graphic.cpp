module;

#include <vulkan/vulkan.h>

module mo_yanxi.assets.graphic;


import mo_yanxi.assets.directories;

import std;
import mo_yanxi.vk.vertex_info;
import mo_yanxi.vk.context;
import mo_yanxi.vk.resources;
import mo_yanxi.vk.command_buffer;

using namespace mo_yanxi;

vk::buffer indices_buffer{};

void mo_yanxi::assets::graphic::load(vk::context& context){
	indices_buffer = vk::templates::create_index_buffer(context.get_allocator(), vk::indices::default_indices_group.size());
	{
		vk::buffer buffer{vk::templates::create_staging_buffer(context.get_allocator(), indices_buffer.get_size())};
		(void)vk::buffer_mapper{buffer}.load_range(vk::indices::default_indices_group);
		{
			auto cmd = context.get_graphic_command_pool_transient().get_transient(context.graphic_queue());
			buffer.copy_to(cmd, indices_buffer, {VkBufferCopy{
				.srcOffset = 0,
				.dstOffset = 0,
				.size = buffer.get_size()
			}});
		}
	}

	buffers::indices_buffer = indices_buffer;

	shaders::vert::world = {context.get_device(), dir::shader_spv / "world.vert.spv"};
	shaders::vert::ui = {context.get_device(), dir::shader_spv / "ui.vert.spv"};

	shaders::frag::world = {context.get_device(), dir::shader_spv / "world.frag.spv"};
	shaders::frag::ui = {context.get_device(), dir::shader_spv / "ui.frag.spv"};

	shaders::comp::resolve = {context.get_device(), dir::shader_spv / "resolve.comp.spv"};
	shaders::comp::bloom = {context.get_device(), dir::shader_spv / "bloom.comp.spv"};
	shaders::comp::ssao = {context.get_device(), dir::shader_spv / "ssao.comp.spv"};

	shaders::comp::world_merge = {context.get_device(), dir::shader_spv / "world.merge.comp.spv"};
	shaders::comp::ui_blit = {context.get_device(), dir::shader_spv / "ui.blit.comp.spv"};
	shaders::comp::ui_merge = {context.get_device(), dir::shader_spv / "ui.merge.comp.spv"};

	samplers::texture_sampler = {context.get_device(), vk::preset::default_texture_sampler};
	samplers::blit_sampler = {context.get_device(), vk::preset::default_blit_sampler};
}

void mo_yanxi::assets::graphic::dispose(){
	indices_buffer = {};
	buffers::indices_buffer = {};

	shaders::vert::world = {};
	shaders::vert::ui = {};
	shaders::frag::world = {};
	shaders::frag::ui = {};
	shaders::comp::resolve = {};
	shaders::comp::bloom = {};
	shaders::comp::world_merge = {};
	shaders::comp::ssao = {};
	shaders::comp::ui_blit = {};
	shaders::comp::ui_merge = {};

	samplers::texture_sampler = {};
	samplers::blit_sampler = {};
}
