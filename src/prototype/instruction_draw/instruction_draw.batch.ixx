module;

#include <vulkan/vulkan.h>
#include "ext/adapted_attributes.hpp"
#include "gch/small_vector.hpp"

export module mo_yanxi.graphic.draw.instruction.batch;

import mo_yanxi.vk;

import mo_yanxi.graphic.draw.instruction;

import mo_yanxi.type_register;
import std;


namespace mo_yanxi::graphic::draw::instruction{

#pragma region mesh_group_dispatch
struct dispatch_result{
	std::byte* next;
	std::uint32_t count;
	std::uint32_t next_primit_offset;
	std::uint32_t next_vertex_offset;
	bool ubo_requires_update;
	bool img_requires_update;

	VkImageView current_img{};

	[[nodiscard]] bool contains_next() const noexcept{
		return next_vertex_offset != 0;
	}

	bool host_process_required() const noexcept{
		return next == nullptr || ubo_requires_update || img_requires_update;
	}

	[[nodiscard]] bool no_next(std::byte* original) const noexcept{
		return next == original && count == 0 && !ubo_requires_update && !img_requires_update;
	}
};

struct alignas(16) dispatch_group_info{
	std::uint32_t instruction_offset; //offset in 16 Byte TODO remove the scaling?
	std::uint32_t vertex_offset;
	std::uint32_t primitive_offset;
	std::uint32_t primitive_count;
};

constexpr inline std::uint32_t MaxTaskDispatchPerTime = 32;
constexpr inline std::uint32_t MaxVerticesPerMesh = 64;

/**
 *
 * @brief parse given instructions between [begin, sentinel), store mesh dispatch info to storage
 */
FORCE_INLINE inline dispatch_result get_dispatch_info(
	std::byte* const instr_begin,
	const std::byte* const instr_sentinel,
	std::span<dispatch_group_info> storage,
	image_view_history& image_cache,
	const std::uint32_t initial_primitive_offset,
	const std::uint32_t initial_vertex_offset
) noexcept{
	CHECKED_ASSUME(!storage.empty());
	CHECKED_ASSUME(instr_sentinel >= instr_begin);

	std::uint32_t currentMeshCount{};
	const auto ptr_to_src = std::assume_aligned<16>(instr_begin);

	std::uint32_t verticesBreakpoint{initial_vertex_offset};
	std::uint32_t nextPrimitiveOffset{initial_primitive_offset};
	VkImageView current_img{};

	auto ptr_to_head = std::assume_aligned<16>(instr_begin);

	bool requires_image_try_push{true};

	while(currentMeshCount < storage.size()){
		storage[currentMeshCount].primitive_offset = nextPrimitiveOffset;
		storage[currentMeshCount].vertex_offset = verticesBreakpoint > 2
			                                          ? (verticesBreakpoint - 2)
			                                          : (verticesBreakpoint = 0);

		std::uint32_t pushedVertices{};
		std::uint32_t pushedPrimitives{};

		const auto ptr_to_chunk_head = ptr_to_head;

		const auto get_remain_vertices = [&] FORCE_INLINE{
			return MaxVerticesPerMesh - pushedVertices;
		};

		const auto save_chunk_head_and_incr = [&] FORCE_INLINE{
			if(pushedPrimitives == 0) return;
			assert((ptr_to_chunk_head - ptr_to_src) % 16 == 0);
			storage[currentMeshCount].instruction_offset = ptr_to_chunk_head - ptr_to_src;
			storage[currentMeshCount].primitive_count = pushedPrimitives;
			++currentMeshCount;
		};

		while(true){
			const auto& head = *start_lifetime_as<instruction_head>(ptr_to_head);
			assert(std::to_underlying(head.type) < std::to_underlying(instruction::instr_type::SIZE));

			if(ptr_to_head + head.get_instr_byte_size() > instr_sentinel){
				save_chunk_head_and_incr();
				return {
						ptr_to_head, currentMeshCount, nextPrimitiveOffset, verticesBreakpoint, false, false,
						current_img
					};
			}

			switch(head.type){
			case instr_type::noop : save_chunk_head_and_incr();
				return {ptr_to_head, currentMeshCount, nextPrimitiveOffset, verticesBreakpoint, false, false};
			case instr_type::uniform_update : save_chunk_head_and_incr();
				return {ptr_to_head, currentMeshCount, nextPrimitiveOffset, verticesBreakpoint, true, false};
			default : break;
			}

			if(requires_image_try_push){
				requires_image_try_push = false;
				if(auto rst = set_image_index(ptr_to_head + sizeof(instruction_head), image_cache); !rst){
					save_chunk_head_and_incr();
					return {
							ptr_to_head, currentMeshCount, nextPrimitiveOffset, verticesBreakpoint, false, true,
							rst.image
						};
				} else{
					current_img = rst.image;
				}
			}

			auto nextVertices = get_vertex_count(head.type, ptr_to_head);

			if(verticesBreakpoint){
				assert(verticesBreakpoint >= 3);
				assert(verticesBreakpoint < nextVertices);
				nextVertices -= (verticesBreakpoint -= 2); //make sure a complete primitive is draw
			}


			if(pushedVertices + nextVertices <= MaxVerticesPerMesh){
				pushedVertices += nextVertices;
				pushedPrimitives += get_primitive_count(head.type, ptr_to_head, nextVertices);

				verticesBreakpoint = 0;
				nextPrimitiveOffset = 0;

				// last_instr = ptr_to_head;
				ptr_to_head = std::assume_aligned<16>(ptr_to_head + head.get_instr_byte_size());
				requires_image_try_push = true;

				if(pushedVertices == MaxVerticesPerMesh) break;
			} else{
				const auto remains = get_remain_vertices();
				const auto primits = get_primitive_count(head.type, ptr_to_head, remains);
				nextPrimitiveOffset += primits;
				pushedPrimitives += primits;

				verticesBreakpoint += remains;

				break;
			}
		}

		save_chunk_head_and_incr();
	}

	return {ptr_to_head, currentMeshCount, nextPrimitiveOffset, verticesBreakpoint, false, false, current_img};
}
#pragma endregion

constexpr inline std::size_t maximum_images = 4;
constexpr inline std::size_t max_dispatch_per_workgroup = 32;
constexpr inline std::size_t working_group_count = 4;



export
struct ubo_index_table{
	struct entry{
		std::uint32_t size;
		std::uint32_t local_offset;
		std::uint32_t global_offset;
		std::uint32_t group_index;

		explicit operator bool() const noexcept{
			return size != 0;
		}

		[[nodiscard]] std::span<const std::byte> to_range(const std::byte* base_address) const noexcept{
			return {base_address + global_offset, size};
		}
	};

private:
	struct identity_entry{
		type_identity_index id;
		entry entry;
	};

	std::size_t maximum_size{};
	gch::small_vector<identity_entry, 4> entries{};


public:
	template <typename ...Ts>
	friend ubo_index_table make_ubo_table();

	[[nodiscard]] ubo_index_table() = default;

	[[nodiscard]] std::uint32_t max_size() const noexcept{
		return maximum_size;
	}

	[[nodiscard]] std::uint32_t group_size_at(const identity_entry* where) const noexcept{
		const auto end = entries.data() + entries.size();
		assert(where < end && where > entries.data());

		std::uint32_t base_offset{where->entry.group_index - where->entry.local_offset};
		for(auto p = where; p != end; p++){
			if(p->entry.group_index != where->entry.group_index){
				return (p->entry.group_index - p->entry.local_offset) - base_offset;
			}
		}

		return maximum_size - base_offset;
	}

	[[nodiscard]] std::size_t count() const noexcept{
		return entries.size();
	}

	[[nodiscard]] const identity_entry* operator[](type_identity_index id) const noexcept{
		const auto beg = entries.data();
		const auto end = beg + entries.size();
		return std::ranges::find(beg, end, id, &identity_entry::id);
	}

	template <typename T>
	[[nodiscard]] const identity_entry& at() const noexcept{
		auto ptr = (*this)[unstable_type_identity_of<T>()];
		assert(ptr != entries.data() + entries.size());
		return *ptr;
	}

	[[nodiscard]] const entry& operator[](std::uint32_t idx) const noexcept{
		assert(idx < entries.size());
		return entries[idx].entry;
	}

	template <typename T>
	[[nodiscard]] uniform_buffer_instr_info index_of() const noexcept{
		const auto pinfo = unstable_type_identity_of<T>();
		const identity_entry* ptr = (*this)[unstable_type_identity_of<T>()];
		assert(ptr < entries.data() + entries.size());
		return {static_cast<std::uint32_t>(ptr - entries.data()), ptr->entry.group_index};
	}

	auto get_entries() const noexcept {
		return std::span{entries.data(), entries.size()};
	}

	void append(const ubo_index_table& other){
		if(entries.empty()){
			*this = other;
			return;
		}
		auto group_base = entries.back().entry.group_index;
		entries.reserve(entries.size() + other.entries.size());
		for (auto entry : other.entries){
			entry.entry.group_index += group_base;
			entry.entry.global_offset += maximum_size;
			entries.push_back(entry);
		}
		maximum_size += other.maximum_size;
	}
};

export
template <typename ...Ts>
[[nodiscard]] ubo_index_table make_ubo_table(){
	ubo_index_table table{};

	auto push = [&]<typename T, std::size_t I>(std::size_t current_base_size){
		table.entries.push_back({
			.id = unstable_type_identity_of<T>(),
			.entry = {
				.size = static_cast<std::uint32_t>(sizeof(T)),
				.local_offset = static_cast<std::uint32_t>(table.maximum_size - current_base_size),
				.global_offset = static_cast<std::uint32_t>(table.maximum_size),
				.group_index = I
			}
		});

		table.maximum_size += math::div_ceil<std::uint32_t>(sizeof(T), 16) * 16;
	};

	[&]<std::size_t ...Idx>(std::index_sequence<Idx...>){
		([&]<typename T, std::size_t I>(){
			const auto cur_base = table.maximum_size;

			[&]<std::size_t ...J>(std::index_sequence<J...>){
				(push.template operator()<std::tuple_element_t<J, T>, I>(cur_base), ...);
			}(std::make_index_sequence<std::tuple_size_v<T>>{});
		}.template operator()<Ts, Idx>(), ...);
	}(std::index_sequence_for<Ts...>());

	return table;
}

struct dspt_info_buffer{
	std::uint32_t shared_instr_image_index_override;
	std::uint32_t cap[3];
	std::array<instruction::dispatch_group_info, max_dispatch_per_workgroup> group_info;
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
		user_descriptor_layout_(ctx.get_device(), VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
								desc_layout_builder),
		user_descriptor_buffer_{
			ctx.get_allocator(), user_descriptor_layout_, user_descriptor_layout_.binding_count(),
			static_cast<std::uint32_t>(groups.size())
		}{
		for(auto&& group : groups){
			group.command_buffer = ctx.get_graphic_command_pool().obtain();
		}
	}


	void bind(std::invocable<std::uint32_t, const vk::descriptor_mapper&> auto group_binder){
		vk::descriptor_mapper m{user_descriptor_buffer_};

		for(auto&& [idx, g] : groups | std::views::enumerate){
			std::invoke(group_binder, idx, m);
		}
	}

	[[nodiscard]] VkDescriptorSetLayout descriptor_set_layout() const noexcept{
		return user_descriptor_layout_;
	}
};

export
struct batch_command_slots{
private:
	std::array<vk::command_buffer, working_group_count> groups{};


public:
	[[nodiscard]] batch_command_slots() = default;

	[[nodiscard]] explicit batch_command_slots(
		const vk::command_pool& pool
	){
		for(auto&& group : groups){
			group = pool.obtain();
		}
	}

	VkCommandBuffer operator[](std::uint32_t idx) const noexcept{
		return groups[idx];
	}

	auto begin() const noexcept{
		return groups.begin();
	}

	auto end() const noexcept{
		return groups.end();
	}
};

export
struct batch_descriptor_slots{
private:

	vk::descriptor_layout user_descriptor_layout_{};
	vk::descriptor_buffer user_descriptor_buffer_{};

public:
	[[nodiscard]] batch_descriptor_slots() = default;

	[[nodiscard]] batch_descriptor_slots(
		vk::context& ctx,
		std::regular_invocable<vk::descriptor_layout_builder&> auto desc_layout_builder
	)
		:
		user_descriptor_layout_(ctx.get_device(), VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
								desc_layout_builder),
		user_descriptor_buffer_{
			ctx.get_allocator(), user_descriptor_layout_, user_descriptor_layout_.binding_count(),
			static_cast<std::uint32_t>(working_group_count)
		}{
	}

	const vk::descriptor_buffer& dbo() const noexcept{
		return user_descriptor_buffer_;
	}

	void bind(std::invocable<std::uint32_t, const vk::descriptor_mapper&> auto group_binder){
		vk::descriptor_mapper m{user_descriptor_buffer_};

		for(std::uint32_t i = 0; i < working_group_count; ++i){
			std::invoke(group_binder, i, m);
		}
	}

	vk::descriptor_mapper get_mapper() noexcept{
		return vk::descriptor_mapper{user_descriptor_buffer_};
	}

	[[nodiscard]] VkDescriptorSetLayout descriptor_set_layout() const noexcept{
		return user_descriptor_layout_;
	}
};

export
struct batch;

struct working_group{
	friend batch;
private:
	image_view_history image_view_history{};

	std::byte* span_begin;
	std::byte* span_end;

	std::uint64_t current_signal_index{};
	vk::semaphore mesh_semaphore{};
	vk::semaphore frag_semaphore{};


	void set_range(std::byte* instr_span_begin, std::byte* instr_span_end) noexcept{
		span_begin = instr_span_begin;
		span_end = instr_span_end;
	}
};

export
struct batch_descriptor_buffer_binding_info{
	VkDescriptorBufferBindingInfoEXT info;
	VkDeviceSize chunk_size;
};

struct batch{
	static constexpr std::uint32_t batch_work_group_count = working_group_count;

private:
	vk::context* context_{};
	vk::descriptor_layout descriptor_layout_{};
	vk::descriptor_buffer descriptor_buffer_{};

	dspt_info_buffer temp_dispatch_info_{};
	std::array<working_group, working_group_count> groups_{};
	std::uint32_t current_idle_group_index_{};
	std::uint32_t current_dspt_group_index_ = groups_.size();

	ubo_index_table ubo_table_;
	std::vector<std::byte> ubo_cache_{};
	gch::small_vector<std::size_t, (working_group_count + 1) * 6> ubo_timeline_mark_{};
	gch::small_vector<ubo_index_table::entry> ubo_update_data_cache_;

	// std::size_t current_ubo_index_{};


	instruction::instruction_buffer instruction_buffer_{};
	std::byte* instruction_idle_ptr_{};
	std::byte* instruction_pend_ptr_{};
	std::byte* instruction_dspt_ptr_{};
	std::uint32_t last_primit_offset_{};
	std::uint32_t last_vertex_offset_{};
	std::uint32_t last_shared_instr_size_{};

	vk::uniform_buffer dispatch_info_buffer{};

	//TODO merge these two?
	vk::buffer_cpu_to_gpu indirect_buffer{};
	vk::buffer_cpu_to_gpu instruction_gpu_buffer{};

	VkSampler sampler_{};

public:
	struct ubo_data_entries{
		const std::byte* base_address;
		std::span<const ubo_index_table::entry> entries;

		explicit operator bool() noexcept{
			return !entries.empty();
		}

	};

private:
	/**
	 *
	 * @brief
	 * (batch group index, ubo data update) -> Buffer to execute
	 */
	std::move_only_function<VkCommandBuffer(std::uint32_t, ubo_data_entries)> on_submit{};

public:

	[[nodiscard]] batch() = default;

	[[nodiscard]] batch(
		vk::context& context,
		VkSampler sampler,
		ubo_index_table ubo_index_table
	)
		:
		context_(&context),
		descriptor_layout_{
			context.get_device(), VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
			[](vk::descriptor_layout_builder& builder){
				builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT); // vertices info
				builder.push_seq(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT); // task barriers
				builder.push_seq(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT,
				                 maximum_images);
			}
		}
		, descriptor_buffer_(context.get_allocator(), descriptor_layout_, descriptor_layout_.binding_count(),
		                     working_group_count)
		, ubo_table_(std::move(ubo_index_table))
		, ubo_cache_(ubo_table_.max_size())
		, ubo_timeline_mark_(ubo_table_.count() * (1 + groups_.size()))
		, instruction_buffer_(1U << 13U)
		, dispatch_info_buffer(
			context.get_allocator(),
			sizeof(dspt_info_buffer) * working_group_count,
				VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
		, indirect_buffer(vk::buffer_cpu_to_gpu(
			context.get_allocator(),
			sizeof(VkDrawMeshTasksIndirectCommandEXT) *
			working_group_count,
			VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT))
		, instruction_gpu_buffer(vk::buffer_cpu_to_gpu(
			context.get_allocator(),
			instruction_buffer_.size(),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT))
		, sampler_(sampler)
	{
		instruction_idle_ptr_ = instruction_buffer_.begin();
		instruction_dspt_ptr_ = instruction_buffer_.begin();
		instruction_pend_ptr_ = instruction_buffer_.begin();

		vk::descriptor_mapper mapper{descriptor_buffer_};
		for(std::uint32_t i = 0; i < working_group_count; ++i){
			(void)mapper.set_uniform_buffer(1, dispatch_info_buffer.get_address() + sizeof(dspt_info_buffer) * i,
			                                sizeof(dspt_info_buffer), i);
		}

		for(auto&& group : groups_){
			group.mesh_semaphore = vk::semaphore{context_->get_device(), 0};
			group.frag_semaphore = vk::semaphore{context_->get_device(), 0};
		}
	}

	void set_submit_callback(std::invocable<std::uint32_t, ubo_data_entries> auto&& fn){
		on_submit = std::forward<decltype(fn)>(fn);
	}

private:
	template <typename T, typename Fn, typename... Args>
	FORCE_INLINE void push_impl(Fn psh_fn, const Args&... args){
		// assert(draw::get_instr_placement_size_requirement<T>() <= instruction_buffer_.size());

		std::byte* rst{nullptr};
		while(this->check_need_block<false, T, Args...>(instruction_idle_ptr_, args...)){
			if(is_all_idle()) consume_n(working_group_count / 2);
			wait_one(false);
		}

		rst = psh_fn(instruction_idle_ptr_);

		if(rst == nullptr){
			if(is_all_idle()) consume_n(working_group_count / 2);
			//Reverse buffer to head
			while(this->check_need_block<true, T, Args...>(instruction_buffer_.begin(), args...)){
				if(is_all_idle()) consume_n(working_group_count / 2);
				wait_one(false);
			}

			rst = psh_fn(instruction_buffer_.begin());
		}

		assert(instruction_idle_ptr_ != nullptr);
		instruction_idle_ptr_ = rst;

		if(is_all_idle()){
			const auto dst = instruction_idle_ptr_ - instruction_dspt_ptr_;
			if(dst > instruction_buffer_.size() / 2 || (dst < 0 && dst > -instruction_buffer_.size() / 2)){
				consume_n(working_group_count / 2);
			}
		}
	}

public:
	template <instruction::known_instruction T, typename... Args>
		requires (instruction::valid_consequent_argument<T, Args...>)
	void push_instruction(const T& instr, const Args&... args){
		this->push_impl<T>([&, this](std::byte* where) noexcept FORCE_INLINE {
			return instruction::place_instruction_at(where, instruction_buffer_.end(), instr, args...);
		}, args...);
	}

	template <typename T>
		requires (!instruction::known_instruction<T>)
	void update_ubo(const T& instr){
		this->push_impl<T>([&, this](std::byte* where) noexcept FORCE_INLINE {
			return instruction::place_ubo_update_at(where, instruction_buffer_.end(), instr, ubo_table_.index_of<T>());
		});
	}

	[[nodiscard]] bool is_all_done() const noexcept{
		return
			instruction_idle_ptr_ == instruction_dspt_ptr_ &&
			instruction_idle_ptr_ == instruction_pend_ptr_ &&
			is_all_idle();
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
		const auto initial_idle = groups_.size() - intital_wait;
		bool unfinished = true;

		auto need_wait = [&](const unsigned has_dispatched_count) -> bool{
			return has_dispatched_count >= initial_idle;
		};

		while(true){
			auto& group = groups_[current_idle_group_index_];
			const auto begin = instruction_pend_ptr_;

			const auto sentinel = instruction_idle_ptr_ >= instruction_pend_ptr_
				                      ? instruction_idle_ptr_
				                      : instruction_buffer_.end();
			const auto dspcinfo = get_dispatch_info(instruction_pend_ptr_, sentinel,
			                                                     temp_dispatch_info_.group_info,
			                                                     group.image_view_history, last_primit_offset_,
			                                                     last_vertex_offset_);
			instruction_pend_ptr_ = dspcinfo.next;


			const auto& next_head = get_instr_head(dspcinfo.next);
			const auto next_instr_type = next_head.type;

			last_primit_offset_ = dspcinfo.next_primit_offset;
			last_vertex_offset_ = dspcinfo.next_vertex_offset;
			if(dspcinfo.img_requires_update){
				//TODO better image update strategy ?
				if(!dspcinfo.count){
					group.image_view_history.clear();
					continue;
				}
			}

			auto update_ubo = [&](std::span<const std::byte> data){
				const auto idx = next_head.payload.ubo.local_index;
				const auto& entry = ubo_table_[idx];
				assert(entry.size == data.size_bytes());
				std::memcpy(ubo_cache_.data() + entry.global_offset, data.data(), data.size_bytes());
				++ubo_timeline_mark_[idx];
			};


			if(dspcinfo.count){
				assert(dspcinfo.next >= begin);

				const auto instr_byte_off = begin - instruction_buffer_.begin();
				const auto instr_shared_size = (dspcinfo.contains_next()
					                                ? next_head.get_instr_byte_size()
					                                : 0);
				const auto instr_size = dspcinfo.next - begin + instr_shared_size;

				const bool is_img_history_changed = group.image_view_history.check_changed();

				if(need_wait(actuals)){
					//If requires wait and is not undispatched command
					wait_one(is_img_history_changed);
				}

				(void)vk::buffer_mapper{instruction_gpu_buffer}
					.load_range(
						std::span{begin + last_shared_instr_size_, dspcinfo.next + instr_shared_size},
						instr_byte_off + last_shared_instr_size_);

				//Resolved the condition when two drawcall share one instruction, so override the first instr img index is needed
				if(last_shared_instr_size_){
					auto& generic = reinterpret_cast<primitive_generic&>(*(begin + sizeof(instruction_head)));
					assert(generic.image.index < image_view_history::max_cache_count || generic.image.index == ~0U);
					temp_dispatch_info_.shared_instr_image_index_override = generic.image.index;
				} else{
					temp_dispatch_info_.shared_instr_image_index_override = image_view_history::max_cache_count;
				}
				last_shared_instr_size_ = instr_shared_size;

				if(instr_shared_size){
					//Resume next image from index to pointer to view
					auto& generic = reinterpret_cast<primitive_generic&>(*(dspcinfo.next + sizeof(instruction_head)));

					generic.image.set_view(dspcinfo.current_img);
				}

				if(is_img_history_changed){
					vk::descriptor_mapper{descriptor_buffer_}
						.set_image(2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler_,
						           group.image_view_history.get(), current_idle_group_index_);
				}

				(void)vk::buffer_mapper{dispatch_info_buffer}
					.load(static_cast<const void*>(&temp_dispatch_info_),
					      16 + sizeof(dispatch_group_info) * dspcinfo.count,
					      sizeof(dspt_info_buffer) * current_idle_group_index_);

				(void)vk::descriptor_mapper{descriptor_buffer_}
					.set_storage_buffer(0,
					                    instruction_gpu_buffer.get_address() + instr_byte_off,
					                    instr_size,
					                    current_idle_group_index_);

				VkDrawMeshTasksIndirectCommandEXT indirectCommandExt{dspcinfo.count, 1, 1};
				(void)vk::buffer_mapper{indirect_buffer}
					.load(indirectCommandExt, sizeof(VkDrawMeshTasksIndirectCommandEXT) * current_idle_group_index_);


				const auto ubo_total_count = ubo_table_.count();
				std::size_t ubo_count = 0;
				ubo_update_data_cache_.clear();
				for(unsigned i = 0; i < ubo_total_count; ++i){
					if(ubo_timeline_mark_[i + (current_idle_group_index_ + 1) * ubo_total_count] < ubo_timeline_mark_[i]){
						ubo_timeline_mark_[i + (current_idle_group_index_ + 1) * ubo_total_count] = ubo_timeline_mark_[i];
						ubo_update_data_cache_.push_back(ubo_table_[i]);
						++ubo_count;
					}
				}

				//usr update
				assert(on_submit);
				const auto cmdbuf =
					on_submit(current_idle_group_index_, ubo_data_entries{ubo_cache_.data(), std::span{ubo_update_data_cache_.data(), ubo_count}});


				if(dspcinfo.ubo_requires_update){
					const auto data = get_ubo_data_span(dspcinfo.next);
					update_ubo(data);
					instruction_pend_ptr_ += data.size_bytes() + sizeof(instruction_head);
				}

				group.set_range(begin, instruction_pend_ptr_);
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
						.stageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
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
			}else{
				if(dspcinfo.ubo_requires_update){
					const auto data = get_ubo_data_span(dspcinfo.next);
					update_ubo(data);
					instruction_pend_ptr_ += data.size_bytes() + sizeof(instruction_head);
					if(is_all_idle()){
						instruction_dspt_ptr_ = instruction_pend_ptr_;
					}else{
						auto idx = get_back_group() - groups_.data();
						get_back_group()->span_end = instruction_pend_ptr_;
					}
				}
			}

			bool hasNext = !dspcinfo.no_next(begin);

			if(next_instr_type == instr_type::noop && instruction_pend_ptr_ > instruction_idle_ptr_){
				assert(last_shared_instr_size_ == 0);
				instruction_pend_ptr_ = instruction_buffer_.begin();
				hasNext = true;
			}

			if(actuals == count || !dspcinfo.count){
				if(hasNext) goto RET;
				break;
			}
		}

		unfinished = false;

	RET:
		if(actuals) vkQueueSubmit2(context_->graphic_queue(), actuals, submitInfo2.data(), nullptr);
		return unfinished;
	}

	void wait_one(bool wait_on_frag = true){
		if(const auto group = get_front_group()){
			if(wait_on_frag){
				group->frag_semaphore.wait(group->current_signal_index);
			} else{
				group->mesh_semaphore.wait(group->current_signal_index);
			}

			instruction_dspt_ptr_ = group->span_end;
			pop_front_group();
		}
	}

	void wait_n(const std::uint32_t count, const bool wait_on_frag = true){
		if(!count) return;

		std::array<std::uint64_t, working_group_count> values{};
		std::array<VkSemaphore, working_group_count> semaphores{};
		std::uint32_t pushed{};
		for_each_submit([&](unsigned, const working_group& group){
			if(pushed >= count) return;
			semaphores[pushed] = wait_on_frag ? group.frag_semaphore : group.mesh_semaphore;
			values[pushed] = group.current_signal_index;
			++pushed;
			instruction_dspt_ptr_ = group.span_end;
		});

		pop_n_group(pushed);

		vk::wait_multiple(context_->get_device(), {semaphores.data(), pushed}, {values.data(), pushed});
	}

	void wait_all(bool wait_on_frag = true){
		wait_n(get_pushed_group_count(), wait_on_frag);
	}

	void record_command(const VkPipelineLayout layout, std::span<const batch_descriptor_buffer_binding_info> bindingInfoExts, std::generator<VkCommandBuffer&&>&& cmdGen) const{
		gch::small_vector<VkDescriptorBufferBindingInfoEXT> infos{descriptor_buffer_};
		infos.reserve(bindingInfoExts.size() + 1);

		auto view1 = bindingInfoExts | std::views::transform(&batch_descriptor_buffer_binding_info::info);
		infos.append(view1.begin(), view1.end());

		gch::small_vector<VkDeviceSize> offsets(infos.size());

		for(auto&& [idx, buf] : std::move(cmdGen) | std::views::take(groups_.size()) | std::ranges::views::enumerate){
			descriptor_buffer_.bind_chunk_to(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, idx);

			vk::cmd::bind_descriptors(
				buf, VK_PIPELINE_BIND_POINT_GRAPHICS, layout,
				0,
				std::span{infos.data(), infos.size()}, std::span{offsets.data(), offsets.size()});
			vk::cmd::drawMeshTasksIndirect(buf, indirect_buffer, idx * sizeof(VkDrawMeshTasksIndirectCommandEXT));

			offsets[0] += descriptor_buffer_.get_chunk_size();
			for(std::size_t i = 0; i < bindingInfoExts.size(); ++i){
				offsets[i + 1] += bindingInfoExts[i].chunk_size;
			}
		}
	}

	[[nodiscard]] VkDescriptorSetLayout get_batch_descriptor_layout() const noexcept{
		return descriptor_layout_;
	}

	void reset() noexcept {
		assert(is_all_done());
		assert(is_all_idle());

		// current_idle_group_index_ = 0;
		instruction_idle_ptr_ = instruction_buffer_.begin();
		instruction_pend_ptr_ = instruction_buffer_.begin();
		instruction_dspt_ptr_ = instruction_buffer_.begin();
		instruction_buffer_.clear();
		temp_dispatch_info_ = {};
	}

	std::size_t work_group_count() const noexcept{
		return groups_.size();
	}

	vk::context& context() const noexcept{
		return *context_;
	}

	void append_ubo_table(const ubo_index_table& table){
		ubo_table_.append(table);
		ubo_cache_.reserve(ubo_table_.max_size());
		ubo_timeline_mark_.resize(ubo_table_.count() * (1 + groups_.size()));
	}

private:
	template <std::invocable<unsigned, working_group&> Fn>
	FORCE_INLINE auto for_each_submit(Fn fn){
		if(is_all_idle()) return;
		auto cur = current_dspt_group_index_;

		do{
			if constexpr(std::predicate<Fn, working_group&>){
				if(std::invoke(fn, cur, groups_[cur])){
					return cur;
				}
			} else{
				std::invoke(fn, cur, groups_[cur]);
			}

			cur = (cur + 1) % groups_.size();
		} while(cur != current_idle_group_index_);
	}

	[[nodiscard]] unsigned get_pushed_group_count() const noexcept{
		if(is_all_idle()) return 0;
		return current_dspt_group_index_ < current_idle_group_index_
			       ? current_idle_group_index_ - current_dspt_group_index_
			       : groups_.size() - (current_dspt_group_index_ - current_idle_group_index_);
	}

	[[nodiscard]] working_group* get_front_group() noexcept{
		return is_all_idle() ? nullptr : groups_.data() + current_dspt_group_index_;
	}

	[[nodiscard]] working_group* get_back_group() noexcept{
		return is_all_idle() ? nullptr : groups_.data() + ((current_idle_group_index_  + groups_.size() - 1) % groups_.size());
	}

	template <bool onReverse, typename InstrT, typename... Args>
	FORCE_INLINE bool check_need_block(const std::byte* where, const Args& ...args) const noexcept{
		if(instruction_dspt_ptr_ == where) return onReverse || !is_all_idle();

		if(instruction_dspt_ptr_ > where){
			const auto end = where + instruction::get_instr_size<InstrT, Args...>(args...) + sizeof(instruction_head);
			return end > instruction_dspt_ptr_;
		} else{
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

// template <typename HeadInstr, typename ...Args>
// struct draw_clause{
// 	batch& batch;
// 	HeadInstr head_instr;
// };
//
// template <typename HeadInstr, typename ...Args>
// 	requires valid_consequent_argument<HeadInstr, Args...>
// batch& operator|(const draw_clause<HeadInstr>& head, const Args&... args){
// 	head.batch.draw(head.head_instr, args...);
// 	return head.batch;
// }
//
// export
// template <known_instruction T>
// FORCE_INLINE auto operator<<(batch& batch, const T& instr){
// 	return draw_clause{batch, instr};
// }
//
// export
// template <typename T>
// 	requires (!known_instruction<T>)
// FORCE_INLINE auto operator<<(batch& batch, const T& instr){
// 	batch.update_ubo(instr);
// 	return batch;
// }
//
// void foo(){
// 	batch batch;
//
// 	batch << line_segments{} | line_node{};
// }
//
//
// struct batch_stream{
// 	batch* batch;
// 	primitive_generic generic;
//
//
// };

}
