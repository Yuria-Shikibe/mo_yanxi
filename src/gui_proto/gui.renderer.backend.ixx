module;

#include <cassert>
#include <vulkan/vulkan.h>

export module prototype.renderer.ui;

export import mo_yanxi.gui.renderer.frontend;

import mo_yanxi.math.rect_ortho;
import mo_yanxi.math.vector2;
import mo_yanxi.math.matrix3;

import mo_yanxi.graphic.draw.instruction.batch;
import mo_yanxi.graphic.draw.instruction;

import mo_yanxi.vk.util.uniform;
import mo_yanxi.vk;
import mo_yanxi.vk.cmd;


import std;

namespace mo_yanxi::gui{

export
struct renderer{
private:
	graphic::draw::instruction::user_data_index_table ubo_table_{
		graphic::draw::instruction::make_user_data_index_table<gui_reserved_user_data_tuple>()
	};

public:
	graphic::draw::instruction::batch batch{};
private:
	graphic::draw::instruction::batch_descriptor_slots general_descriptors_{};
	vk::uniform_buffer uniform_buffer_{};

	graphic::draw::instruction::batch_command_slots batch_commands_{};
	vk::pipeline_layout pipeline_layout_{};
	vk::pipeline pipeline_{};

	// vk::descriptor_layout descriptor_layout_{};
	// vk::descriptor_buffer descriptor_buffer_{};

	std::unique_ptr<std::pmr::unsynchronized_pool_resource> pool_resource{
			std::make_unique<std::pmr::unsynchronized_pool_resource>()
		};

	//screen space to uniform space viewport
	std::pmr::vector<layer_viewport> viewports{pool_resource.get()};
	math::mat3 uniform_proj{};

	vk::color_attachment attachment_base{};
	vk::color_attachment attachment_background{};

public:
	[[nodiscard]] renderer() = default;

	[[nodiscard]] explicit renderer(
		vk::context& ctx,
		VkSampler spr,
		const vk::shader_module& ui_main_shader
		)
		: batch(ctx, spr, ubo_table_)
	, general_descriptors_(ctx, [](vk::descriptor_layout_builder& b){
		b.push_seq(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT);
		b.push_seq(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT);
	})
	, uniform_buffer_(ctx.get_allocator(), ubo_table_.max_size() * batch.work_group_count(), VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
	, batch_commands_(ctx.get_graphic_command_pool())
	, pipeline_layout_(
		ctx.get_device(), 0,
{batch.get_batch_descriptor_layout(), general_descriptors_.descriptor_set_layout()})

	{
		general_descriptors_.bind([&](const std::uint32_t idx, const vk::descriptor_mapper& mapper){
			using T = graphic::draw::instruction::user_data_index_table;
			for(auto&& [group, rng] : ubo_table_.get_entries() | std::views::chunk_by([](const T::identity_entry& l, const T::identity_entry& r){
				return l.entry.group_index == r.entry.group_index;
			}) | std::views::enumerate){
				for (const auto & [binding, ientry] : rng | std::views::enumerate){
					auto& entry = ientry.entry;
					(void)mapper.set_uniform_buffer(
						binding,
						uniform_buffer_.get_address() + idx * ubo_table_.max_size() + entry.local_offset, entry.size,
						idx);
				}
			}

		});


		batch.set_submit_callback([&](const std::uint32_t idx, graphic::draw::instruction::batch::ubo_data_entries spn) -> VkCommandBuffer {
			if(spn){
				//TODO support multiple ubo
				vk::buffer_mapper mapper{uniform_buffer_};
				for (const auto & entry : spn.entries){
					if(!entry)continue;
					(void)mapper.load_range(entry.to_range(spn.base_address), entry.local_offset + idx * ubo_table_.max_size());
				}
			}

			return batch_commands_[idx];
		});

		init_pipeline(ui_main_shader);

	}

	void resize(VkExtent2D extent){
		{
			const auto [ox, oy] = attachment_base.get_image().get_extent2();
			if(extent.width == ox && extent.height == oy)return;
		}

		attachment_base =
			vk::color_attachment{
				context().get_allocator(), extent,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
				VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_FORMAT_R16G16B16A16_SFLOAT
			};
		attachment_background =
			vk::color_attachment{
				context().get_allocator(), extent,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
				VK_IMAGE_USAGE_SAMPLED_BIT
			};

		{
			auto transient = context().get_transient_graphic_command_buffer();
			attachment_base.init_layout(transient);
			attachment_background.init_layout(transient);
		}

		record_command();
	}

	[[nodiscard]] vk::context& context() const noexcept{
		return batch.context();
	}

private:
	[[nodiscard]] VkRect2D get_screen_area() const noexcept{
		return {0, 0, attachment_base.get_image().get_extent2()};
	}

	void record_command(){
		vk::dynamic_rendering dynamic_rendering{
			{attachment_base.get_image_view()},
			nullptr};

		const graphic::draw::instruction::batch_descriptor_buffer_binding_info dbo_info{
			general_descriptors_.dbo(),
			general_descriptors_.dbo().get_chunk_size()
		};

		batch.record_command(pipeline_layout_, std::span(&dbo_info, 1), [&] -> std::generator<VkCommandBuffer&&> {
				for (const auto & [idx, buf] : batch_commands_ | std::views::enumerate){
					vk::scoped_recorder recorder{buf, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT};

					dynamic_rendering.begin_rendering(recorder, context().get_screen_area());
					pipeline_.bind(recorder, VK_PIPELINE_BIND_POINT_GRAPHICS);

					vk::cmd::set_viewport(recorder, get_screen_area());
					vk::cmd::set_scissor(recorder, get_screen_area());

					co_yield buf.get();

					vkCmdEndRendering(recorder);
				}
			}());
	}

	void init_pipeline(const vk::shader_module& shader_module){
		vk::graphic_pipeline_template gtp{};
		gtp.set_shaders({
			shader_module.get_create_info(VK_SHADER_STAGE_MESH_BIT_EXT, nullptr, "main_mesh"),
			shader_module.get_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, nullptr, "main_frag")
		});
		gtp.push_color_attachment_format(VK_FORMAT_R16G16B16A16_SFLOAT, vk::blending::alpha_blend);

		pipeline_ = vk::pipeline{context().get_device(), pipeline_layout_, VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT, gtp};
	}

public:
	void wait_idle(){
		batch.consume_all();
		batch.wait_all();
		assert(batch.is_all_done());
	}


#pragma region Getter
	[[nodiscard]] vk::image_handle get_base() const noexcept{
		return attachment_base;
	}

	[[nodiscard]] const math::mat3& get_screen_uniform_proj() const noexcept{
		return uniform_proj;
	}
#pragma endregion

	renderer_frontend create_frontend(){
		return renderer_frontend{graphic::draw::instruction::get_batch_interface(batch)};
	}
};

}
