module;

#include <vulkan/vulkan.h>
#include <cassert>

export module mo_yanxi.graphic.post_processor.resolve;

export import mo_yanxi.graphic.post_processor.base;
export import mo_yanxi.vk.shader;
// import mo_yanxi.vk.sampler;
import std;

namespace mo_yanxi::graphic{
	export
	struct resolver_info{
		std::int32_t sample_count;
		// std::int32_t image_count;
	};

	export
	struct post_process_resolver : post_processor{
		static constexpr std::size_t max_images_count{3};
		// static constexpr std::size_t resolver_group_size{4};
		// static constexpr std::size_t get_resolver_group_count(const std::size_t count) noexcept{
		// 	return (count + (resolver_group_size - 1)) / resolver_group_size;
		// }

		std::size_t sample_counts{};
		VkSampler sampler{};

		vk::uniform_buffer uniform_buffer{};

		[[nodiscard]] post_process_resolver() = default;

		[[nodiscard]] explicit post_process_resolver(
			vk::context& context, math::usize2 size,
			const vk::shader_module& bloom_shader,
			VkSampler sampler,
			const std::size_t nX = 1
			)
			: post_processor(context, size), sample_counts(1 << nX), sampler(sampler), uniform_buffer{context.get_allocator(), sizeof(resolver_info)}{

			descriptor_layout = vk::descriptor_layout(context.get_device(), [](vk::descriptor_layout_builder& builder){
				builder.push_seq(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);

				builder.push_seq(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT, max_images_count);
				builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, max_images_count);
			});

			default_init_descriptor_buffer();
			default_init_pipeline(bloom_shader.get_create_info());

			vk::descriptor_mapper info{descriptor_buffer};
			vk::buffer_mapper{uniform_buffer}.load(resolver_info{static_cast<std::int32_t>(sample_counts)});
			(void)info.set_uniform_buffer(0, uniform_buffer);
		}

		void set_input(std::initializer_list<post_process_socket> sockets) override{
			assert(sockets.size() <= max_images_count);

			post_processor::set_input(sockets);

			vk::descriptor_mapper info{descriptor_buffer};
			info.set_image(1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler, inputs | std::views::transform(&post_process_socket::view));
		}

		void set_output(std::initializer_list<post_process_socket> sockets) override{
			post_processor::set_output(sockets);

			assert(sockets.size() <= max_images_count);
			vk::descriptor_mapper info{descriptor_buffer};
			info.set_image(2, VK_IMAGE_LAYOUT_GENERAL, nullptr, outputs | std::views::transform(&post_process_socket::view));
		}

		void after_set_sockets(){
			record_commands();
		}

	protected:
		void record_commands() override{
			const std::size_t count = std::min(inputs.size(), outputs.size());
			if(!count)return;

			const auto extent = std::bit_cast<math::u32size2>(context().get_extent());
			vk::scoped_recorder scoped_recorder{main_command_buffer, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT};
			vk::cmd::dependency_gen dependency{};

			for(std::size_t i = 0; i < count; i++){
				const auto& in = inputs[i];
				const auto& out = outputs[i];

				dependency.push_graphic_to_compute(
					in.image,
					in.queue_family_index,
					context().compute_family(),
					VK_ACCESS_2_SHADER_READ_BIT,
					in.layout
				);

				dependency.push(
					out.image,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
					out.layout,
					VK_IMAGE_LAYOUT_GENERAL,
					vk::image::default_image_subrange
				);
			}

			dependency.apply(scoped_recorder);

			pipeline.bind(scoped_recorder, VK_PIPELINE_BIND_POINT_COMPUTE);
			descriptor_buffer.bind_to(scoped_recorder, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0);
			auto groups = get_work_group_size(extent);
			vkCmdDispatch(scoped_recorder, groups.x, groups.y, count);


			for(std::size_t i = 0; i < count; i++){
				const auto& in = inputs[i];

				vk::cmd::clear_color(scoped_recorder, in.image, {}, vk::image::default_image_subrange,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					in.layout,
					context().compute_family(),
					context().compute_family(),
					in.queue_family_index
				);

			}
			dependency.apply(scoped_recorder);
		}
	};


	/*
	 void record_commands() override{
			const std::size_t count = std::min(inputs.size(), outputs.size());
			if(!count)return;

			vk::scoped_recorder scoped_recorder{main_command_buffer, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT};
			vk::cmd::dependency_gen dependency{};

			for(std::size_t i = 0; i < count; i++){
				const auto& in = inputs[i];
				const auto& out = outputs[i];

				dependency.push(
					in.image,
					VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
					VK_PIPELINE_STAGE_2_TRANSFER_BIT,
					VK_ACCESS_2_TRANSFER_READ_BIT,
					in.layout,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					vk::image::default_image_subrange
				);

				dependency.push(
					out.image,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
					VK_PIPELINE_STAGE_2_TRANSFER_BIT,
					VK_ACCESS_2_TRANSFER_WRITE_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					vk::image::default_image_subrange
				);

				dependency.apply(scoped_recorder);

				// pipeline.bind(scoped_recorder, VK_PIPELINE_BIND_POINT_COMPUTE);
				// descriptor_buffer.bind_to(scoped_recorder, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0);
				// auto groups = get_work_group_size(extent);
				// vkCmdDispatch(scoped_recorder, groups.x, groups.y, count);
				VkImageResolve2 resolve2{
					.sType = VK_STRUCTURE_TYPE_IMAGE_RESOLVE_2,
					.pNext = nullptr,
					.srcSubresource = {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.mipLevel = 0,
						.baseArrayLayer = 0,
						.layerCount = 1
					},
					.srcOffset = {},
					.dstSubresource = {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.mipLevel = 0,
						.baseArrayLayer = 0,
						.layerCount = 1
					},
					.dstOffset = {},
					.extent = context().get_extent3()
				};
				VkResolveImageInfo2 resolve_image_info2{
					.sType = VK_STRUCTURE_TYPE_RESOLVE_IMAGE_INFO_2,
					.pNext = nullptr,
					.srcImage = in.image,
					.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					.dstImage = out.image,
					.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					.regionCount = 1,
					.pRegions = &resolve2
				};
				vkCmdResolveImage2(scoped_recorder, &resolve_image_info2);

				dependency.push(
					out.image,
					VK_PIPELINE_STAGE_2_TRANSFER_BIT,
					VK_ACCESS_2_TRANSFER_WRITE_BIT,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					VK_ACCESS_2_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL,
					vk::image::default_image_subrange
				);

				vk::cmd::clear_color(scoped_recorder, in.image, {}, vk::image::default_image_subrange,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_ACCESS_TRANSFER_READ_BIT,
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					in.layout,
					context().compute_family(),
					context().compute_family(),
					in.queue_family_index
				);

			}
			dependency.apply(scoped_recorder);
		}
	 */
}
