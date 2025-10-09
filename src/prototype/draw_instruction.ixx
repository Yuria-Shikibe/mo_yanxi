module;

#include <vulkan/vulkan.h>
#include <immintrin.h>
#include "../ext/adapted_attributes.hpp"

export module draw_instruction;
export import mo_yanxi.math.vector2;
export import mo_yanxi.graphic.color;

import mo_yanxi.math;

import std;

namespace mo_yanxi::graphic{


	export
	struct image_view_history{
		static constexpr std::size_t max_cache_count = 4;
		static_assert(max_cache_count % 4 == 0);
		using handle_t = VkImageView;

	private:
		handle_t latest{};
		std::uint32_t latest_index{};
		alignas(32) std::array<handle_t, max_cache_count> images{};
		std::uint32_t count{};

	public:
		FORCE_INLINE void clear(this image_view_history& self) noexcept{
			self = {};
		}

		[[nodiscard]] FORCE_INLINE std::span<const handle_t> get() const noexcept{
			return {images.data(), count};
		}

		[[nodiscard]] FORCE_INLINE /*constexpr*/ std::uint32_t try_push(handle_t image) noexcept{
			if(!image) return std::numeric_limits<std::uint8_t>::max(); //directly vec4(1)
			if(image == latest) return latest_index;

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

			const __m256i target = _mm256_set1_epi64x(std::bit_cast<int64_t>(image));
			const __m256i zero = _mm256_setzero_si256();

			for(std::uint32_t group_idx = 0; group_idx != max_cache_count; group_idx += 4){
				const auto group = _mm256_load_si256(reinterpret_cast<const __m256i*>(images.data() + group_idx));

				const auto eq_mask = _mm256_cmpeq_epi64(group, target);
				if(const auto eq_bits = std::bit_cast<std::uint32_t>(_mm256_movemask_epi8(eq_mask))){
					const auto idx = group_idx + /*local offset*/std::countr_zero(eq_bits) / 8;
					latest = image;
					latest_index = idx;
					return idx;
				}

				const auto null_mask = _mm256_cmpeq_epi64(group, zero);
				if(const auto null_bits = std::bit_cast<std::uint32_t>(_mm256_movemask_epi8(null_mask))){
					const auto idx = group_idx + /*local offset*/std::countr_zero(null_bits) / 8;
					images[idx] = image;
					latest = image;
					latest_index = idx;
					++count;
					return idx;
				}
			}

			return max_cache_count;
		}
	};

}

namespace mo_yanxi::graphic::draw{
	using float2 = math::vec2;
	struct alignas(16) float4 : color{};


	export enum struct draw_instruction_type : std::uint32_t{
		noop,
		uniform_update,

		triangle,
		rectangle,
		line,
		circle,
		partial_circle,


		SIZE,
	};

	constexpr inline float CircleVertPrecision{12};

	export
	FORCE_INLINE constexpr std::uint32_t get_circle_vertices(const float radius) noexcept{
		return math::clamp<std::uint32_t>(static_cast<std::uint32_t>(radius * math::pi / CircleVertPrecision), 10U, 256U);
	}

	export union image_view{
		VkImageView view;
		std::uint64_t index;
	};

	export struct draw_mode{
		std::uint32_t cap;
	};

	export struct alignas(16) primitive_generic{
		image_view image;
		draw_mode mode;
		float depth;
	};

	export struct alignas(16) triangle_draw{
		primitive_generic generic;
		float2 p0, p1, p2;
		float2 uv0, uv1, uv2;
		float4 c0, c1, c2;


		[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::uint32_t get_vertex_count(this const triangle_draw& instruction) noexcept{return 3;}

		[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::uint32_t get_primitive_count(this const triangle_draw& instruction) noexcept{return 1;}

	};

	export struct alignas(16) rectangle_draw{
		primitive_generic generic;
		float2 pos;
		float angle;
		float scale; //TODO uses other?

		float4 c0, c1, c2, c3;
		float2 extent;
		float2 uv00, uv11;


		[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::uint32_t get_vertex_count(this const rectangle_draw& instruction) noexcept{return 4;}

		[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::uint32_t get_primitive_count(this const rectangle_draw& instruction) noexcept{return 2;}

	};

	export struct alignas(16) rectangle_ortho_draw{
		primitive_generic generic;
		float2 v00, v11;
		float4 c0, c1, c2, c3;
		float2 uv00, uv11;


		[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::uint32_t get_vertex_count(this const rectangle_draw& instruction) noexcept{return 4;}

		[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::uint32_t get_primitive_count(this const rectangle_draw& instruction) noexcept{return 2;}

	};

	export struct alignas(16) line_draw{
		primitive_generic generic;
		float2 src, dst;
		float2 uv00, uv11;
		float stroke;
		std::uint32_t cap; //TODO
		float4 csrc, cdst;


		[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::uint32_t get_vertex_count(this const line_draw& instruction) noexcept{return 4;}

		[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::uint32_t get_primitive_count(this const line_draw& instruction) noexcept{return 2;}

	};

	export struct alignas(16) poly_fill_draw{
		primitive_generic generic;
		float2 pos;
		std::uint32_t segments;
		float initial_angle;

		//TODO native dashline support
		std::uint32_t cap1;
		std::uint32_t cap2;

		math::range radius;
		float2 uv00, uv11;
		float4 inner, outer;


		[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::uint32_t get_vertex_count(this const poly_fill_draw& instruction) noexcept{
			return instruction.radius.from == 0 ? instruction.segments + 2 : (instruction.segments + 1) * 2;
		}

		[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::uint32_t get_primitive_count(this const poly_fill_draw& instruction) noexcept{
			return instruction.segments << unsigned(instruction.radius.from != 0);
		}
	};

	export struct partial_poly_fill_draw{
		primitive_generic generic;
		float2 pos;
		std::uint32_t segments;
		std::uint32_t cap;

		math::range radius;
		math::based_section<float> range;
		float2 uv00, uv11;
		float4 inner, outer;

		[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::uint32_t get_vertex_count(this const partial_poly_fill_draw& instruction) noexcept{
			return instruction.radius.from == 0 ? instruction.segments + 1 : instruction.segments * 2;
		}

		[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::uint32_t get_primitive_count(this const partial_poly_fill_draw& instruction) noexcept{
			return instruction.segments << unsigned(instruction.radius.from != 0);
		}
	};


	export union draw_instruction_payload{
		triangle_draw tri;
		rectangle_draw rect;
		line_draw line;
		poly_fill_draw circle;
		partial_poly_fill_draw partial_circle_draw;
	};

	static_assert(std::is_standard_layout_v<draw_instruction_payload>);
	static_assert(std::is_aggregate_v<draw_instruction_payload>);
	static_assert(std::is_trivially_copyable_v<draw_instruction_payload>);
	static_assert(std::is_trivially_constructible_v<draw_instruction_payload>);
	static_assert(std::is_trivially_destructible_v<draw_instruction_payload>);

	static_assert(std::is_pointer_interconvertible_with_class(&triangle_draw::generic));
	static_assert(std::is_pointer_interconvertible_with_class(&rectangle_draw::generic));
	static_assert(std::is_pointer_interconvertible_with_class(&line_draw::generic));
	static_assert(std::is_pointer_interconvertible_with_class(&poly_fill_draw::generic));
	static_assert(std::is_pointer_interconvertible_with_class(&partial_poly_fill_draw::generic));

	template <typename T>
	constexpr inline draw_instruction_type instruction_type_of = draw_instruction_type::uniform_update;

	template <>
	constexpr inline draw_instruction_type instruction_type_of<triangle_draw> = draw_instruction_type::triangle;

	template <>
	constexpr inline draw_instruction_type instruction_type_of<rectangle_draw> = draw_instruction_type::rectangle;

	template <>
	constexpr inline draw_instruction_type instruction_type_of<line_draw> = draw_instruction_type::line;

	template <>
	constexpr inline draw_instruction_type instruction_type_of<poly_fill_draw> = draw_instruction_type::circle;

	template <>
	constexpr inline draw_instruction_type instruction_type_of<partial_poly_fill_draw> = draw_instruction_type::partial_circle;

	export
	template <typename T>
	concept known_instruction = instruction_type_of<T> != draw_instruction_type::uniform_update;


	union dispatch_info_payload{
		struct {
			std::uint32_t vertex_count;
			std::uint32_t primitive_count;
		} draw;

		struct {
			std::uint32_t offset;
			std::uint32_t cap;
		} ubo;
	};


	export
	struct alignas(16) instruction_head{
		draw_instruction_type type;
		std::uint32_t size; //size include head
		dispatch_info_payload payload;

		[[nodiscard]] constexpr std::ptrdiff_t get_instr_byte_size() const noexcept{
			return size << 4u;
		}

		[[nodiscard]] constexpr std::ptrdiff_t get_payload_byte_size() const noexcept{
			return (size - 1 /*head*/) << 4u;
		}
	};

	static_assert(std::is_standard_layout_v<instruction_head>);

	template <typename T>
	constexpr instruction_head make_instruction_head(const T& instr) noexcept{
		static constexpr std::uint32_t payload_size = static_cast<std::uint32_t>(sizeof(instr));
		static constexpr std::ptrdiff_t required = sizeof(instruction_head) + payload_size;
		static_assert(required % 16 == 0);

		const auto vtx = [&] -> std::uint32_t{
			if constexpr (known_instruction<T>){
				return instr.get_vertex_count();
			}else{
				return 0;
			}
		}();

		const auto pmt = [&] -> std::uint32_t{
			if constexpr (known_instruction<T>){
				return instr.get_primitive_count();
			}else{
				return 0;
			}
		}();


		return instruction_head{
			.type = instruction_type_of<T>,
			.size = required / 16,
			.payload = {.draw = {.vertex_count = vtx, .primitive_count = pmt}}
		};
	}

	template <typename T>
	const T* start_lifetime_as(const void* p) noexcept{
		const auto mp = const_cast<void*>(p);
		const auto bytes = new(mp) std::byte[sizeof(T)];
		const auto ptr = reinterpret_cast<const T*>(bytes);
		(void)*ptr;
		return ptr;
	}

	FORCE_INLINE CONST_FN constexpr std::uint32_t get_completed_primitives_at(std::uint32_t vtx) noexcept{
		return vtx < 3 ? 0 : vtx - 2;
	}

	[[nodiscard]] FORCE_INLINE CONST_FN std::uint32_t get_vertex_count(draw_instruction_type type, const void* instruction) noexcept{
		switch(type){
		case draw_instruction_type::triangle: return 3U;
		case draw_instruction_type::rectangle: return 4U;
		case draw_instruction_type::line: return 4U;
		case draw_instruction_type::circle: return static_cast<const poly_fill_draw*>(instruction)->get_vertex_count();
		case draw_instruction_type::partial_circle: return static_cast<const partial_poly_fill_draw*>(instruction)->get_vertex_count();
		default: std::unreachable();
		}
	}

	[[nodiscard]] FORCE_INLINE CONST_FN std::uint32_t get_primitive_count(draw_instruction_type type, const void* instruction) noexcept{
		switch(type){
		case draw_instruction_type::triangle: return 1U;
		case draw_instruction_type::rectangle: return 2U;
		case draw_instruction_type::line: return 2U;
		case draw_instruction_type::circle: return static_cast<const poly_fill_draw*>(instruction)->get_primitive_count();
		case draw_instruction_type::partial_circle: return static_cast<const partial_poly_fill_draw*>(instruction)->get_primitive_count();
		default: std::unreachable();
		}
	}

	[[nodiscard]] FORCE_INLINE CONST_FN std::uint32_t get_primitive_count(draw_instruction_type type, const void* instruction, std::uint32_t pushed_vertex) noexcept{
		assert(get_vertex_count(type, instruction) >= pushed_vertex);
		return get_completed_primitives_at(pushed_vertex);
	}

	export struct instruction_buffer{
		static constexpr std::size_t align = 32;
	private:
		std::byte* src{};
		std::byte* dst{};

	public:
		[[nodiscard]] constexpr instruction_buffer() noexcept = default;

		[[nodiscard]] explicit instruction_buffer(std::size_t byte_size){
			const auto actual_size = ((byte_size + (align - 1)) / align) * align;
			const auto p = ::operator new(actual_size, std::align_val_t{align});
			src = new (p) std::byte[actual_size]{};
			dst = src + actual_size;
		}

		~instruction_buffer(){
			std::destroy_n(src, size());
			::operator delete(src, size(), std::align_val_t{align});
		}

		[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::size_t size() const noexcept{
			return dst - src;
		}

		[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::byte* begin() const noexcept{
			return std::assume_aligned<align>(src);
		}
		//
		// [[nodiscard]] FORCE_INLINE CONST_FN constexpr std::byte* begin() noexcept{
		// 	return std::assume_aligned<align>(src);
		// }

		[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::byte* end() const noexcept{
			return std::assume_aligned<align>(dst);
		}
		//
		// [[nodiscard]] FORCE_INLINE CONST_FN constexpr std::byte* end() noexcept{
		// 	return std::assume_aligned<align>(dst);
		// }

		instruction_buffer(const instruction_buffer& other)
			: instruction_buffer(other.size()){
			std::memcpy(src, other.src, size());
		}

		constexpr instruction_buffer(instruction_buffer&& other) noexcept
			: src{std::exchange(other.src, {})},
			  dst{std::exchange(other.dst, {})}{
		}

		instruction_buffer& operator=(const instruction_buffer& other){
			if(this == &other) return *this;
			*this = instruction_buffer{other};
			return *this;
		}

		constexpr instruction_buffer& operator=(instruction_buffer&& other) noexcept{
			if(this == &other) return *this;
			std::destroy_n(src, size());
			::operator delete(src, size(), std::align_val_t{align});

			src = std::exchange(other.src, {});
			dst = std::exchange(other.dst, {});
			return *this;
		}
	};

	export
	template <typename T>
		requires (std::is_trivially_copyable_v<T>)
	consteval std::size_t get_instr_size() noexcept{
		const auto ret = sizeof(instruction_head) + sizeof(T);
		if(ret % 16 != 0){
			throw std::invalid_argument{"bad alignment"};
		}
		return ret;
	}


	template <typename T>
		requires (std::is_trivially_copyable_v<T>)
	[[nodiscard]] FORCE_INLINE std::byte* place_instr_at_impl(
		std::byte* const where,
		const std::byte* const sentinel,
		const instruction_head& head,
		const T& payload
	){
		if(sentinel - where < get_instr_size<T>() + sizeof(instruction_head))return nullptr;

		const auto pwhere = std::assume_aligned<16>(where);

		// assert(std::ranges::none_of(pwhere, pwhere + draw::get_instr_size<T>() + sizeof(instruction_head), std::to_integer<bool>));

		std::construct_at(reinterpret_cast<instruction_head*>(pwhere), head);
		std::construct_at(reinterpret_cast<T*>(pwhere + sizeof(instruction_head)), payload);
		std::memset(pwhere + get_instr_size<T>(), 0, sizeof(instruction_head));
		return std::assume_aligned<16>(pwhere + get_instr_size<T>());
	}

	export
	template <known_instruction T>
		requires (std::is_trivially_copyable_v<T>)
	[[nodiscard]] FORCE_INLINE std::byte* place_instruction_at(
		std::byte* const where,
		const std::byte* const sentinel,
		const T& payload) noexcept{
		return draw::place_instr_at_impl(where, sentinel, draw::make_instruction_head(payload), payload);
	}

	export
	template <typename T>
		requires (std::is_trivially_copyable_v<T>)
	[[nodiscard]] FORCE_INLINE std::byte* place_ubo_update_at(
		std::byte* const where,
		const std::byte* const sentinel,
		const T& payload,
		const std::uint32_t offset = 0
		) noexcept{
		return draw::place_instr_at_impl(where, sentinel, instruction_head{
			.type = draw_instruction_type::uniform_update,
			.size = get_instr_size<T>() / 16,
			.payload = {.ubo = {.offset = offset}}
		}, payload);
	}

	constexpr inline std::ptrdiff_t GPU_BUF_UNIT_SIZE = 16;

	struct dispatch_result{
		std::byte* next;
		std::uint32_t count;
		std::uint32_t next_primit_offset;
		std::uint32_t next_vertex_offset;
		bool ubo_requires_update;
		bool img_requires_update;

		[[nodiscard]] bool contains_next() const noexcept{
			return next_primit_offset != 0;
		}

		bool host_process_required() const noexcept{
			return next == nullptr || ubo_requires_update || img_requires_update;
		}

		[[nodiscard]] bool no_next(std::byte* original) const noexcept{
			return next == original && count == 0 && !ubo_requires_update && !img_requires_update;
		}
	};

	export
	const instruction_head& get_instr_head(const std::byte* p) noexcept{
		return *start_lifetime_as<instruction_head>(std::assume_aligned<16>(p));
	}

	export
	[[nodiscard]] std::span<const std::byte> get_ubo_data(const std::byte* p) noexcept{
		const auto head = get_instr_head(p);
		const std::size_t ubo_size = head.get_payload_byte_size();
		return std::span{p + sizeof(instruction_head), ubo_size};
	}

	constexpr inline std::uint32_t MaxTaskDispatchPerTime = 32;
	constexpr inline std::uint32_t MaxMeshDispatchPerTask = 16;
	constexpr inline std::uint32_t MaxVerticesPerMesh = 64;
	constexpr inline std::uint32_t MaxVerticesPerTask = MaxVerticesPerMesh * MaxMeshDispatchPerTask;
	constexpr inline std::uint32_t MaxVertices = MaxVerticesPerTask * MaxTaskDispatchPerTime;

	bool set_image_index(void* instruction, image_view_history& cache) noexcept{
		auto& generic = *static_cast<primitive_generic*>(instruction);
		auto idx = cache.try_push(generic.image.view);
		if(idx == image_view_history::max_cache_count)return false;
		std::destroy_at(&generic.image.view);
		std::construct_at(&generic.image.index, idx);
		return true;
	}

	export
	struct alignas(16) dispatch_group_info{
		std::uint32_t instruction_offset; //offset in 16 Byte
		std::uint32_t vertex_offset;
		std::uint32_t primitive_offset;
		std::uint32_t primitive_count;
	};

	/**
	 *
	 * @brief parse given instructions between [begin, sentinel), store mesh dispatch info to storage
	 */
	export
	FORCE_INLINE dispatch_result get_dispatch_info(
		std::byte* const instr_begin,
		const std::byte* const instr_sentinel,
		std::span<dispatch_group_info> storage,
		image_view_history& image_cache,
		const std::uint32_t initial_primitive_offset,
		const std::uint32_t initial_vertex_offset
	) noexcept {
		CHECKED_ASSUME(!storage.empty());
		CHECKED_ASSUME(instr_sentinel >= instr_begin);

		std::uint32_t currentMeshCount{};
		const auto ptr_to_src = std::assume_aligned<16>(instr_begin);

		std::uint32_t verticesBreakpoint{initial_vertex_offset};
		std::uint32_t nextPrimitiveOffset{initial_primitive_offset};

		auto ptr_to_head = std::assume_aligned<16>(instr_begin);

		while(currentMeshCount < storage.size()){
			storage[currentMeshCount].primitive_offset = nextPrimitiveOffset;
			storage[currentMeshCount].vertex_offset = verticesBreakpoint > 2 ? verticesBreakpoint - 2 : 0;

			std::uint32_t pushedVertices{};
			std::uint32_t pushedPrimitives{};

			const auto ptr_to_chunk_head = ptr_to_head;

			const auto get_remain_vertices = [&] FORCE_INLINE {
				return MaxVerticesPerMesh - pushedVertices;
			};

			const auto save_chunk_head_and_incr = [&] FORCE_INLINE {
				if(pushedPrimitives == 0)return;
				assert((ptr_to_chunk_head - ptr_to_src) % GPU_BUF_UNIT_SIZE == 0);
				storage[currentMeshCount].instruction_offset = (ptr_to_chunk_head - ptr_to_src) / GPU_BUF_UNIT_SIZE;
				storage[currentMeshCount].primitive_count = pushedPrimitives;
				++currentMeshCount;

			};

			while(true){
				const auto& head = *start_lifetime_as<instruction_head>(ptr_to_head);
				assert(std::to_underlying(head.type) < std::to_underlying(draw::draw_instruction_type::SIZE));
				if(ptr_to_head + head.get_instr_byte_size() > instr_sentinel){
					save_chunk_head_and_incr();
					return {ptr_to_head, currentMeshCount, nextPrimitiveOffset, verticesBreakpoint, false, false};
				}

				switch(head.type){
				case draw_instruction_type::noop:
					save_chunk_head_and_incr();
					return {ptr_to_head, currentMeshCount, nextPrimitiveOffset, verticesBreakpoint, false, false};
				case draw_instruction_type::uniform_update:
					save_chunk_head_and_incr();
					return {ptr_to_head, currentMeshCount, nextPrimitiveOffset, verticesBreakpoint, true, false};
				default: break;
				}

				if(!set_image_index(ptr_to_head + sizeof(instruction_head), image_cache)){
					save_chunk_head_and_incr();
					return {ptr_to_head, currentMeshCount, nextPrimitiveOffset, verticesBreakpoint, false, true};
				}

				auto nextVertices = get_vertex_count(head.type, ptr_to_head + sizeof(instruction_head));

				if(verticesBreakpoint){
					assert(verticesBreakpoint >= 3);
					assert(verticesBreakpoint < nextVertices);
					nextVertices -= (verticesBreakpoint -= 2); //make sure a complete primitive is draw
				}

				if(pushedVertices + nextVertices <= MaxVerticesPerMesh){
					verticesBreakpoint = 0;
					pushedVertices += nextVertices;
					pushedPrimitives += get_primitive_count(head.type, ptr_to_head + sizeof(instruction_head), nextVertices);
					nextPrimitiveOffset = 0;

					ptr_to_head = std::assume_aligned<16>(ptr_to_head + head.get_instr_byte_size());

					if(pushedVertices == MaxVerticesPerMesh)break;
				}else{
					const auto remains = get_remain_vertices();
					const auto primits = get_primitive_count(head.type, ptr_to_head + sizeof(instruction_head), remains);
					nextPrimitiveOffset += primits;
					pushedPrimitives += primits;
					verticesBreakpoint += remains;

					break;
				}
			}

			save_chunk_head_and_incr();
		}

		return {ptr_to_head, currentMeshCount, nextPrimitiveOffset, verticesBreakpoint, false, false};
	}

}