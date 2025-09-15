module;

#include <vulkan/vulkan.h>

export module mo_yanxi.graphic.post_processor.anti_aliasing;

export import mo_yanxi.graphic.post_processor.base;
export import mo_yanxi.vk.shader;
import std;

namespace mo_yanxi::graphic{

	struct anti_aliasing_info{
		float scale;
		std::uint32_t cap1, cap2, cap3;
	};

	export
	struct anti_aliasing final : post_processor{
		VkSampler sampler{};

		vk::uniform_buffer uniform_buffer{};

		[[nodiscard]] anti_aliasing() = default;

		[[nodiscard]] explicit anti_aliasing(
			vk::context& context,
			math::usize2 size,
			const vk::shader_module& bloom_shader,
			VkSampler sampler,
			const std::size_t nX = 1
			)
			:
		post_processor(context, size),
		sampler(sampler), uniform_buffer{context.get_allocator(), sizeof(anti_aliasing_info)}{

			descriptor_layout = vk::descriptor_layout(context.get_device(), [](vk::descriptor_layout_builder& builder){
				builder.push_seq(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
				builder.push_seq(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);
				builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
			});

			default_init_descriptor_buffer();
			default_init_pipeline(bloom_shader.get_create_info());

			vk::descriptor_mapper info{descriptor_buffer};
			vk::buffer_mapper{uniform_buffer}.load(anti_aliasing_info{1.f});
			(void)info.set_uniform_buffer(0, uniform_buffer);
		}

		void set_input(std::initializer_list<post_process_socket> sockets) override{
			post_processor::set_input(sockets);

			vk::descriptor_mapper info{descriptor_buffer};
			info.set_image(1, inputs[0].view, 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler);
		}

		void set_output(std::initializer_list<post_process_socket> sockets) override{
			post_processor::set_output(sockets);

			vk::descriptor_mapper info{descriptor_buffer};
			info.set_storage_image(2, outputs[0].view);
		}

		void after_set_sockets(){
			record_commands();
		}

	protected:
		void record_commands() override{
			const auto extent = std::bit_cast<math::u32size2>(context().get_extent());
			vk::scoped_recorder scoped_recorder{main_command_buffer, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT};
			vk::cmd::dependency_gen dependency{};


			dependency.push(
				inputs[0].image,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_READ_BIT,
				inputs[0].src_layout,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				vk::image::default_image_subrange
			);

			dependency.push(
				outputs[0].image,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_GENERAL,
				vk::image::default_image_subrange
			);

			dependency.apply(scoped_recorder);

			pipeline.bind(scoped_recorder, VK_PIPELINE_BIND_POINT_COMPUTE);
			descriptor_buffer.bind_to(scoped_recorder, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0);
			auto groups = get_work_group_size(extent);
			vkCmdDispatch(scoped_recorder, groups.x, groups.y, 1);



			dependency.push(
				inputs[0].image,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_READ_BIT,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_LAYOUT_GENERAL,
				vk::image::default_image_subrange
			);


			dependency.apply(scoped_recorder);
		}
	};
}