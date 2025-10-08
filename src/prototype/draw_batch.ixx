module;

#include <vulkan/vulkan.h>

#include "../ext/adapted_attributes.hpp"

export module draw_batch;

import draw_instruction;

import mo_yanxi.vk.context;
import mo_yanxi.vk.command_buffer;
import mo_yanxi.vk.uniform_buffer;
import mo_yanxi.vk.descriptor_buffer;
import mo_yanxi.vk.pipeline.layout;
import mo_yanxi.vk.resources;
import mo_yanxi.vk.sync;
import mo_yanxi.vk.ext;
import mo_yanxi.vk.util.cmd.render;

import std;

namespace mo_yanxi::graphic{
	constexpr inline std::size_t maximum_images = 4;

	constexpr inline std::size_t max_dispatch_per_workgroup = 32;
	constexpr inline std::size_t working_group_count = 4;

	using dspt_info_buffer = std::array<draw::dispatch_group_info, max_dispatch_per_workgroup>;

	struct working_group{
		image_view_history image_view_history{};
		//TODO UBO data
		dspt_info_buffer dispatch_info{};
		VkDrawMeshTasksIndirectCommandEXT indirect{};

		std::byte* span_begin;
		std::byte* span_end;

		std::size_t current_ubo_index_{};
		std::uint64_t current_signal_index{};
		vk::semaphore mesh_semaphore{};
		vk::semaphore frag_semaphore{};


		void dispatch(std::uint32_t groups, std::byte* instr_span_begin, std::byte* instr_span_end/*, vk::buffer& ubo, std::span<std::byte> ubo_data*/) noexcept{
			indirect = {groups, 1, 1};
			span_begin = instr_span_begin;
			span_end = instr_span_end;
			// vk::buffer_mapper{ubo}.load_range(ubo_data, )
		}
	};

	export
	struct batch_external_data{
		struct work_group_data{
			vk::command_buffer command_buffer{};
		};

		vk::descriptor_layout user_descriptor_layout_{};
		vk::descriptor_buffer user_descriptor_buffer_{};

		std::array<work_group_data, working_group_count> groups{};

		[[nodiscard]] batch_external_data() = default;

		[[nodiscard]] explicit batch_external_data(
			vk::context& ctx,
			std::regular_invocable<vk::descriptor_layout_builder&> auto desc_layout_builder
		)
			:
		user_descriptor_layout_(ctx.get_device(), VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT, desc_layout_builder),
		user_descriptor_buffer_{ctx.get_allocator(), user_descriptor_layout_, user_descriptor_layout_.binding_count(),
			static_cast<std::uint32_t>(groups.size())
		}

		{
			for (auto && group : groups){
				group.command_buffer = ctx.get_graphic_command_pool().obtain();
			}
		}


		void bind(std::invocable<std::uint32_t, const vk::descriptor_mapper&> auto group_binder){
			vk::descriptor_mapper m{user_descriptor_buffer_};

			for (auto && [idx, g] : groups | std::views::enumerate){
				std::invoke(group_binder, idx, m);
			}
		}

		[[nodiscard]] VkDescriptorSetLayout descriptor_set_layout() const noexcept{
			return user_descriptor_layout_;
		}
	};

	export
	struct instruction_batch{
		static constexpr std::uint32_t batch_work_group_count = working_group_count;
		vk::context* context_{};
		vk::descriptor_layout descriptor_layout_{};
		vk::descriptor_buffer descriptor_buffer_{};

		std::array<working_group, working_group_count> groups_{};
		std::uint32_t current_idle_group_index_{};
		std::uint32_t current_dspt_group_index_ = groups_.size();

		//TODO ubo data
		alignas(16) std::array<std::byte, 1024> latest_user_ubo_data_{};
		std::uint32_t user_ubo_data_size_{};
		std::size_t current_ubo_index_{};


		draw::instruction_buffer instruction_buffer_{};
		std::byte* instruction_idle_ptr_{};
		std::byte* instruction_pend_ptr_{};
		std::byte* instruction_dspt_ptr_{};
		std::uint32_t last_primit_offset_{};
		std::uint32_t last_vertex_offset_{};
		std::uint32_t last_shared_instr_size_{};

		vk::buffer indirect_buffer{};
		vk::uniform_buffer dispatch_info_buffer{};
		vk::buffer instruction_gpu_buffer{};
		vk::uniform_buffer user_uniform_info_buffer{};

		std::move_only_function<VkCommandBuffer(std::uint32_t, std::span<const std::byte>)> on_submit{};

		[[nodiscard]] instruction_batch() = default;

		[[nodiscard]] explicit instruction_batch(
			vk::context& context
			)
		:
		context_(&context)
		, descriptor_layout_{
			context.get_device(), VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT, [](vk::descriptor_layout_builder& builder){
				builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT); // vertices info
				builder.push_seq(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT); //task barriers
				// builder.push_seq(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, maximum_images);
			}
		}
		, descriptor_buffer_(context.get_allocator(), descriptor_layout_, descriptor_layout_.binding_count(), working_group_count)
		, instruction_buffer_(1U << 11U)
		, indirect_buffer(vk::templates::create_storage_buffer(context.get_allocator(), sizeof(VkDrawMeshTasksIndirectCommandEXT) * working_group_count, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT))
		, dispatch_info_buffer(context.get_allocator(), sizeof(dspt_info_buffer) * working_group_count, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
		, instruction_gpu_buffer(vk::templates::create_storage_buffer(context.get_allocator(), instruction_buffer_.size(), VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT))

		{
			instruction_idle_ptr_ = instruction_buffer_.begin();
			instruction_dspt_ptr_ = instruction_buffer_.begin();
			instruction_pend_ptr_ = instruction_buffer_.begin();

			vk::descriptor_mapper mapper{descriptor_buffer_};
			for(std::uint32_t i = 0; i < working_group_count; ++i){
				(void)mapper.set_uniform_buffer(1, dispatch_info_buffer.get_address() + sizeof(dspt_info_buffer) * i, sizeof(dspt_info_buffer), i);
			}

			for (auto&& group : groups_){
				group.mesh_semaphore = vk::semaphore{context_->get_device(), 0};
				group.frag_semaphore = vk::semaphore{context_->get_device(), 0};
			}
		}

		template <draw::known_instruction T>
		void push_instruction(const T& instr){
			{
				const auto head = get_instruction_placement_ptr();
				if(check_need_block<T>(head)){
					wait_last_group_and_pop();
				}
				instruction_idle_ptr_ = draw::place_instruction_at(head, instruction_buffer_.end(), instr);
			}
			if(instruction_idle_ptr_ == nullptr){
				instruction_idle_ptr_ = instruction_buffer_.begin();

				const auto head = get_instruction_placement_ptr();
				if(check_need_block<T>(head)){
					wait_last_group_and_pop();
				}
				instruction_idle_ptr_ = draw::place_instruction_at(head, instruction_buffer_.end(), instr);
				assert(instruction_idle_ptr_ != nullptr);
			}
		}

		template <typename T>
		void update_ubo(const T& instr){
			{
				const auto head = get_instruction_placement_ptr();
				if(check_need_block<T>(head)){
					wait_last_group_and_pop();
				}
				instruction_idle_ptr_ = draw::place_ubo_update_at(head, instruction_buffer_.end(), instr);
			}
			if(instruction_idle_ptr_ == nullptr){
				instruction_idle_ptr_ = instruction_buffer_.begin();

				const auto head = get_instruction_placement_ptr();
				if(check_need_block<T>(head)){
					wait_last_group_and_pop();
				}
				instruction_idle_ptr_ = draw::place_ubo_update_at(head, instruction_buffer_.end(), instr);
				assert(instruction_idle_ptr_ != nullptr);
			}
		}


		[[nodiscard]] bool is_all_done() const noexcept{
			return instruction_idle_ptr_ == instruction_dspt_ptr_ && instruction_idle_ptr_ == instruction_pend_ptr_;
		}

		void consume_all(){
			while(consume_one()){}
		}

		bool consume_one(){
			if(is_all_pending()){
				wait_last_group_and_pop();
			}

			auto& group = groups_[current_idle_group_index_];
			const auto begin = instruction_pend_ptr_;
			const auto sentinel = instruction_idle_ptr_ >= instruction_pend_ptr_ ? instruction_idle_ptr_ : instruction_buffer_.end();
			const auto dspcinfo = draw::get_dispatch_info(instruction_pend_ptr_, sentinel, group.dispatch_info, group.image_view_history, last_primit_offset_, last_vertex_offset_);

			last_primit_offset_ = dspcinfo.next_primit_offset;
			last_vertex_offset_ = dspcinfo.next_vertex_offset;
			if(dspcinfo.img_requires_update){
				throw std::invalid_argument{"not impl"};
				//TODO image update strategy
			}

			if(dspcinfo.ubo_requires_update){
				//TODO skip contiguous ubo update til last one ?
				const auto data = draw::get_ubo_data(dspcinfo.next);
				std::memcpy(latest_user_ubo_data_.data(), data.data(), data.size_bytes());
				user_ubo_data_size_ = data.size_bytes();
				++current_ubo_index_;

				instruction_pend_ptr_ = dspcinfo.next + data.size_bytes() + sizeof(draw::instruction_head);
				if(!dspcinfo.count){
					instruction_dspt_ptr_ = instruction_pend_ptr_;
				}
			}else{
				instruction_pend_ptr_ = dspcinfo.next;
			}

			if(dspcinfo.count){
				const auto off = begin - instruction_buffer_.begin();
				assert(dspcinfo.next >= begin);

				const auto shared_instr_size = (dspcinfo.contains_next() ? draw::get_instr_head(dspcinfo.next).get_instr_byte_size() : 0);
				const auto instr_size = dspcinfo.next - begin + shared_instr_size;

				(void)vk::buffer_mapper{instruction_gpu_buffer}
					.load_range(
						std::span{begin + last_shared_instr_size_, dspcinfo.next + shared_instr_size},
						off + last_shared_instr_size_);

				(void)vk::buffer_mapper{dispatch_info_buffer}
					.load_range(std::span{group.dispatch_info}, sizeof(dspt_info_buffer) * current_idle_group_index_);

				(void)vk::descriptor_mapper{descriptor_buffer_}
					.set_storage_buffer(0,
						instruction_gpu_buffer.get_address() + off,
						instr_size,

						current_idle_group_index_);


				assert(instruction_pend_ptr_ >= begin);
				group.dispatch(dspcinfo.count, begin, instruction_pend_ptr_);
				assert(group.span_end >= begin);

				(void)vk::buffer_mapper{indirect_buffer}
					.load(group.indirect, sizeof(VkDrawMeshTasksIndirectCommandEXT) * current_idle_group_index_);

				last_shared_instr_size_ = shared_instr_size;

				//usr update
				assert(on_submit);
				std::span<const std::byte> ubodata{};
				if(group.current_ubo_index_ < current_ubo_index_){
					ubodata = {latest_user_ubo_data_.data(), user_ubo_data_size_};
					group.current_ubo_index_ = current_ubo_index_;
				}
				const auto cmdbuf =
					on_submit(current_idle_group_index_, ubodata);

				//GPU submit
				++group.current_signal_index;
				std::array<VkSemaphoreSubmitInfo, 2> semaphore_submit_infos{
					VkSemaphoreSubmitInfo{
						.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
						.semaphore = group.mesh_semaphore,
						.value = group.current_signal_index,
						.stageMask = VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT,
					},
					VkSemaphoreSubmitInfo{
						.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
						.semaphore = group.frag_semaphore,
						.value = group.current_signal_index,
						.stageMask = VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT,
					}
				};

				VkCommandBufferSubmitInfo submitInfo{
					.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
					.commandBuffer = cmdbuf,
				};

				VkSubmitInfo2 submitInfo2{
					.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
					.waitSemaphoreInfoCount = 0,
					.pWaitSemaphoreInfos = nullptr,
					.commandBufferInfoCount = 1,
					.pCommandBufferInfos = &submitInfo,
					.signalSemaphoreInfoCount = semaphore_submit_infos.size(),
					.pSignalSemaphoreInfos = semaphore_submit_infos.data()
				};

				vkQueueSubmit2(context_->graphic_queue(), 1, &submitInfo2, nullptr);
				// vk::cmd::q

				push_current_group();
			}

			if(draw::get_instr_head(dspcinfo.next).type == draw::draw_instruction_type::noop && instruction_pend_ptr_ > instruction_idle_ptr_){
				instruction_pend_ptr_ = instruction_buffer_.begin();
				return true;
			}

			return !dspcinfo.no_next(begin);
		}

		void wait_last_group_and_pop(bool wait_on_frag = false){
			if(const auto group = get_front_group()){
				if(wait_on_frag){
					group->frag_semaphore.wait(group->current_signal_index);
				}else{
					group->mesh_semaphore.wait(group->current_signal_index);
				}

				auto idx_1 = group->span_begin - instruction_buffer_.begin();
				auto idx_2 = group->span_end - instruction_buffer_.begin();

				instruction_dspt_ptr_ = group->span_end;

				std::memset(group->dispatch_info.data(), 0, sizeof(dspt_info_buffer));
				std::memset(group->span_begin, 0, group->span_end - group->span_begin);

				instruction_dspt_ptr_ = group->span_end;
				pop_front_group();
			}
		}

		void wait_all(bool wait_on_frag = false){
			while(!is_all_idle()){
				wait_last_group_and_pop(wait_on_frag);
			}
		}

		void record_command(const VkPipelineLayout layout, std::generator<VkCommandBuffer&&>&& cmdGen) const{
			for (auto&& [idx, buf] : std::move(cmdGen) | std::views::take(groups_.size()) | std::ranges::views::enumerate){
				descriptor_buffer_.bind_chunk_to(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, idx);
				vk::cmd::drawMeshTasksIndirect(buf, indirect_buffer, idx * sizeof(VkDrawMeshTasksIndirectCommandEXT));
			}
		}

		[[nodiscard]] VkDescriptorSetLayout get_batch_descriptor_layout() const noexcept{
			return descriptor_layout_;
		}

	private:
		[[nodiscard]] working_group* get_front_group() noexcept{
			return is_all_idle() ? nullptr : groups_.data() + current_dspt_group_index_;
		}

		template <typename InstrT>
		bool check_need_block(const std::byte* where) const noexcept{
			const auto end = where + draw::get_instr_size<InstrT>();

			if(is_all_idle()){
				return false;
			}

			if(instruction_dspt_ptr_ == instruction_idle_ptr_)return true;

			if(instruction_dspt_ptr_ > instruction_idle_ptr_){
				return end > instruction_dspt_ptr_;
			}else{
				//reversed, where next instruction will never directly collide with dispatched instructions;
				return false;
			}
		}

		[[nodiscard]] bool is_all_idle() const noexcept{
			return current_dspt_group_index_ == groups_.size();
		}

		void pop_front_group() noexcept{
			current_dspt_group_index_ = (current_dspt_group_index_ + 1) % groups_.size();
			if(current_dspt_group_index_ == current_idle_group_index_){
				current_dspt_group_index_ = groups_.size();
			}
		}

		[[nodiscard]] bool is_all_pending() const noexcept{
			return current_dspt_group_index_ == current_idle_group_index_;
		}

		void push_current_group() noexcept{
			if(is_all_idle()){
				current_dspt_group_index_ = current_idle_group_index_;
			}
			current_idle_group_index_ = (current_idle_group_index_ + 1) % groups_.size();
		}

		[[nodiscard]] std::byte* get_instruction_placement_ptr() const noexcept{
			const auto ret = std::assume_aligned<16>(instruction_idle_ptr_ ? instruction_idle_ptr_ : instruction_buffer_.begin());
			CHECKED_ASSUME(ret != nullptr);
			return ret;
		}
	};
}
