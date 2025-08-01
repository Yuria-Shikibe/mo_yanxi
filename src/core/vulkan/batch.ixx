module;

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <cassert>
#include <immintrin.h>
#include <gch/small_vector.hpp>
#include "../src/ext/adapted_attributes.hpp"

export module mo_yanxi.vk.batch;

export import mo_yanxi.vk.resources;
export import mo_yanxi.vk.util.cmd.resources;
export import mo_yanxi.vk.descriptor_buffer;
export import mo_yanxi.vk.pipeline.layout;
export import mo_yanxi.vk.context;
export import mo_yanxi.vk.sync;
import mo_yanxi.circular_array;
import mo_yanxi.circular_queue;
import std;

namespace mo_yanxi::vk{
	enum class region_state{
		valid,
		frozen,
		// submitted,
	};

	struct aligned_data{
	private:
		std::size_t align;
		exclusive_handle_member<std::byte*> data;

	public:
		[[nodiscard]] aligned_data() = default;

		~aligned_data(){
			if(data)::operator delete[](data, std::align_val_t{std::bit_ceil(align)});
		}

		[[nodiscard]] explicit aligned_data(const std::size_t size, const std::size_t align)
			: align(align){
			data = static_cast<std::byte*>(::operator new[](size, std::align_val_t{std::bit_ceil(align)}));
		}

		[[nodiscard]] std::byte* get() const noexcept{
			return data.handle;
		}

		aligned_data(const aligned_data& other) = delete;
		aligned_data(aligned_data&& other) noexcept = default;
		aligned_data& operator=(const aligned_data& other) = delete;
		aligned_data& operator=(aligned_data&& other) noexcept = default;
	};

	export
	struct barrier_append{
		struct promise_type;
		using handle = std::coroutine_handle<promise_type>;
		using value_type = cmd::dependency_gen;

		[[nodiscard]] barrier_append() = default;

		[[nodiscard]] explicit barrier_append(handle&& hdl)
			: hdl{std::move(hdl)}{}

		struct promise_type{
			[[nodiscard]] promise_type() = default;

			barrier_append get_return_object(){
				return barrier_append{handle::from_promise(*this)};
			}

			[[nodiscard]] static auto initial_suspend() noexcept{ return std::suspend_never{}; }

			[[nodiscard]] static auto final_suspend() noexcept{ return std::suspend_always{}; }

			static void return_void() noexcept{

			}

			auto yield_value(value_type& val) noexcept{
				toSubmit = &val;
				return std::suspend_always{};
			}

			[[noreturn]] static void unhandled_exception() noexcept{
				std::terminate();
			}

		private:
			friend barrier_append;

			value_type* toSubmit;
		};

		void set_event_dependencies() const{
			hdl->resume();
		}

		[[nodiscard]] bool done() const noexcept{
			return hdl->done();
		}

		[[nodiscard]] value_type& dependency_info() const noexcept{
			assert(hdl->promise().toSubmit);
			return *hdl->promise().toSubmit;
		}

		~barrier_append(){
			if(hdl){
				assert(done());
				hdl->destroy();
			}
		}

		barrier_append(const barrier_append& other) = delete;
		barrier_append(barrier_append&& other) noexcept = default;
		barrier_append& operator=(const barrier_append& other) = delete;
		barrier_append& operator=(barrier_append&& other) noexcept = default;

	private:
		exclusive_handle_member<handle> hdl{};
	};
	
	export
	struct batch{
		static constexpr std::size_t maximum_images = 4;

	private:
		context* context{};

		VkBuffer index_buffer_handle{};
		VkSampler sampler{};

		std::size_t vertex_unit_size{};
		std::size_t vertex_chunk_size{};
		std::size_t vertex_chunk_count{};

		buffer vertex_buffer{};
		buffer indirect_buffer{};
		std::byte* mapped_data{};

		descriptor_layout texture_descriptor_layout{};
		descriptor_buffer descriptor_buffer_{};

		std::move_only_function<VkCommandBuffer(std::size_t) const> external_consumer{};

		// bool unordered{};

	public:
		struct batch_vertex_allocation{
			std::byte* data;
			std::uint32_t size_in_bytes;
			std::uint32_t view_index;

			[[nodiscard]] std::uint32_t get_vertex_group_count(std::uint32_t unit_size) const noexcept{
				return size_in_bytes / 4 / unit_size;
			}

			constexpr explicit operator bool() const noexcept{
				return data != nullptr;
			}
		};

		struct allocation_group{
			batch_vertex_allocation pre;
			batch_vertex_allocation post;

		};


		struct image_view_history{

		private:
			VkImageView latest{};
			std::uint32_t latest_index{};
			alignas(32) std::array<VkImageView, maximum_images> images{};
			std::uint32_t count{};

			static_assert(maximum_images % 4 == 0);

		public:
			FORCE_INLINE void clear(this image_view_history& self) noexcept{
				self = {};
			}

			[[nodiscard]] FORCE_INLINE std::span<const VkImageView> get() const noexcept{
				return {images.data(), count};
			}

			[[nodiscard]] FORCE_INLINE /*constexpr*/ std::uint32_t try_push(VkImageView image) noexcept {
				if(!image)return std::numeric_limits<std::uint8_t>::max();
				if(image == latest)return latest_index;

				// for(std::size_t i = 0; i < maximum_images; ++i){
				// 	auto& cur = images[i];
				// 	if(image == cur){
				// 		latest = image;
				// 		latest_index = i;
				// 		return i;
				// 	}
				//
				// 	if(cur == nullptr){
				// 		latest = cur = image;
				// 		latest_index = i;
				// 		count = i + 1;
				// 		return i;
				// 	}
				// }

				// 将目标指针广播到整个AVX寄存器
				const __m256i target = _mm256_set1_epi64x(std::bit_cast<int64_t>(image));
				const __m256i zero = _mm256_setzero_si256();

				for (std::uint32_t i = 0; i != maximum_images; i += 4) {
					// 对齐加载4个指针
					const auto vec = _mm256_load_si256(reinterpret_cast<const __m256i*>(&images[i]));

					// 检查是否与目标指针相等
					const auto eq_mask = _mm256_cmpeq_epi64(vec, target);
					const auto eq_bits = _mm256_movemask_epi8(eq_mask);

					// 高效生成元素掩码（无分支）
					const std::uint32_t eq_element_mask =
						((eq_bits & 0x000000FF) == 0x000000FF) |
						(((eq_bits & 0x0000FF00) == 0x0000FF00) << 1) |
						(((eq_bits & 0x00FF0000) == 0x00FF0000) << 2) |
						(((eq_bits & 0xFF000000) == 0xFF000000) << 3);

					if (eq_element_mask) {
						// 使用 C++20 位操作找到第一个匹配位置
						const auto j = std::countr_zero(eq_element_mask);
						latest = image;
						latest_index = i + j;
						return i + j;
					}

					// 检查空槽（nullptr）
					const auto null_mask = _mm256_cmpeq_epi64(vec, zero);
					const auto null_bits = _mm256_movemask_epi8(null_mask);

					// 高效生成空槽掩码（无分支）
					const std::uint32_t null_element_mask =
						((null_bits & 0x000000FF) == 0x000000FF) |
						(((null_bits & 0x0000FF00) == 0x0000FF00) << 1) |
						(((null_bits & 0x00FF0000) == 0x00FF0000) << 2) |
						(((null_bits & 0xFF000000) == 0xFF000000) << 3);

					if (null_element_mask) {
						// 使用 C++20 位操作找到第一个空槽位置
						const auto j = std::countr_zero(null_element_mask);
						images[i + j] = image;
						latest = image;
						latest_index = i + j;
						count = i + j + 1;
						return i + j;
					}
				}

				return maximum_images;
			}
		};

		struct draw_region{
			std::array<VkImageView, maximum_images> last_images{};
			std::uint32_t last_image_count{};

			image_view_history views{};
			std::uint32_t vertex_group_count{};
			std::size_t identity{}; //TODO unordered draw

			aligned_data host_vertices{}; //TODO batch global buffer

			semaphore device_vertex_semaphore{};
			semaphore device_descriptor_semaphore{};

			std::uint64_t device_vertex_semaphore_signal{};
			std::uint64_t device_descriptor_semaphore_signal{};

			region_state state{};

			[[nodiscard]] draw_region() = default;

			[[nodiscard]] draw_region(const vk::context& context, std::size_t chunk_size, std::size_t unit_size)
				:
			device_vertex_semaphore{context.get_device(), 0},
			device_descriptor_semaphore{context.get_device(), 0}
			{
				host_vertices = aligned_data{chunk_size, unit_size * 4};
			}

			FORCE_INLINE void advance_and_reset() noexcept{
				auto span = views.get();
				last_image_count = span.size();
				std::ranges::copy(span, last_images.data());
				views.clear();
				vertex_group_count = 0;
				identity = 0;
				state = region_state::valid;
			}

			[[nodiscard]] FORCE_INLINE bool is_image_compatitble() const noexcept{
				auto span = views.get();
				if(last_image_count < span.size())return false;

				for(std::size_t i = 0; i < span.size(); ++i){
					if(span[i] != last_images[i]){
						return false;
					}
				}

				return true;
			}

			[[nodiscard]] FORCE_INLINE constexpr bool saturate(const std::size_t group_capacity) const noexcept{
				return vertex_group_count == group_capacity;
			}

			[[nodiscard]] FORCE_INLINE /*constexpr*/ batch_vertex_allocation acquire(
				VkImageView image_view,
				const std::uint32_t acquired_group_count,
				const std::size_t unit_size,
				const std::size_t chunk_vertex_group_capacity
			) noexcept{
				assert(state == region_state::valid);

				const auto viewIdx = views.try_push(image_view);
				if(viewIdx == maximum_images)return {};

				auto next = this->vertex_group_count + acquired_group_count;
				std::uint32_t realCount = acquired_group_count;

				if(next > chunk_vertex_group_capacity){
					realCount -= next - chunk_vertex_group_capacity;
					next = chunk_vertex_group_capacity;
				}

				const batch_vertex_allocation rst = {
					host_vertices.get() + vertex_group_count * unit_size * 4,
					static_cast<std::uint32_t>(realCount * unit_size * 4), viewIdx
				};
				vertex_group_count = next;
				return rst;
			}

			[[nodiscard]] FORCE_INLINE constexpr batch_vertex_allocation fast_acquire(
				VkImageView image_view,
				const std::uint32_t acquired_group_count,

				const std::size_t unit_size,
				const std::size_t chunk_vertex_group_capacity
			) noexcept{
				assert(state == region_state::valid);

				if(acquired_group_count + this->vertex_group_count <= chunk_vertex_group_capacity){
					const auto viewIdx = views.try_push(image_view);
					if(viewIdx == maximum_images)return {};

					const batch_vertex_allocation rst = {
						host_vertices.get() + vertex_group_count * unit_size * 4,
						static_cast<std::uint32_t>(acquired_group_count * unit_size * 4),
						viewIdx
					};

					vertex_group_count += acquired_group_count;

					return rst;
				}

				return {};
			}
		};

		struct draw_call{
			std::size_t chunk_index{};
		};

	private:
		static constexpr std::size_t static_length = 6;
		std::size_t current_index{};
		gch::small_vector<draw_region, static_length> fan{};
		gch::small_vector<draw_call, static_length> pending_draw_calls{};

		FORCE_INLINE draw_region& get_current() noexcept{
			return fan[current_index];
		}

		FORCE_INLINE draw_region* get_another() noexcept{
			std::size_t idx = (current_index + 1) % vertex_chunk_count;
			while(idx != current_index){
				auto& region = fan[idx];
				if(region.state == region_state::valid){
					return &region;
				}
				idx = (idx + 1) % vertex_chunk_count;
			}
			return nullptr;
		}

		FORCE_INLINE void advance_chunk() noexcept{
			current_index = (current_index + 1) % vertex_chunk_count;
		}

		FORCE_INLINE void push_to_staging(const std::size_t index){
			fan[index].state = region_state::frozen;

			pending_draw_calls.push_back({
				.chunk_index = index
			});
		}

	public:
		void bind_resources(
			VkCommandBuffer command_buffer,
			VkPipelineLayout pipeline_layout,
			const std::size_t chunk_index,
			bool bind_descriptors = false,
			const std::uint32_t set_index = 1
			){
			VkDeviceSize offset = chunk_index * vertex_chunk_size;

			vkCmdBindVertexBuffers(
				command_buffer, 0, 1, vertex_buffer.as_data(), &offset);

			vkCmdBindIndexBuffer(
				command_buffer, index_buffer_handle, 0,
				VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexedIndirect(
				command_buffer, indirect_buffer, sizeof(VkDrawIndexedIndirectCommand) * chunk_index, 1, sizeof(VkDrawIndexedIndirectCommand));

			if(bind_descriptors){
				descriptor_buffer_.bind_chunk_to(
					command_buffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipeline_layout,
					set_index,
					chunk_index
				);
			}
		}

		[[nodiscard]] batch() = default;

		[[nodiscard]] batch(
			vk::context& context,
			VkBuffer indexBuffer,
			VkSampler sampler,

			const std::size_t chunk_count,
			const std::size_t vertex_unit_size,
			const std::size_t vertex_group_count) :
			context(&context),
			index_buffer_handle(indexBuffer),
			sampler(sampler),
			vertex_unit_size(vertex_unit_size),
			vertex_chunk_size(vertex_group_count * 4 * vertex_unit_size),
			vertex_chunk_count(chunk_count),

			vertex_buffer(context.get_allocator(), {
				              .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				              .size = get_total_size(),
				              .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
			              }, VmaAllocationCreateInfo{
				              .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
				              .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				              .preferredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT
			              }),
			indirect_buffer(
				context.get_allocator(), {
					.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
					.size = sizeof(VkDrawIndexedIndirectCommand) * chunk_count,
					.usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT
				}, {.usage = VMA_MEMORY_USAGE_CPU_TO_GPU}),

			texture_descriptor_layout(
				context.get_device(),
				VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
				[](descriptor_layout_builder& builder){
					builder.push_seq(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, maximum_images);
				}),
			descriptor_buffer_(context.get_allocator(), texture_descriptor_layout,
			                  texture_descriptor_layout.binding_count(), chunk_count){

			VmaAllocationInfo info;
			vmaGetAllocationInfo(context.get_allocator(), vertex_buffer.get_allocation(), &info);
			mapped_data = static_cast<std::byte*>(info.pMappedData);
			fan.reserve(chunk_count);
			for(std::size_t i = 0; i < chunk_count; ++i){
				fan.push_back({});
			}
			pending_draw_calls.reserve(chunk_count);
			for (auto && [idx, draw_region] : fan | std::views::enumerate){
				draw_region = batch::draw_region{context, vertex_chunk_size, vertex_unit_size};
			}
		}

		[[nodiscard]] constexpr std::size_t get_total_size() const noexcept{
			return vertex_chunk_count * vertex_chunk_size;
		}

		[[nodiscard]] constexpr std::size_t get_chunk_count() const noexcept{
			return vertex_chunk_count;
		}

		[[nodiscard]] constexpr std::size_t get_vertex_group_count_per_chunk() const noexcept{
			return vertex_chunk_size / vertex_unit_size / 4;
		}

		[[nodiscard]] vk::context* get_context() const noexcept{
			return context;
		}

		void set_consumer(decltype(external_consumer)&& cons) noexcept{
			external_consumer = std::move(cons);
		}

		[[nodiscard]] const descriptor_buffer& descriptor_buffer() const noexcept{
			return descriptor_buffer_;
		}

		[[nodiscard]] const descriptor_layout& descriptor_layout() const noexcept{
			return texture_descriptor_layout;
		}

		[[nodiscard]] allocation_group acquire(std::uint32_t vertex_group_count, VkImageView image_view){
			//TODO order irrelevant acquire

			auto& region = get_current();

			if(region.state == region_state::frozen){
				consume_all();
			}else{
				if(pending_draw_calls.size() > fan.size() / 2)consume_pending();
			}

			auto pre_rst = region.acquire(image_view, vertex_group_count, vertex_unit_size, get_vertex_group_count_per_chunk());

			if(pre_rst.size_in_bytes == 0){
				//Current run out of valid image regions
				pend_current();
				// advance_chunk();
				pre_rst = get_current().acquire(image_view, vertex_group_count, vertex_unit_size, get_vertex_group_count_per_chunk());
			}

			if(pre_rst.get_vertex_group_count(vertex_unit_size) == vertex_group_count){
				//Current is fit
				if(get_current().saturate(get_vertex_group_count_per_chunk())){
					pend_current();
				}

				return {pre_rst};
			}else{
				//Current fit partial the acquire
				vertex_group_count -= pre_rst.get_vertex_group_count(vertex_unit_size);

				pend_current();
				// advance_chunk();

				const auto post_rst = get_current().acquire(image_view, vertex_group_count, vertex_unit_size, get_vertex_group_count_per_chunk());

				if(post_rst.get_vertex_group_count(vertex_unit_size) != vertex_group_count){
					std::println(std::cerr, "[Graphic] Too much vertex in one acquisition");
				}

				return {pre_rst, post_rst};
			}
		}

		FORCE_INLINE void pend_current(){
			push_to_staging(current_index);
			advance_chunk();
		}

		void consume_pending(){
			if(pending_draw_calls.empty())return;

			static constexpr std::size_t submit_buffer_size{static_length};

			std::array<VkSubmitInfo2, submit_buffer_size> submit_infos{};
			std::array<VkSemaphoreSubmitInfo, submit_buffer_size * 2> semaphore_submit_infos{};
			std::array<VkCommandBufferSubmitInfo, submit_buffer_size> command_buffer_submit_infos{};
			std::size_t submit_count{};

			if(pending_draw_calls.size() > submit_buffer_size){
				throw std::runtime_error("[Graphic] Too much draw calls");
			}

			for(const auto draw_call : pending_draw_calls){
				auto& region = fan[draw_call.chunk_index];
				const auto vertex_to_wait = region.device_vertex_semaphore_signal++;
				const auto descriptor_to_wait = region.device_descriptor_semaphore_signal++;
				const auto size = region.vertex_group_count * 4 * vertex_unit_size;


				region.device_vertex_semaphore.wait(vertex_to_wait);


				buffer_mapper{indirect_buffer}.load(VkDrawIndexedIndirectCommand{
					.indexCount = region.vertex_group_count * 6,
					.instanceCount = 1,
					.firstIndex = 0,
					.vertexOffset = 0,
					.firstInstance = 0
				}, static_cast<std::ptrdiff_t>(sizeof(VkDrawIndexedIndirectCommand) * draw_call.chunk_index));


				std::memcpy(mapped_data + draw_call.chunk_index * vertex_chunk_size, region.host_vertices.get(), size);
				vertex_buffer.flush(size, draw_call.chunk_index * vertex_chunk_size);


				if(!region.is_image_compatitble()){
					region.device_descriptor_semaphore.wait(descriptor_to_wait);
					descriptor_mapper{descriptor_buffer_}.set_image(
						0,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						sampler,
						region.views.get(),
						draw_call.chunk_index
					);
				}


				{
					const auto cmd = external_consumer(draw_call.chunk_index);

					semaphore_submit_infos[submit_count * 2] = VkSemaphoreSubmitInfo{
						.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
						.semaphore = region.device_vertex_semaphore,
						.value = region.device_vertex_semaphore_signal,
						.stageMask = VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT
					};

					semaphore_submit_infos[submit_count * 2 + 1] = VkSemaphoreSubmitInfo{
						.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
							.semaphore = region.device_descriptor_semaphore,
							.value = region.device_descriptor_semaphore_signal,
							.stageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT
					};

					command_buffer_submit_infos[submit_count] = VkCommandBufferSubmitInfo{
						.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
						.commandBuffer = cmd,
					};

					submit_infos[submit_count++] = VkSubmitInfo2{
						.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
						.waitSemaphoreInfoCount = 0,
						.pWaitSemaphoreInfos = nullptr,
						.commandBufferInfoCount = 1,
						.pCommandBufferInfos = &command_buffer_submit_infos[submit_count],
						.signalSemaphoreInfoCount = 2,
						.pSignalSemaphoreInfos = &semaphore_submit_infos[submit_count * 2]
					};
				}
			}

			vkQueueSubmit2(context->graphic_queue(), submit_count, submit_infos.data(), nullptr);

			for(const auto draw_call : pending_draw_calls){
				auto& region = fan[draw_call.chunk_index];
				const auto size = region.vertex_group_count * 4 * vertex_unit_size;

				std::memset(region.host_vertices.get(), 0, size);
				region.advance_and_reset();
			}

			pending_draw_calls.clear();
		}

		FORCE_INLINE void consume_all(){
			while(true){
				const auto& cur = get_current();
				if(cur.state == region_state::valid && cur.vertex_group_count){
					push_to_staging(current_index);
					advance_chunk();
				}else{
					break;
				}
			}

			consume_pending();
		}
	};

	export
	template <typename T>
	struct batch_allocation_range : batch::allocation_group{
		using value_type = std::array<T, 4>;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference = value_type&;
		using const_reference = const value_type&;
		using pointer = value_type*;
		using const_pointer = const value_type*;

	private:
		struct sentinel_impl{
			difference_type index;
		};

		struct iterator_impl{
			using value_type = batch_allocation_range::value_type;
			using difference_type = std::ptrdiff_t;

			value_type* pre;
			size_type pre_size;

			value_type* post;
			size_type index;

			friend constexpr bool operator==(const iterator_impl& lhs, const iterator_impl& rhs) noexcept{
				return lhs.index == rhs.index;
			}

			friend constexpr bool operator==(const sentinel_impl& lhs, const iterator_impl& rhs) noexcept{
				return lhs.index == rhs.index;
			}

			friend constexpr bool operator==(const iterator_impl& lhs, const sentinel_impl& rhs) noexcept{
				return lhs.index == rhs.index;
			}

			auto constexpr operator<=>(const iterator_impl& rhs) const noexcept{
				return index <=> rhs.index;
			}

			auto constexpr operator<=>(const sentinel_impl& rhs) const noexcept{
				return index <=> static_cast<size_type>(rhs.index);
			}

			constexpr value_type* operator->() const noexcept{
				return index < pre_size ? pre + index : post + (index - pre_size);
			}

			constexpr value_type& operator*() const noexcept{
				return *std::to_address(*this);
			}

			constexpr iterator_impl& operator++() noexcept{
				++index;
				return *this;
			}

			constexpr iterator_impl operator++(int) noexcept{
				iterator_impl i{*this};
				this->operator++();
				return i;
			}

			constexpr iterator_impl operator+(const difference_type off) const noexcept{
				iterator_impl cpy{*this};
				cpy.index + off;
				return cpy;
			}

			constexpr iterator_impl& operator+=(const difference_type off) noexcept{
				index += off;
				return *this;
			}

			constexpr iterator_impl operator-(const difference_type off) const noexcept{
				iterator_impl cpy{*this};
				cpy.index - off;
				return cpy;
			}

			constexpr value_type& operator[](const difference_type off) const noexcept{
				size_type offed = index + off;
				return *(offed < pre_size ? pre + offed : post + (offed - pre_size));
			}

			constexpr iterator_impl& operator--() noexcept{
				--index;
				return *this;
			}

			constexpr iterator_impl operator--(int) noexcept{
				iterator_impl i{*this};
				this->operator--();
				return i;
			}


		};
	public:

		using iterator = iterator_impl;
		using sentinel = sentinel_impl;

		[[nodiscard]] constexpr std::uint32_t get_image_index(std::uint32_t ptr_idx) const noexcept{
			if(ptr_idx < pre.size_in_bytes / sizeof(value_type)){
				return pre.view_index;
			}

			return post.view_index;
		}

		[[nodiscard]] iterator begin() const noexcept{
			return {
				reinterpret_cast<value_type*>(pre.data),
				pre.size_in_bytes / sizeof(value_type),
				reinterpret_cast<value_type*>(post.data),
				0,
			};
		}

		[[nodiscard]] sentinel end() const noexcept{
			return {
				static_cast<difference_type>((pre.size_in_bytes + post.size_in_bytes) / sizeof(value_type)),
			};
		}
	};
}
