module;

#include <immintrin.h>
#include <vulkan/vulkan.h>
#include "ext/adapted_attributes.hpp"

export module mo_yanxi.graphic.draw.instruction_draw:facility;

export import mo_yanxi.math.vector2;
export import mo_yanxi.math.vector4;
export import mo_yanxi.math.matrix4;
export import mo_yanxi.graphic.color;

import mo_yanxi.math;

import std;

namespace mo_yanxi::graphic::draw{
namespace instruction{

struct image_view_history{
	static constexpr std::size_t max_cache_count = 4;
	static_assert(max_cache_count % 4 == 0);
	static_assert(sizeof(void*) == sizeof(std::uint64_t));
	using handle_t = VkImageView;

private:
	handle_t latest{};
	std::uint32_t latest_index{};
	alignas(32) std::array<handle_t, max_cache_count> images{};
	std::uint32_t count{};
	bool changed{};

public:
	bool check_changed() noexcept{
		return std::exchange(changed, false);
	}

	FORCE_INLINE void clear(this image_view_history& self) noexcept{
		self = {};
	}

	[[nodiscard]] FORCE_INLINE std::span<const handle_t> get() const noexcept{
		return {images.data(), count};
	}

	[[nodiscard]] FORCE_INLINE /*constexpr*/ std::uint32_t try_push(handle_t image) noexcept{
		if(!image) return std::numeric_limits<std::uint32_t>::max(); //directly vec4(1)
		if(image == latest) return latest_index;
		//
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
		// 		changed = true;
		// 		return i;
		// 	}
		// }

		const __m256i target = _mm256_set1_epi64x(std::bit_cast<std::int64_t>(image));
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
				count = idx + 1;
				changed = true;
				return idx;
			}
		}

		return max_cache_count;
	}
};


struct instruction_buffer{
 	static constexpr std::size_t align = 32;

private:
	std::byte* src{};
	std::byte* dst{};

public:
	[[nodiscard]] constexpr instruction_buffer() noexcept = default;

	[[nodiscard]] explicit instruction_buffer(std::size_t byte_size){
		const auto actual_size = ((byte_size + (align - 1)) / align) * align;
		const auto p = ::operator new(actual_size, std::align_val_t{align});
		src = new(p) std::byte[actual_size]{};
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

	void clear() const noexcept{
		std::memset(src, 0, size());
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


struct image_set_result{
	VkImageView image;
	bool succeed;

	constexpr explicit operator bool() const noexcept{
		return succeed;
	}
};


export union image_view{
	VkImageView view;
	std::uint64_t index;

	void set_view(VkImageView view) noexcept{
		new(&this->view) VkImageView(view);
	}

	void set_index(std::uint32_t idx) noexcept{
		new(&this->index) std::uint64_t(idx);
	}

	[[nodiscard]] VkImageView get_image_view() const noexcept{
		return view; // std::bit_cast<VkImageView>(std::bit_cast<std::uintptr_t>(*this) & (~0b111));
	}
};

export struct draw_mode{
	std::uint32_t cap;
};

export struct alignas(16) primitive_generic{
	image_view image;
	draw_mode mode;
	float depth;
};

enum struct instr_type : std::uint32_t{
	noop,
	uniform_update,

	triangle,
	rectangle,

	//TODO trivial line
	line_segments,
	//TODO line_segments_closed,

	poly,
	poly_partial,

	constrained_curve,

	//TODO nine patch
	//TODO ortho rect
	//TODO ortho rect bound

	SIZE,
};


union dispatch_info_payload{
	struct{
		std::uint32_t vertex_count;
		std::uint32_t primitive_count;
	} draw;

	struct{
		std::uint32_t offset;
		std::uint32_t cap;
	} ubo;
};

struct alignas(16) instruction_head{
	instr_type type;
	std::uint32_t size; //size include head
	dispatch_info_payload payload;

	[[nodiscard]] constexpr std::ptrdiff_t get_instr_byte_size() const noexcept{
		return size;
	}

	[[nodiscard]] constexpr std::ptrdiff_t get_payload_byte_size() const noexcept{
		return size - sizeof(instruction_head);
	}
};

static_assert(std::is_standard_layout_v<instruction_head>);


template <typename Instr, typename Arg>
constexpr inline bool is_valid_consequent_argument = false;

export
template <typename Instr, typename... Args>
concept valid_consequent_argument = (is_valid_consequent_argument<Instr, Args> && ...);

export
template <typename T>
constexpr inline instr_type instruction_type_of = instr_type::uniform_update;

export
template <typename T>
concept known_instruction = instruction_type_of<T> != instr_type::uniform_update;




template <typename T, typename... Args>
	requires (std::is_trivially_copyable_v<T> && valid_consequent_argument<T, Args...>)
consteval std::size_t get_instr_size() noexcept{
	const auto ret = sizeof(instruction_head) + sizeof(T);
	return ret + []{
		if constexpr(sizeof...(Args) > 0){
			return (sizeof(Args) + ...);
		} else{
			return 0;
		}
	}();
}

template <typename T, typename... Args>
constexpr instruction_head make_instruction_head(const T& instr, const Args&... args) noexcept{
	static constexpr std::ptrdiff_t required = get_instr_size<T, Args...>();
	static_assert(required % 16 == 0);

	const auto vtx = [&] -> std::uint32_t{
		if constexpr(known_instruction<T>){
			return instr.get_vertex_count(args...);
		} else{
			return 0;
		}
	}();

	const auto pmt = [&] -> std::uint32_t{
		if constexpr(known_instruction<T>){
			return instr.get_primitive_count(args...);
		} else{
			return 0;
		}
	}();


	return instruction_head{
		.type = instruction_type_of<T>,
		.size = required,
		.payload = {.draw = {.vertex_count = vtx, .primitive_count = pmt}}
	};
}


template <typename T>
[[nodiscard]] const T* start_lifetime_as(const void* p) noexcept{
	const auto mp = const_cast<void*>(p);
	const auto bytes = new(mp) std::byte[sizeof(T)];
	const auto ptr = reinterpret_cast<const T*>(bytes);
	(void)*ptr;
	return ptr;
}

[[nodiscard]] FORCE_INLINE const instruction_head& get_instr_head(const void* p) noexcept{
	return *start_lifetime_as<instruction_head>(std::assume_aligned<16>(p));
}


template <typename T, typename... Args>
	requires (std::is_trivially_copyable_v<T> && valid_consequent_argument<T, Args...>)
[[nodiscard]] FORCE_INLINE std::byte* place_instr_at_impl(
	std::byte* const where,
	const std::byte* const sentinel,
	const instruction_head& head,
	const T& payload,
	const Args&... args
){
	static_assert(((sizeof(Args) % 16 == 0) && ...));

	static constexpr auto total_size = instruction::get_instr_size<T, Args...>();

	if(sentinel - where < total_size + sizeof(instruction_head)) return nullptr;

	auto pwhere = std::assume_aligned<16>(where);

	std::construct_at(reinterpret_cast<instruction_head*>(pwhere), head);

	pwhere += sizeof(instruction_head);
	std::construct_at(reinterpret_cast<T*>(pwhere), payload);

	if constexpr(sizeof...(args) > 0){
		pwhere += sizeof(instruction_head);

		[&]<std::size_t ... Idx>(std::index_sequence<Idx...>) FORCE_INLINE{
			((std::construct_at(reinterpret_cast<Args*>(pwhere), args), pwhere = std::assume_aligned<16>(
				pwhere + sizeof(Args))), ...);
		}(std::make_index_sequence<sizeof...(Args)>{});

		std::memset(pwhere, 0, sizeof(instruction_head));
		return pwhere;
	} else{
		const auto end = std::assume_aligned<16>(pwhere + total_size - sizeof(instruction_head));
		std::memset(end, 0, sizeof(instruction_head));
		return end;
	}
}

template <known_instruction T, typename... Args>
	requires (std::is_trivially_copyable_v<T> && valid_consequent_argument<T, Args...>)
[[nodiscard]] FORCE_INLINE std::byte* place_instruction_at(
	std::byte* const where,
	const std::byte* const sentinel,
	const T& payload,
	const Args&... args
) noexcept{
	return instruction::place_instr_at_impl(
		where, sentinel, instruction::make_instruction_head(payload, args...), payload, args...);
}

template <typename T>
	requires (std::is_trivially_copyable_v<T>)
[[nodiscard]] FORCE_INLINE std::byte* place_ubo_update_at(
	std::byte* const where,
	const std::byte* const sentinel,
	const T& payload,
	const std::uint32_t offset = 0
) noexcept{
	return instruction::place_instr_at_impl(
		where, sentinel, instruction_head{
			.type = instr_type::uniform_update,
			.size = get_instr_size<T>(),
			.payload = {.ubo = {.offset = offset}}
		}, payload);
}

[[nodiscard]] FORCE_INLINE inline std::span<const std::byte> get_ubo_data_span(const std::byte* ptr_to_instr) noexcept{
	const auto head = get_instr_head(ptr_to_instr);
	const std::size_t ubo_size = head.get_payload_byte_size();
	return std::span{ptr_to_instr + sizeof(instruction_head), ubo_size};
}

FORCE_INLINE inline image_set_result set_image_index(void* instruction, image_view_history& cache) noexcept{
	auto& generic = *static_cast<primitive_generic*>(instruction);

	const auto view = generic.image.get_image_view();
	const auto idx = cache.try_push(view);
	if(idx == image_view_history::max_cache_count) return image_set_result{view, false};
	generic.image.set_index(idx);
	return image_set_result{view, true};
}



}
}
