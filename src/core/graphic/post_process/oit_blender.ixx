module;

#include <vulkan/vulkan.h>
#include <cassert>

export module mo_yanxi.graphic.post_processor.oit_blender;

export import mo_yanxi.graphic.post_processor.base;
export import mo_yanxi.vk.image_derives;
export import mo_yanxi.vk.resources;

import std;

namespace mo_yanxi::graphic{

	export struct oit_statistic{
		std::uint32_t capacity;
		std::uint32_t size;

		std::uint32_t cap1;
		std::uint32_t cap2;
	};

	export struct oit_node{ // Desc Only
		using color_type = std::array<std::uint16_t, 4>;
		color_type color_base;
		color_type color_light;
		color_type color_normal;

		float depth;
		std::uint32_t next;
	};

	export std::uint32_t get_oit_buffer_count(VkExtent2D ext){
		return ext.width * ext.height * 6u;
	}

	export VkDeviceSize get_oit_buffer_size(VkExtent2D ext){
		return sizeof(oit_statistic) + get_oit_buffer_count(ext) * sizeof(oit_node);
	}

	export
	struct oit_blender final : post_processor{
	private:

		vk::buffer_borrow oit_buffer{};
		VkSampler sampler{};
	public:
		[[nodiscard]] oit_blender() = default;

		[[nodiscard]] explicit oit_blender(
			vk::context& context, const math::usize2 size,
			const VkSampler sampler,
			const VkPipelineShaderStageCreateInfo& shader_info)
			: post_processor(context, size), sampler{sampler}{
			descriptor_layout = vk::descriptor_layout(context.get_device(), [](vk::descriptor_layout_builder& builder){
				builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); //oit list

				builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT); //heads

				builder.push_seq(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT); //depth
				builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT); //opaque input_base
				builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT); //opaque input_light
				// builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);	//opaque input_normal

				builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT); //output_base
				builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT); //output_light
				// builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);	//output_normal
			});

			default_init_descriptor_buffer();
			default_init_pipeline(shader_info);
		}

		void update_oit_buffer(const vk::buffer_borrow& range){
			assert(range.size > sizeof(oit_statistic));

			oit_buffer = range;

			(void)vk::descriptor_mapper{descriptor_buffer}
				.set_storage_buffer(0, oit_buffer.begin() + sizeof(oit_statistic), oit_buffer.size - sizeof(oit_statistic));
		}

	public:
		void set_input(const std::initializer_list<post_process_socket> sockets) override{
			post_processor::set_input(sockets);
			assert(inputs.size() == 4);

			auto& heads = get_oit_head_image();
			auto& depth = get_input_depth();
			auto& color_base = get_input_color_base();
			auto& color_light = get_input_color_light();

			(void)vk::descriptor_mapper{descriptor_buffer}
				.set_storage_image(1, heads.view)
				.set_image(2, depth.view, 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler)
				.set_storage_image(3, color_base.view)
				.set_storage_image(4, color_light.view);
		}

		void set_output(const std::initializer_list<post_process_socket> sockets) override{
			post_processor::set_output(sockets);
			assert(outputs.size() == 2);
			auto& color_base =  get_output_color_base();
			auto& color_light = get_output_color_light();

			(void)vk::descriptor_mapper{descriptor_buffer}
				.set_storage_image(4 + 1, color_base.view)
				.set_storage_image(4 + 2, color_light.view);
		}

		void update(
			math::usize2 size,
			const vk::buffer_borrow& oit_buffer_range,
			const std::initializer_list<post_process_socket> inputs,
			const std::initializer_list<post_process_socket> outputs){
			update_oit_buffer(oit_buffer_range);
			set_input(inputs);
			set_output(outputs);
			resize(nullptr, size, true);
		}

	protected:
		[[nodiscard]] auto& get_oit_head_image() const noexcept{
			return inputs[0];
		}

		[[nodiscard]] auto& get_input_depth() const noexcept{
			return inputs[1];
		}

		[[nodiscard]] auto& get_input_color_base() const noexcept{
			return inputs[2];
		}

		[[nodiscard]] auto& get_input_color_light() const noexcept{
			return inputs[3];
		}

		[[nodiscard]] std::span<const post_process_socket> get_color_attachment_inputs() const noexcept{
			return {inputs.data() + 2, inputs.size() - 2};
		}

		[[nodiscard]] auto& get_output_color_base() const noexcept{
			return outputs[0];
		}

		[[nodiscard]] auto& get_output_color_light() const noexcept{
			return outputs[1];
		}

		void record_commands() override{
			vk::scoped_recorder recorder{main_command_buffer, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT};

			vk::cmd::dependency_gen dependency_gen{};

			auto& heads = get_oit_head_image();
			auto& depth = get_input_depth();

			dependency_gen.push(
				heads.image,
				VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
				VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
				heads.layout,
				VK_IMAGE_LAYOUT_GENERAL,
				vk::image::default_image_subrange,
				heads.queue_family_index,
				context().compute_family()
			);

			dependency_gen.push(
				depth.image,
				VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
				VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
				depth.layout,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				vk::image::depth_image_subrange,
				depth.queue_family_index,
				context().compute_family()
			);

			for (const auto & input : get_color_attachment_inputs()){
				dependency_gen.push(
					input.image,
					VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
					input.layout,
					VK_IMAGE_LAYOUT_GENERAL,
					vk::image::default_image_subrange,
					input.queue_family_index,
					context().compute_family()
				);
			}


			auto& bufferB = dependency_gen.push(
				oit_buffer.buffer,
				VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
				VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
				context().graphic_family(),
				context().compute_family(),
				oit_buffer.offset, oit_buffer.size
			);

			dependency_gen.apply(recorder, true);

			const auto sz = get_work_group_size(size());
			vkCmdBindPipeline(recorder, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
			descriptor_buffer.bind_to(recorder, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0);
			vkCmdDispatch(recorder, sz.x, sz.y, 1);

			for (const auto & in : get_color_attachment_inputs()){
				vk::cmd::clear_color(recorder, in.image, {}, vk::image::default_image_subrange,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
					VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
					in.layout,
					context().compute_family(),
					context().compute_family(),
					in.queue_family_index
				);
			}

			vk::cmd::clear_color(recorder, heads.image,
			                     {
				                     .uint32 = {
					                     std::numeric_limits<std::uint32_t>::max()
				                     }
			                     }, vk::image::default_image_subrange,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
					VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
					VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
					heads.layout,
					context().compute_family(),
					context().compute_family(),
					heads.queue_family_index
				);

			dependency_gen.image_memory_barriers.clear();
			dependency_gen.push(
				depth.image,
				VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
				VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
				depth.layout,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				vk::image::depth_image_subrange,
				depth.queue_family_index,
				context().compute_family()
			);
			dependency_gen.swap_stages();

			bufferB.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			bufferB.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			bufferB.srcQueueFamilyIndex = bufferB.dstQueueFamilyIndex = context().compute_family();

			dependency_gen.apply(recorder, true);
			dependency_gen.image_memory_barriers.clear();

			const auto off = std::bit_cast<std::uint32_t>(&oit_statistic::size);
			vkCmdFillBuffer(recorder, oit_buffer.buffer, oit_buffer.offset + off, sizeof(oit_statistic) - off, 0);

			dependency_gen.swap_stages();
			bufferB.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			bufferB.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
			bufferB.dstQueueFamilyIndex = context().graphic_family();
			dependency_gen.apply(recorder, true);


		}
	};
}