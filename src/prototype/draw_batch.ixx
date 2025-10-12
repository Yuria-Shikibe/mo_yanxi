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

	struct dspt_info_buffer{
		std::uint32_t shared_instr_image_index_override;
		std::uint32_t cap[3];
		std::array<draw::dispatch_group_info, max_dispatch_per_workgroup> group_info;
	};

	struct working_group{
		image_view_history image_view_history{};
		//TODO UBO data
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

	private:
		vk::context* context_{};
		vk::descriptor_layout descriptor_layout_{};
		vk::descriptor_buffer descriptor_buffer_{};

		dspt_info_buffer temp_dispatch_info_{};
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

		vk::uniform_buffer dispatch_info_buffer{};

		//TODO merge these two?
		vk::buffer indirect_buffer{};
		vk::buffer instruction_gpu_buffer{};

		VkSampler sampler_{};

	public:
		std::move_only_function<VkCommandBuffer(std::uint32_t, std::span<const std::byte>)> on_submit{};

		[[nodiscard]] instruction_batch() = default;

		[[nodiscard]] explicit instruction_batch(
			vk::context& context,
			VkSampler sampler
			)
		:
		context_(&context),
		descriptor_layout_{
			context.get_device(), VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT, [](vk::descriptor_layout_builder& builder){
				builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT); // vertices info
				builder.push_seq(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT); // task barriers
				builder.push_seq(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, maximum_images);
			}
		}
		, descriptor_buffer_(context.get_allocator(), descriptor_layout_, descriptor_layout_.binding_count(), working_group_count)
		, instruction_buffer_(1U << 12U)
		, dispatch_info_buffer(context.get_allocator(), sizeof(dspt_info_buffer) * working_group_count, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
		, indirect_buffer(vk::templates::create_storage_buffer(context.get_allocator(), sizeof(VkDrawMeshTasksIndirectCommandEXT) * working_group_count, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT))
		, instruction_gpu_buffer(vk::templates::create_storage_buffer(context.get_allocator(), instruction_buffer_.size(), VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT))
		, sampler_(sampler)

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

	private:

		template <typename T, typename Fn>
		FORCE_INLINE void push_impl(Fn psh_fn){
			assert(draw::get_instr_placement_size_requirement<T>() <= instruction_buffer_.size());

			std::byte* rst{nullptr};
			while(check_need_block<T>(instruction_idle_ptr_)){
				if(is_all_idle())consume_n(working_group_count / 2);
				wait_last_group_and_pop();
			}

			rst = psh_fn(instruction_idle_ptr_);

			if(rst == nullptr){
				//Reverse buffer to head
				while(check_need_block<T>(instruction_buffer_.begin())){
					if(is_all_idle())consume_n(working_group_count / 2);
					wait_last_group_and_pop();
				}

				rst = psh_fn(instruction_buffer_.begin());
			}
			assert(instruction_idle_ptr_ != nullptr);
			instruction_idle_ptr_ = rst;
		}

	public:
		template <draw::known_instruction T>
		void push_instruction(const T& instr){
			this->push_impl<T>([&, this](std::byte* where){
				return draw::place_instruction_at(where, instruction_buffer_.end(), instr);
			});
		}

		template <typename T>
			requires (!draw::known_instruction<T>)
		void update_ubo(const T& instr){
			this->push_impl<T>([&, this](std::byte* where){
				return draw::place_ubo_update_at(where, instruction_buffer_.end(), instr);
			});
		}

		[[nodiscard]] bool is_all_done() const noexcept{
			return instruction_idle_ptr_ == instruction_dspt_ptr_ && instruction_idle_ptr_ == instruction_pend_ptr_ && is_all_idle();
		}

		void consume_all(){
			while(consume_n(working_group_count)){}
		}


		bool consume_n(unsigned count){
			assert(count <= working_group_count);
			assert(count > 0);
			std::array<VkSemaphoreSubmitInfo, working_group_count * 2> semaphores{};
			std::array<VkCommandBufferSubmitInfo, working_group_count> command_buffers{};
			std::array<VkSubmitInfo2, working_group_count> submitInfo2{};
			unsigned actuals{};
			const auto intital_wait = get_pushed_group_count();
			const auto initial_remain = groups_.size() - intital_wait;
			bool unfinished = true;

			while(true){
				auto& group = groups_[current_idle_group_index_];
				const auto begin = instruction_pend_ptr_;
				const auto sentinel = instruction_idle_ptr_ >= instruction_pend_ptr_ ? instruction_idle_ptr_ : instruction_buffer_.end();
				const auto dspcinfo = draw::get_dispatch_info(instruction_pend_ptr_, sentinel, temp_dispatch_info_.group_info, group.image_view_history, last_primit_offset_, last_vertex_offset_);
				const auto next_instr_type = draw::get_instr_head(dspcinfo.next).type;

				// if(dspcinfo.count)std::memset(temp_dispatch_info_.group_info.data() + dspcinfo.count, 0, (temp_dispatch_info_.group_info.size() - dspcinfo.count) * sizeof(draw::dispatch_group_info));

				last_primit_offset_ = dspcinfo.next_primit_offset;
				last_vertex_offset_ = dspcinfo.next_vertex_offset;
				if(dspcinfo.img_requires_update){
					//TODO better image update strategy ?
					if(!dspcinfo.count){
						group.image_view_history.clear();
						continue;
					}
				}


				if(dspcinfo.ubo_requires_update){
					const auto data = draw::get_ubo_data(dspcinfo.next);

					if(actuals <= intital_wait && actuals >= initial_remain){
						wait_last_group_and_pop();
					}

					std::memcpy(latest_user_ubo_data_.data(), data.data(), data.size_bytes());
					user_ubo_data_size_ = data.size_bytes();
					++current_ubo_index_;

					instruction_pend_ptr_ = dspcinfo.next + data.size_bytes() + sizeof(draw::instruction_head);
					if(!dspcinfo.count && is_all_idle()){
						instruction_dspt_ptr_ = instruction_pend_ptr_;
					}
				}else{
					instruction_pend_ptr_ = dspcinfo.next;
				}

				if(dspcinfo.count){
					assert(dspcinfo.next >= begin);

					const auto instr_byte_off = begin - instruction_buffer_.begin();
					const auto instr_shared_size = (dspcinfo.contains_next() ? draw::get_instr_head(dspcinfo.next).get_instr_byte_size() : 0);
					const auto instr_size = dspcinfo.next - begin + instr_shared_size;

					const bool is_img_history_changed = group.image_view_history.check_changed();

					if(actuals <= intital_wait && actuals >= initial_remain){
						//If requires wait and is not undispatched command
						wait_last_group_and_pop(is_img_history_changed);
					}

					(void)vk::buffer_mapper{instruction_gpu_buffer}
					.load_range(
						std::span{begin + last_shared_instr_size_, dspcinfo.next + instr_shared_size},
						instr_byte_off + last_shared_instr_size_);

					//Resolved the condition when two drawcall share one instruction, so override the first instr img index is needed
					if(last_shared_instr_size_){
						auto& generic = reinterpret_cast<draw::primitive_generic&>(*(begin + sizeof(draw::instruction_head)));
						assert(generic.image.index < image_view_history::max_cache_count || generic.image.index == ~0U);

						if(draw::get_instr_head(begin).type == draw::draw_instruction_type::rectangle){
							assert(generic.image.index != ~0U);
						}
						temp_dispatch_info_.shared_instr_image_index_override = generic.image.index;
					}else{
						temp_dispatch_info_.shared_instr_image_index_override = image_view_history::max_cache_count;
					}

					if(instr_shared_size){
						//Resume next image from index to pointer to view
						auto& generic = reinterpret_cast<draw::primitive_generic&>(*(dspcinfo.next + sizeof(draw::instruction_head)));
						generic.image.set_view(dspcinfo.current_img);
					}

					if(is_img_history_changed){
						vk::descriptor_mapper{descriptor_buffer_}
						.set_image(2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler_, group.image_view_history.get(), current_idle_group_index_);
					}

					(void)vk::buffer_mapper{dispatch_info_buffer}
						.load(static_cast<const void*>(&temp_dispatch_info_), 16 + sizeof(draw::instruction_head) * dspcinfo.count, sizeof(dspt_info_buffer) * current_idle_group_index_);

					(void)vk::descriptor_mapper{descriptor_buffer_}
						.set_storage_buffer(0,
							instruction_gpu_buffer.get_address() + instr_byte_off,
							instr_size,
							current_idle_group_index_);


					assert(instruction_pend_ptr_ >= begin);
					group.dispatch(dspcinfo.count, begin, instruction_pend_ptr_);
					assert(group.span_end >= begin);

					(void)vk::buffer_mapper{indirect_buffer}
					.load(group.indirect, sizeof(VkDrawMeshTasksIndirectCommandEXT) * current_idle_group_index_);

					last_shared_instr_size_ = instr_shared_size;

					//usr update
					assert(on_submit);
					std::span<const std::byte> ubodata{};
					if(group.current_ubo_index_ < current_ubo_index_){
						ubodata = {latest_user_ubo_data_.data(), user_ubo_data_size_};
						group.current_ubo_index_ = current_ubo_index_;
					}

					const auto cmdbuf =
						on_submit(current_idle_group_index_, ubodata);

					++group.current_signal_index;

					semaphores[actuals * 2] = VkSemaphoreSubmitInfo{
						.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
						.semaphore = group.mesh_semaphore,
						.value = group.current_signal_index,
						.stageMask = VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT,
					};
					semaphores[actuals * 2 + 1] = VkSemaphoreSubmitInfo{
						.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
						.semaphore = group.frag_semaphore,
						.value = group.current_signal_index,
						.stageMask = VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT,
					};
					command_buffers[actuals] = VkCommandBufferSubmitInfo{
						.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
						.commandBuffer = cmdbuf,
					};

					submitInfo2[actuals] = VkSubmitInfo2{
						.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
						.waitSemaphoreInfoCount = 0,
						.pWaitSemaphoreInfos = nullptr,
						.commandBufferInfoCount = 1,
						.pCommandBufferInfos = command_buffers.data() + actuals,
						.signalSemaphoreInfoCount = 2,
						.pSignalSemaphoreInfos = semaphores.data() + actuals * 2,
					};

					++actuals;

					push_current_group();
				}

				bool hasNext = !dspcinfo.no_next(begin);

				if(next_instr_type == draw::draw_instruction_type::noop && instruction_pend_ptr_ > instruction_idle_ptr_){
					assert(last_shared_instr_size_ == 0);
					instruction_pend_ptr_ = instruction_buffer_.begin();
					hasNext = true;
				}

				if(actuals == count || !dspcinfo.count){
					if(hasNext)goto RET;
					break;
				}
			}

			unfinished = false;

			RET:
			if(actuals)vkQueueSubmit2(context_->graphic_queue(), actuals, submitInfo2.data(), nullptr);
			return unfinished;
		}

		//TODO consume multiple at one time

		void wait_last_group_and_pop(bool wait_on_frag = false){
			if(const auto group = get_front_group()){
				if(wait_on_frag){
					group->frag_semaphore.wait(group->current_signal_index);
				}else{
					group->mesh_semaphore.wait(group->current_signal_index);
				}

				// std::memset(group->dispatch_info.data(), 0, sizeof(dspt_info_buffer) );
				instruction_dspt_ptr_ = group->span_end;
				pop_front_group();
			}
		}

		void wait_n(const std::uint32_t count, const bool wait_on_frag = false){
			if(!count)return;

			std::array<std::uint64_t, working_group_count> values{};
			std::array<VkSemaphore, working_group_count> semaphores{};
			std::uint32_t pushed{};
			for_each_submit([&](unsigned, const working_group& group){
				if(pushed >= count)return;
				semaphores[pushed] = wait_on_frag ? group.frag_semaphore : group.mesh_semaphore;
				values[pushed] = group.current_signal_index;
				++pushed;
				instruction_dspt_ptr_ = group.span_end;
			});

			pop_n_group(pushed);

			vk::wait_multiple(context_->get_device(), {semaphores.data(), pushed}, {values.data(), pushed});
		}

		void wait_until(unsigned index, const bool wait_on_frag = false){
			bool found{};

			std::array<std::uint64_t, working_group_count> values{};
			std::array<VkSemaphore, working_group_count> semaphores{};
			std::uint32_t pushed{};
			std::byte* next_dspt{};
			for_each_submit([&](unsigned idx, const working_group& group){

				if(found)return;
				if(idx == index)found = true;
				semaphores[pushed] = wait_on_frag ? group.frag_semaphore : group.mesh_semaphore;
				values[pushed] = group.current_signal_index;
				++pushed;
				next_dspt = group.span_end;
			});

			if(!found)return;

			instruction_dspt_ptr_ = next_dspt;
			pop_n_group(pushed);

			vk::wait_multiple(context_->get_device(), {semaphores.data(), pushed}, {values.data(), pushed});
		}

		void wait_all(bool wait_on_frag = false){
			wait_n(get_pushed_group_count(), wait_on_frag);
		}

		void record_command(const VkPipelineLayout layout, std::generator<const vk::command_buffer&>&& cmdGen) const{
			for (auto&& [idx, buf] : std::move(cmdGen) | std::views::take(groups_.size()) | std::ranges::views::enumerate){
				descriptor_buffer_.bind_chunk_to(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, idx);
				vk::cmd::drawMeshTasksIndirect(buf, indirect_buffer, idx * sizeof(VkDrawMeshTasksIndirectCommandEXT));
			}
		}

		[[nodiscard]] VkDescriptorSetLayout get_batch_descriptor_layout() const noexcept{
			return descriptor_layout_;
		}

	private:
		template <std::invocable<unsigned, working_group&> Fn>
		FORCE_INLINE auto for_each_submit(Fn fn){
			if(is_all_idle())return;
			auto cur = current_dspt_group_index_;

			do{
				if constexpr (std::predicate<Fn, working_group&>){
					if(std::invoke(fn, cur, groups_[cur])){
						return cur;
					}
				}else{
					std::invoke(fn, cur, groups_[cur]);
				}

				cur = (cur + 1) % groups_.size();
			}while(cur != current_idle_group_index_);
		}

		[[nodiscard]] unsigned get_pushed_group_count() const noexcept{
			if(is_all_idle())return 0;
			return current_dspt_group_index_ < current_idle_group_index_ ? current_idle_group_index_ - current_dspt_group_index_ : groups_.size() - (current_dspt_group_index_ - current_idle_group_index_);
		}

		[[nodiscard]] working_group* get_front_group() noexcept{
			return is_all_idle() ? nullptr : groups_.data() + current_dspt_group_index_;
		}

		template <typename InstrT>
		bool check_need_block(const std::byte* where) const noexcept{
			const bool directly_done = draw::get_instr_head(where).type == draw::draw_instruction_type::noop;
			// return std::any_of(where, where + draw::get_instr_size<InstrT>(), std::to_integer<bool>);
			//
			// if(is_all_idle()){
			// 	return directly_done;
			// }
			//
			const auto end = where + draw::get_instr_size<InstrT>() + sizeof(draw::instruction_head);
			if(instruction_dspt_ptr_ == where)return !directly_done;

			if(instruction_dspt_ptr_ > where){
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

		FORCE_INLINE void pop_n_group(std::uint32_t count) noexcept{
			assert(count <= groups_.size());
			current_dspt_group_index_ = (current_dspt_group_index_ + count) % groups_.size();
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

		void push_group_n(unsigned n) noexcept{
			assert(n <= groups_.size() - get_pushed_group_count());
			if(is_all_idle()){
				current_dspt_group_index_ = current_idle_group_index_;
			}
			current_idle_group_index_ = (current_idle_group_index_ + n) % groups_.size();
		}

	};

	export
	template <typename T>
		requires (std::is_trivially_copyable_v<T>)
	FORCE_INLINE instruction_batch& operator<<(instruction_batch& batch, const T& instr){
		if constexpr (draw::known_instruction<T>){
			batch.push_instruction(instr);
		}else{
			batch.update_ubo(instr);
		}
		return batch;
	}
}
