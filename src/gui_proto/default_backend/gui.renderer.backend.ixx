module;

#include <cassert>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

export module prototype.renderer.ui;

export import mo_yanxi.gui.renderer.frontend;

import mo_yanxi.math.rect_ortho;
import mo_yanxi.math.vector2;
import mo_yanxi.math.matrix3;

import mo_yanxi.graphic.draw.instruction.batch;
import mo_yanxi.graphic.draw.instruction;

import mo_yanxi.vk.util.uniform;
import mo_yanxi.vk.util;
import mo_yanxi.vk;
import mo_yanxi.vk.cmd;

import mo_yanxi.gui.alloc;

import std;

namespace mo_yanxi::gui{

#pragma region BlitUtil

struct indirect_dispatcher{
	using value_type = VkDispatchIndirectCommand;

private:
	vk::buffer buffer{};
	value_type current{};

public:
	[[nodiscard]] indirect_dispatcher() = default;

	[[nodiscard]] indirect_dispatcher(
		vk::allocator& allocator,
		const std::size_t chunk_count)
		: buffer(allocator, VkBufferCreateInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = sizeof(value_type) * chunk_count,
			.usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
		}, {
			.usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
		})
	{}

	void set(math::u32size2 extent, math::u32size2 group_extent = {16, 16}) noexcept{
		//Ceil Div
		extent.add(group_extent.copy().sub(1u, 1u)).div(group_extent);

		current = value_type{
			.x = extent.x,
			.y = extent.y,
			.z = 1
		};
	}

	void set_divided(math::u32size2 workgroup_extent) noexcept{
		current = value_type{
			.x = workgroup_extent.x,
			.y = workgroup_extent.y,
			.z = 1
		};
	}

	void update(std::size_t index){
		assert(index < buffer.get_size() / sizeof(value_type));

		(void)vk::buffer_mapper{buffer}.load(
				current, sizeof(value_type) * index
			);
	}

	[[nodiscard]] VkDeviceSize offset_at(std::size_t index) const noexcept{
		assert(index < buffer.get_size() / sizeof(value_type));
		return index * sizeof(value_type);
	}

	explicit(false) operator VkBuffer() const noexcept{
		return buffer;
	}
};

struct blit_resources{
	vk::image_handle input_base;
	vk::image_handle input_back;

	vk::image_handle output_base;
	vk::image_handle output_back;
};

struct blitter{
	struct ui_blit_info{
		math::usize2 offset{};
		math::usize2 cap{};

		friend bool operator==(const ui_blit_info& lhs, const ui_blit_info& rhs) noexcept = default;
	};

private:
	std::size_t current_blit_chunk_index{};
	vk::command_chunk blit_command_chunk{};

	std::vector<ui_blit_info> blit_infos{};
	indirect_dispatcher dispatcher{};

	vk::descriptor_layout descriptor_layout_{};
	vk::descriptor_buffer descriptor_buffer_{};
	vk::uniform_buffer uniform_buffer_{};

	vk::pipeline_layout blit_pipeline_layout_{};
	vk::pipeline blit_pipeline_{};
public:
	[[nodiscard]] blitter() = default;

	[[nodiscard]] blitter(
		vk::context& ctx,
		const std::size_t chunk_count,
		const VkPipelineShaderStageCreateInfo& shaderStageInfo
		) :
	blit_command_chunk(ctx.get_device(), ctx.get_compute_command_pool(), chunk_count)
	, blit_infos(chunk_count)
	, dispatcher(ctx.get_allocator(), chunk_count)

	, descriptor_layout_(ctx.get_device(), VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT, [](vk::descriptor_layout_builder& builder){
		builder.push_seq(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);

		builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
		builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);

		builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
		builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
	})
	, descriptor_buffer_(ctx.get_allocator(), descriptor_layout_, descriptor_layout_.binding_count(), chunk_count)
	, uniform_buffer_(ctx.get_allocator(), sizeof(ui_blit_info) * chunk_count)
	, blit_pipeline_layout_(ctx.get_device(), {}, {descriptor_layout_})
	, blit_pipeline_(ctx.get_device(), blit_pipeline_layout_, VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT, shaderStageInfo)
	{
		const auto map = get_mapper();
		for(std::size_t i = 0; i < descriptor_buffer_.get_chunk_count(); ++i){
			(void)map.set_uniform_buffer(0,
				uniform_buffer_.get_address() + sizeof(ui_blit_info) * i,
				sizeof(ui_blit_info),
				i
			);
		}
	}

	[[nodiscard]] vk::descriptor_mapper get_mapper() {
		return vk::descriptor_mapper{descriptor_buffer_};
	}

	void update(const blit_resources& resources){
		{
			const auto map = get_mapper();
			for(std::size_t i = 0; i < descriptor_buffer_.get_chunk_count(); ++i){
				map.set_storage_image(1, resources.input_base.image_view, VK_IMAGE_LAYOUT_GENERAL, i);
				map.set_storage_image(2, resources.input_back.image_view, VK_IMAGE_LAYOUT_GENERAL, i);
				map.set_storage_image(3, resources.output_base.image_view, VK_IMAGE_LAYOUT_GENERAL, i);
				map.set_storage_image(4, resources.output_back.image_view, VK_IMAGE_LAYOUT_GENERAL, i);
			}
		}

		create_command(resources);
	}

	[[nodiscard]] auto& blit(math::rect_ortho_trivial<unsigned> region){
		auto& cur = blit_command_chunk[current_blit_chunk_index];

		static constexpr math::usize2 wg_unit_size{16, 16};
		const auto wg_size = region.extent.copy().add(wg_unit_size.copy().sub(1u, 1u)).div(wg_unit_size);
		const auto [rx, ry] = (wg_size * wg_unit_size - region.extent) / 2;
		if(region.src.x > rx)region.src.x -= rx;
		if(region.src.y > ry)region.src.y -= ry;

		if(blit_infos[current_blit_chunk_index].offset != region.src){
			const ui_blit_info info{region.src};
			blit_infos[current_blit_chunk_index] = info;

			cur.wait(blit_command_chunk.get_device());
			(void)vk::buffer_mapper{uniform_buffer_}.load(info, sizeof(ui_blit_info) * current_blit_chunk_index);
		}else{
			cur.wait(blit_command_chunk.get_device());
		}

		dispatcher.set_divided(wg_size);
		dispatcher.update(current_blit_chunk_index);
		current_blit_chunk_index = (current_blit_chunk_index + 1) % blit_command_chunk.size();

		return cur;
	}

private:
	void create_command(const blit_resources& resources){
		std::array attachments{resources.input_base.image, resources.input_back.image};
		vk::cmd::dependency_gen dependency_gen{};

		for (auto && [idx, unit] : blit_command_chunk | std::views::enumerate){
			const vk::scoped_recorder scoped_recorder{unit, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT};

			for(const auto attachment : attachments){
				dependency_gen.push(
					attachment,
					VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					VK_IMAGE_LAYOUT_GENERAL,
					vk::image::default_image_subrange
				);
			}

			dependency_gen.apply(scoped_recorder);

			blit_pipeline_.bind(scoped_recorder, VK_PIPELINE_BIND_POINT_COMPUTE);
			descriptor_buffer_.bind_chunk_to(scoped_recorder, VK_PIPELINE_BIND_POINT_COMPUTE, blit_pipeline_layout_, 0, idx);

			vkCmdDispatchIndirect(scoped_recorder, dispatcher, dispatcher.offset_at(idx));

			for(const auto attachment : attachments){
				dependency_gen.push(
					attachment,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
					VK_PIPELINE_STAGE_2_TRANSFER_BIT,
					VK_ACCESS_2_TRANSFER_WRITE_BIT,
					VK_IMAGE_LAYOUT_GENERAL,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					vk::image::default_image_subrange
				);
			}

			dependency_gen.apply(scoped_recorder);

			static constexpr VkClearColorValue clear{};
			for(const auto attachment : attachments){
				vkCmdClearColorImage(
					scoped_recorder,
					attachment, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					&clear,
					1, &vk::image::default_image_subrange);
			}

			for(const auto attachment : attachments){
				dependency_gen.push(
					attachment,
					VK_PIPELINE_STAGE_2_TRANSFER_BIT,
					VK_ACCESS_2_TRANSFER_WRITE_BIT,
					VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					vk::image::default_image_subrange
				);
			}

			dependency_gen.apply(scoped_recorder);
		}
	}
};
#pragma endregion

export
struct draw_mode_pipeline_data{
	vk::pipeline_layout pipeline_layout{};
	vk::pipeline pipeline{};
	graphic::draw::instruction::batch_command_slots command_slots{};

	[[nodiscard]] draw_mode_pipeline_data() = default;

	[[nodiscard]] explicit draw_mode_pipeline_data(const vk::command_pool& command_pool)
	: command_slots(command_pool){
	}
};

export
struct pipeline_creator{
	vk::shader_module* shader_module;
	void(* creator)(vk::pipeline&, const vk::shader_module&, const vk::pipeline_layout&);

	[[nodiscard]] pipeline_creator() = default;

	[[nodiscard]] pipeline_creator(vk::shader_module& shader_module,
		void(* creator)(vk::pipeline&, const vk::shader_module&, const vk::pipeline_layout&))
	: shader_module(std::addressof(shader_module)),
	creator(creator){
	}
};

//TODO adopt heap allocator from scene?

export
struct renderer{
private:
	graphic::draw::instruction::user_data_index_table ubo_table_{
		graphic::draw::instruction::make_user_data_index_table<gui_reserved_user_data_tuple_uniform_buffer_used>()
	};

public:
	graphic::draw::instruction::batch batch{};
private:
	graphic::draw::instruction::batch_descriptor_slots general_descriptors_{};
	vk::uniform_buffer uniform_buffer_{};

	std::array<draw_mode_pipeline_data, std::to_underlying(draw_mode::COUNT_or_fallback)> pipeline_slots{};


	//screen space to uniform space viewport
	mr::vector<layer_viewport> viewports{};
	math::mat3 uniform_proj{};

	vk::color_attachment attachment_base{};
	vk::color_attachment attachment_background{};

	//Blit State
	blitter blitter_{};
	vk::storage_image blit_base{};
	vk::storage_image blit_background{};

	//Draw Mode State
	mr::vector<draw_mode_param> draw_mode_history_{};
	draw_mode_param current_draw_mode_{};

	//Cache
	mr::vector<VkCommandBufferSubmitInfo> cache_command_buffer_submit_info_{};
	mr::vector<VkSubmitInfo2> cache_submit_info_{};


public:
	[[nodiscard]] renderer() = default;

	[[nodiscard]] explicit renderer(
		vk::context& ctx,
		VkSampler spr,
		const std::array<pipeline_creator, std::to_underlying(draw_mode::COUNT_or_fallback)>& pipe_creator,
		const vk::shader_module& ui_main_blit_shader
	)
	: batch(ctx, spr,
		graphic::draw::instruction::make_user_data_index_table<gui_reserved_user_data_tuple>({
				{}, {}, graphic::draw::instruction::user_data_flag::fence_like,
				graphic::draw::instruction::user_data_flag::fence_like
			}))
	, general_descriptors_(ctx, [](vk::descriptor_layout_builder& b){
		b.push_seq(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT);
		b.push_seq(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT);
	})
	, uniform_buffer_(ctx.get_allocator(), ubo_table_.required_capacity() * batch.work_group_count(),
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)

	, blitter_{ctx, 1, ui_main_blit_shader.get_create_info(VK_SHADER_STAGE_COMPUTE_BIT)}{
		general_descriptors_.bind([&](const std::uint32_t idx, const vk::descriptor_mapper& mapper){
			using T = graphic::draw::instruction::user_data_index_table;

			for(auto&& [binding, ientry] : ubo_table_.get_entries()
			    | std::views::take_while([](const T::identity_entry& l){
				    return l.entry.group_index == 0;
			    })
			    | std::views::enumerate){
				auto& entry = ientry.entry;
				(void)mapper.set_uniform_buffer(
					binding,
					uniform_buffer_.get_address() + idx * ubo_table_.required_capacity() + entry.local_offset, entry.size,
					idx
					);
			}
		});

		batch.set_submit_callback([this](const graphic::draw::instruction::batch::command_acquire_config& config){
			VkSemaphoreSubmitInfo blit_semaphore_submit_info{};
			using namespace graphic::draw::instruction;

			if(config.data_entries){
				vk::buffer_mapper mapper{uniform_buffer_};
				for (const auto & entry : config.data_entries.entries){
					const auto data_span = entry.to_range(config.data_entries.base_address);

					switch(entry.global_offset){
					case gui::data_offset_of<gui::blit_config> :{
						process_blit_(config, blit_semaphore_submit_info,
							data_span);
						break;
					}
					case gui::data_offset_of<gui::draw_mode_param> :{
						draw_mode_param param;
						std::memcpy(&param, data_span.data(), data_span.size_bytes());
						if(param.mode_ == draw_mode::COUNT_or_fallback){
							draw_mode_history_.pop_back();
							current_draw_mode_ = draw_mode_history_.back();
						}else{
							draw_mode_history_.push_back(current_draw_mode_);
							current_draw_mode_ = param;
						}
						break;
					}
					default :{
						for(const unsigned idx : config.group_indices){
							(void)mapper.load_range(data_span,
								entry.local_offset + idx * ubo_table_.required_capacity());
						}
					}
					}
				}
			}

			// VkSemaphoreSubmitInfo blit_semaphore_submit_info_wait{blit_semaphore_submit_info};
			// blit_semaphore_submit_info_wait.stageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			cache_command_buffer_submit_info_.reserve(config.group_indices.size());
			for(const auto [i, group_idx] : config.group_indices | std::views::enumerate){
				auto& cmd_ref = cache_command_buffer_submit_info_.emplace_back(VkCommandBufferSubmitInfo{
					.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
					.commandBuffer = pipeline_slots[std::to_underlying(current_draw_mode_.mode_)].command_slots[group_idx],
				});

				//TODO wait blit?
				cache_submit_info_.push_back({
					.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
					// .waitSemaphoreInfoCount = (blit_semaphore_submit_info_wait.semaphore ? 1u : 0u),
					// .pWaitSemaphoreInfos = (blit_semaphore_submit_info_wait.semaphore ? &blit_semaphore_submit_info_wait : nullptr),
					.commandBufferInfoCount = 1,
					.pCommandBufferInfos = &cmd_ref,
					.signalSemaphoreInfoCount = 2,
					.pSignalSemaphoreInfos = config.group_semaphores.data() + i * 2
				});
			}

			if(cache_submit_info_.empty())return;
			vkQueueSubmit2(context().graphic_queue(), cache_submit_info_.size(), cache_submit_info_.data(), nullptr);

			cache_command_buffer_submit_info_.clear();
			cache_submit_info_.clear();
		});

		for (auto && vkCommandBuffers : pipeline_slots){
			vkCommandBuffers = draw_mode_pipeline_data{ctx.get_graphic_command_pool()};
		}

		init_pipeline(pipe_creator);
		draw_mode_history_.reserve(16);
		draw_mode_history_.push_back(current_draw_mode_);
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

		blit_base =
			vk::storage_image{
				context().get_allocator(), extent,
				1,
				VK_FORMAT_R16G16B16A16_SFLOAT,
				VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			};

		blit_background =
			vk::storage_image{
				context().get_allocator(), extent,
				1,
				VK_FORMAT_R8G8B8A8_UNORM,
				VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
			};

		{
			auto transient = context().get_transient_graphic_command_buffer();
			attachment_base.init_layout(transient);
			attachment_background.init_layout(transient);

			blit_base.init_layout_general(transient);
			blit_background.init_layout_general(transient);
		}

		blitter_.update({
			attachment_base, attachment_background,
			blit_base, blit_background
		});
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
			{attachment_base.get_image_view(), attachment_background.get_image_view()},
			nullptr};

		const graphic::draw::instruction::batch_descriptor_buffer_binding_info dbo_info{
			general_descriptors_.dbo(),
			general_descriptors_.dbo().get_chunk_size()
		};

		for(const auto& arr : pipeline_slots){
			batch.record_command(arr.pipeline_layout, std::span(&dbo_info, 1), [&] -> std::generator<VkCommandBuffer&&>{
				for(const auto& [idx, buf] : arr.command_slots | std::views::enumerate){
					vk::scoped_recorder recorder{buf, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT};

					dynamic_rendering.begin_rendering(recorder, context().get_screen_area());
					arr.pipeline.bind(recorder, VK_PIPELINE_BIND_POINT_GRAPHICS);

					vk::cmd::set_viewport(recorder, get_screen_area());
					vk::cmd::set_scissor(recorder, get_screen_area());

					co_yield buf.get();

					vkCmdEndRendering(recorder);
				}
			}());
		}

	}

	void init_pipeline(const std::array<pipeline_creator, std::to_underlying(draw_mode::COUNT_or_fallback)>& creators){
		for (const auto & [pipe, creator] : std::views::zip(pipeline_slots, creators)){
			pipe.pipeline_layout = vk::pipeline_layout(
				context().get_device(), 0,
			{batch.get_batch_descriptor_layout(), general_descriptors_.descriptor_set_layout()});

			creator.creator(pipe.pipeline, *creator.shader_module, pipe.pipeline_layout);
		}
	}

public:
	void wait_idle(){
		batch.consume_all();
		batch.wait_all();
		assert(batch.is_all_done());
	}


#pragma region Getter
	[[nodiscard]] vk::image_handle get_base() const noexcept{
		return blit_base;
	}

	[[nodiscard]] const math::mat3& get_screen_uniform_proj() const noexcept{
		return uniform_proj;
	}
#pragma endregion

	renderer_frontend create_frontend(){
		return renderer_frontend{graphic::draw::instruction::get_batch_interface(batch)};
	}

private:
	void process_blit_(
		const graphic::draw::instruction::batch::command_acquire_config& config,
		VkSemaphoreSubmitInfo& blit_semaphore_submit_info,
		std::span<const std::byte> data_span
		){
		math::rect_ortho_trivial<unsigned> rect;
		std::memcpy(&rect, data_span.data(), data_span.size_bytes());

		/*
		std::uint32_t blit_wait_count{};

		batch.for_each_submit([&](unsigned idx, batch::work_group& group){
			if(std::ranges::contains(config.group_indices, idx))return;
			blit_to_waits[blit_wait_count] = VkSemaphoreSubmitInfo{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.semaphore = group.get_attc_semaphore(),
				.value = group.get_current_signal_index(),
				.stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			};

			++blit_wait_count;
		});*/

		auto& cmd_unit = blitter_.blit(rect);
		blit_semaphore_submit_info = cmd_unit.get_next_semaphore_submit_info(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		cache_command_buffer_submit_info_.reserve(1 + config.group_indices.size());
		cache_command_buffer_submit_info_.push_back(cmd_unit.get_command_submit_info());

		cache_submit_info_.push_back(VkSubmitInfo2{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.commandBufferInfoCount = 1,
			.pCommandBufferInfos = cache_command_buffer_submit_info_.data(),
			.signalSemaphoreInfoCount = 1,
			.pSignalSemaphoreInfos = &blit_semaphore_submit_info
		});
	}
};

}
