module;

#include <vulkan/vulkan.h>

export module mo_yanxi.graphic.renderer.merger;

export import mo_yanxi.graphic.renderer;
export import mo_yanxi.vk.resources;
export import mo_yanxi.vk.image_derives;
export import mo_yanxi.vk.pipeline;
export import mo_yanxi.vk.pipeline.layout;
export import mo_yanxi.graphic.post_processor.base;

import std;

namespace mo_yanxi::graphic{

	export
	struct renderer_merge{
	private:
		vk::context * context{};
		const renderer_export* imports{};

		vk::descriptor_layout descriptor_layout{};
		vk::descriptor_buffer descriptor_buffer{};
		vk::pipeline_layout pipeline_layout{};
		vk::pipeline pipeline{};
		vk::command_buffer merge_command{};
		vk::storage_image merge_result{};

	public:
		[[nodiscard]] renderer_merge() = default;

		[[nodiscard]] renderer_merge(vk::context& context, const renderer_export& imports, const VkPipelineShaderStageCreateInfo& module)
			: context(&context), imports{&imports},

		descriptor_layout{context.get_device(), [](vk::descriptor_layout_builder& builder){
			builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
			builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
			builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
		}},
		descriptor_buffer{context.get_allocator(), descriptor_layout, descriptor_layout.binding_count()},
		pipeline_layout{context.get_device(), 0, {descriptor_layout}},
		pipeline(context.get_device(), pipeline_layout, VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT, module),
		merge_command{context.get_compute_command_pool().obtain()},
		merge_result{
		context.get_allocator(), context.get_extent(), 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
		}
		{

			on_resize(context.get_extent());
		}

		[[nodiscard]] vk::image_handle get_result() const noexcept{
			return merge_result;
		}

		void submit() const{
			vk::cmd::submit_command(context->compute_queue(), merge_command);
		}

		void resize(VkExtent2D ext){
			merge_result.resize(ext);

			on_resize(ext);
		}

	private:
		void on_resize(VkExtent2D ext){
			merge_result.init_layout_general(context->get_transient_compute_command_buffer());

			const auto img_world = imports->find("renderer.world");
			const auto img_ui = imports->find("renderer.ui");

			vk::descriptor_mapper{descriptor_buffer}
				.set_storage_image(0, merge_result.get_image_view())
				.set_storage_image(1, img_world.image_view)
				.set_storage_image(2, img_ui.image_view);

			create_command(ext);
		}

		void create_command(const VkExtent2D ext) const{
			vk::scoped_recorder recorder{merge_command, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT};

			auto sz = post_processor::get_work_group_size(std::bit_cast<math::u32size2>(ext));

			vkCmdBindPipeline(recorder, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
			descriptor_buffer.bind_to(recorder, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0);
			vkCmdDispatch(recorder, sz.x, sz.y, 1);
		}
	};
}
