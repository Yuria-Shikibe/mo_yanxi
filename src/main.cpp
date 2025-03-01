#include <vulkan/vulkan.h>
#include <cassert>

// #define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

extern "C++"{
#include <finders_interface.h>
}

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
import mo_yanxi.vk.util.uniform;

import mo_yanxi.vk.vertex_info;
import mo_yanxi.vk.ext;

import mo_yanxi.assets.directories;
import mo_yanxi.graphic.shaderc;
import mo_yanxi.graphic.color;

import mo_yanxi.core.window;
import mo_yanxi.allocator_2D;
import mo_yanxi.algo;

import mo_yanxi.graphic.camera;


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
}

void init_test_image(mo_yanxi::vk::context& context, mo_yanxi::vk::image& image){
	using namespace mo_yanxi;

	graphic::color colora, colorb;
	auto c = colora.lerp(colorb, 1.f);

	graphic::bitmap test{R"(D:\projects\mo_yanxi\prop\CustomUVChecker_byValle_1K.png)"};
	image = {
			context.get_allocator(), {
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.imageType = VK_IMAGE_TYPE_2D,
				.format = VK_FORMAT_R8G8B8A8_UNORM,
				.extent = {test.width(), test.height(), 1},
				.mipLevels = 1,
				.arrayLayers = 1,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.tiling = VK_IMAGE_TILING_OPTIMAL,
				.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
			},
			{
				.usage = VMA_MEMORY_USAGE_GPU_ONLY
			}
		};

	auto command_buffer = context.get_graphic_command_pool_transient().obtain();
	vk::buffer buffer{vk::templates::create_staging_buffer(context.get_allocator(), test.size_bytes())};
	{
		vk::buffer_mapper mapper{buffer};
		mapper.load_range(test.to_span());
	}

	command_buffer.begin();
	image.init_layout_write(command_buffer);

	vk::cmd::copy_buffer_to_image(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {
		                              VkBufferImageCopy{
			                              .imageSubresource = {
				                              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				                              .mipLevel = 0,
				                              .baseArrayLayer = 0,
				                              .layerCount = 1
			                              },
			                              .imageOffset = {},
			                              .imageExtent = image.get_extent()
		                              }
	                              });

	command_buffer.end();
	context.submit_graphics(command_buffer);
	context.wait_on_graphic();
}

void main_loop(){
	using namespace mo_yanxi;

	vk::context context{vk::ApplicationInfo};
	vk::ext::load(context.get_instance());

	/*
	vk::texture texture{context.get_allocator(), {4096, 4096}};
	vk::image image{};
	init_test_image(context, image);
	{
		vk::fence fence{context.get_device(), false};

		auto cmd = context.get_graphic_command_pool_transient().get_transient(context.graphic_queue(), fence);
		texture.init_layout(cmd);

		allocator2d allocator2d{{texture.get_image().get_extent().width, texture.get_image().get_extent().height}};
		std::vector<vk::texture_bitmap_write> tasks{};

		(assets::dir::texture / "test").for_all_subs([&](io::file&& file){
			if(file.extension() != ".png")return;
			vk::texture_bitmap_write task{};
			task.bitmap = graphic::bitmap{file.path()};
			if(task.bitmap.area() > 2045 * 2048)return;

			if(auto pos = allocator2d.allocate(task.bitmap.extent())){
				task.offset = pos.value().as<int>();
				tasks.push_back(std::move(task));
			}
		});

		auto _ = texture.write(cmd, tasks);
		cmd = context.get_graphic_command_pool_transient().get_transient(context.graphic_queue(), fence);

		graphic::bitmap bitmap{4096, 4096};

		auto handle = vk::dump_image(
			context.get_allocator(), cmd,
			texture.get_image(), VK_FORMAT_R8G8B8A8_UNORM,
			{{}, bitmap.width(), bitmap.height()},
			bitmap.to_mdspan_row_major(),

			VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			VK_ACCESS_2_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			0
		);

		cmd = {};
		handle.resume();
		bitmap.write(R"(D:\projects\mo_yanxi\prop\test.png)", true);
	}
	*/

	vk::shader_module test_mesh{context.get_device(), assets::dir::shader_spv / "test.mesh.spv"};
	vk::shader_module test_frag{context.get_device(), assets::dir::shader_spv / "test.frag.spv"};

	vk::shader_module world_vert{context.get_device(), assets::dir::shader_spv / "world.vert.spv"};
	vk::shader_module world_frag{context.get_device(), assets::dir::shader_spv / "world.frag.spv"};


	vk::command_buffer command_buffer{context.get_graphic_command_pool().obtain()};
	vk::fence fence{context.get_device(), false};


	vk::buffer indices_buffer{vk::templates::create_index_buffer(context.get_allocator(), vk::indices::default_indices_group.size())};
	vk::buffer vertices_buffer{
		vk::templates::create_vertex_buffer(context.get_allocator(), sizeof(vk::vertices::vertex_world) * 4)};
	vk::buffer indirect_buffer{context.get_allocator(), {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = sizeof(VkDrawIndexedIndirectCommand),
		.usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT
	}, {.usage = VMA_MEMORY_USAGE_CPU_TO_GPU}};


	{
		vk::buffer buffer{vk::templates::create_staging_buffer(context.get_allocator(), indices_buffer.get_size())};
		(void)vk::buffer_mapper{buffer}.load_range(vk::indices::default_indices_group);
		{
			vk::scoped_recorder scoped_recorder{command_buffer};

			vk::cmd::dependency_gen dependency_gen{};
			dependency_gen.push_staging(buffer);
			dependency_gen.push_on_initialization(indices_buffer);
			dependency_gen.apply(scoped_recorder);

			buffer.copy_to(scoped_recorder, indices_buffer, {VkBufferCopy{
				.srcOffset = 0,
				.dstOffset = 0,
				.size = buffer.get_size()
			}});

			dependency_gen.push_post_write(indices_buffer, VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT, VK_ACCESS_2_INDEX_READ_BIT);
			dependency_gen.apply(scoped_recorder);
		}

		context.submit_graphics(command_buffer, fence);
		fence.wait_and_reset();
	}

	vk::color_attachment color_attachment{
		context.get_allocator(),
		context.get_extent(),
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
	};


	{
		{
			vk::scoped_recorder scoped_recorder{command_buffer};
			color_attachment.init_layout(scoped_recorder);
		}
		context.submit_graphics(command_buffer, fence);
		fence.wait_and_reset();
	}


	struct proj{
		vk::padded_mat3 view;
	};

	vk::descriptor_layout descriptor_layout_proj{
		context.get_device(),
		VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
		[](vk::descriptor_layout_builder& builder){
			builder.push_seq(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
		}
	};
	vk::descriptor_layout descriptor_layout_image{
		context.get_device(),
		VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
		[](vk::descriptor_layout_builder& builder){
			builder.push_seq(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4);
		}
	};

	vk::uniform_buffer ubo{
		context.get_allocator(),
		sizeof(proj)
	};

	vk::descriptor_buffer descriptor_buffer_proj{
		context.get_allocator(),
		descriptor_layout_proj, descriptor_layout_proj.binding_count()};
	vk::descriptor_buffer descriptor_buffer_image{
		context.get_allocator(),
		descriptor_layout_image, descriptor_layout_image.binding_count()};

	(void)vk::descriptor_mapper{descriptor_buffer_proj}
		.set_uniform_buffer(0, ubo);

	proj proj{};
	math::mat3 mat;
	mat.set_orthogonal_flip_y({}, math::vector2{context.get_extent().width, context.get_extent().height}.as<float>());
	proj.view = mat;

	(void)vk::buffer_mapper{ubo}.load(proj);



	vk::pipeline_layout pipeline_layout{context.get_device(), 0, {descriptor_layout_proj, descriptor_layout_image}};
	vk::graphic_pipeline_template pipeline_template{};
	pipeline_template.set_viewport({0, 0, static_cast<float>(context.get_extent().width), static_cast<float>(context.get_extent().height), 0, 1});
	pipeline_template.set_scissor(context.get_screen_area());
	pipeline_template.set_shaders({world_vert, world_frag});
	pipeline_template.set_vertex_info(
		vk::vertices::vertex_world_info.get_default_bind_desc(),
		vk::vertices::vertex_world_info.get_default_attr_desc()
	);
	pipeline_template.push_color_attachment_format(VK_FORMAT_R8G8B8A8_UNORM);

	vk::pipeline pipeline{context.get_device(), pipeline_layout, VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT, pipeline_template};

	(void)vk::buffer_mapper{indirect_buffer}.load(VkDrawIndexedIndirectCommand{
		.indexCount = 6,
		.instanceCount = 1,
		.firstIndex = 0,
		.vertexOffset = 0,
		.firstInstance = 0
	});

	(void)vk::buffer_mapper{vertices_buffer}.load(std::array<vk::vertices::vertex_world, 4>{
		vk::vertices::vertex_world{math::vec2{100, 500} + math::vec2{0, 0}},
		vk::vertices::vertex_world{math::vec2{100, 500} + math::vec2{50, 0}},
		vk::vertices::vertex_world{math::vec2{100, 500} + math::vec2{50, 50}},
		vk::vertices::vertex_world{math::vec2{100, 500} + math::vec2{0, 50}},
	});

	auto record_cmd = [&]{
		{
			vk::scoped_recorder scoped_recorder{command_buffer, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT};

			vk::dynamic_rendering dynamic_rendering{{color_attachment.get_image_view()}};
			dynamic_rendering.begin_rendering(scoped_recorder, context.get_screen_area());
			pipeline.bind(scoped_recorder, VK_PIPELINE_BIND_POINT_GRAPHICS);
			vk::ext::cmdBindThenSetDescriptorBuffers(
				scoped_recorder,
				VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0,
				{
					descriptor_buffer_proj.get_bind_info(VK_BUFFER_USAGE_2_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT),
					descriptor_buffer_image.get_bind_info(VK_BUFFER_USAGE_2_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT)
				});

			constexpr VkDeviceSize zero{};
			vkCmdBindVertexBuffers(
				scoped_recorder, 0, 1, vertices_buffer.as_data(), &zero);

			vkCmdBindIndexBuffer(
				scoped_recorder, indices_buffer, 0,
				VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexedIndirect(
				scoped_recorder, indirect_buffer, 0, 1, sizeof(VkDrawIndexedIndirectCommand));

			vkCmdEndRendering(scoped_recorder);
		}
	};

	context.register_post_resize("test", [&](window_instance::resize_event event){
		vkDeviceWaitIdle(context.get_device());
		{
			auto cmd = context.get_graphic_command_pool_transient().get_transient(context.graphic_queue());
		   color_attachment.resize(cmd, event.size);
		}

		record_cmd();

		context.set_staging_image({
				.image = color_attachment.get_image(),
				.extent = color_attachment.get_image().get_extent2(),
				.clear = true,
				.owner_queue_family = context.graphic_family(),
				.src_stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				.src_access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
				.dst_stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				.dst_access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
				.src_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.dst_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			}, false);
	});

	record_cmd();
	context.set_staging_image({
		.image = color_attachment.get_image(),
		.extent = color_attachment.get_image().get_extent2(),
		.clear = true,
		.owner_queue_family = context.graphic_family(),
		.src_stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		.src_access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		.dst_stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dst_access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		.src_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.dst_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	});


	while(!context.window().should_close()){
		context.window().poll_events();

		context.submit_graphics(command_buffer);

		context.flush();
	}

	vkDeviceWaitIdle(context.get_device());
}

int main(){
	using namespace mo_yanxi;

	init_assets();
	compile_shaders();


	core::glfw::init();

	main_loop();

	core::glfw::terminate();

	// size_chunked_vector v{4096};
	// for(int i = 0; i < 15; ++i){
	// 	v.insert_or_assign(4096, {static_cast<unsigned>(i)});
	//
	// }
}
