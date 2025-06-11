module;

#include <vulkan/vulkan.h>

export module mo_yanxi.graphic.layers.ui.grid_drawer;

export import mo_yanxi.graphic.batch_proxy;
export import mo_yanxi.graphic.color;
import std;

import mo_yanxi.assets.graphic;
import mo_yanxi.vk.vertex_info;
import mo_yanxi.math.vector2;

namespace mo_yanxi::graphic::layers{

	export struct grid_drawer_data{
		math::vec2 chunk_size{};
		math::vector2<std::int32_t> solid_spacing{};

		color line_color{};
		color main_line_color{};

		float line_width{};
		float main_line_width{};
		float dash_line_spacing{};
		float dash_line_gap{};
	};

	export grid_drawer_data default_grid_style{
		.chunk_size = {128, 128},
		.solid_spacing = {5, 5},
		.line_color = colors::dark_gray,
		.main_line_color = colors::pale_green.copy().mul_rgb(.66f),
		.line_width = 4,
		.main_line_width = 6,
		.dash_line_spacing = 15,
		.dash_line_gap = 5
	};

	export
	struct grid_drawer : batch_layer{
		static constexpr std::string_view layer_name{"grid"};
	public:
		batch_layer_data<grid_drawer_data> data{};

		[[nodiscard]] grid_drawer() = default;

		template <vk::contigious_range_of<VkDescriptorSetLayout> Rng>
		[[nodiscard]] grid_drawer(
			vk::context& context,
			const std::size_t chunk_count,
			Rng&& public_sets)
			:
		batch_layer(context, chunk_count),
		data(context, chunk_count)
		{
			vk::graphic_pipeline_template pipeline_template{};
			pipeline_template.set_shaders({
				assets::graphic::shaders::vert::ui,
				assets::graphic::shaders::frag::ui_grid});
			pipeline_template.set_vertex_info(
				vk::vertices::vertex_ui_info.get_default_bind_desc(),
				vk::vertices::vertex_ui_info.get_default_attr_desc()
			);

			pipeline_template.push_color_attachment_format(VK_FORMAT_R8G8B8A8_UNORM);
			pipeline_template.push_color_attachment_format(VK_FORMAT_R8G8B8A8_UNORM);
			pipeline_template.push_color_attachment_format(VK_FORMAT_R8G8B8A8_UNORM);

			descriptor_layout = vk::descriptor_layout{context.get_device(), [](vk::descriptor_layout_builder& builder){
				builder.push_seq(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
			}};

			this->create_pipeline(context, pipeline_template, std::forward<Rng>(public_sets));
			create_descriptor_buffer(context, chunk_count);
			const vk::descriptor_mapper mapper{descriptor_buffer};
			data.bind_to(mapper, 0);
		}

		command_buffer_modifier create_command(
			const vk::dynamic_rendering& rendering,
			vk::batch& batch, VkRect2D region
		) override{
			for (auto && [idx, vk_command_buffer] : command_buffers | std::views::enumerate){
				vk::scoped_recorder scoped_recorder{vk_command_buffer, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT};

				rendering.begin_rendering(scoped_recorder, region);

				pipeline.bind(scoped_recorder, VK_PIPELINE_BIND_POINT_GRAPHICS);

				co_yield scoped_recorder.get();

				vk::cmd::set_viewport(scoped_recorder, region);
				vk::cmd::set_scissor(scoped_recorder, region);

				batch.bind_resources(scoped_recorder, pipeline_layout, idx);
				descriptor_buffer.bind_chunk_to(scoped_recorder, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 2, idx);

				vkCmdEndRendering(scoped_recorder);
			}
		}

		void try_post_data(std::size_t index) override{
			data.update<true>(index);
		}
	};
}
