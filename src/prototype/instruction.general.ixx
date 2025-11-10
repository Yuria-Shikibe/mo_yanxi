module;

#include "ext/adapted_attributes.hpp"
#include <immintrin.h>
#include <vulkan/vulkan.h>

export module mo_yanxi.graphic.draw.instruction.general;
import std;

namespace mo_yanxi::graphic::draw::instruction{

export template<typename Rng, typename T>
concept contiguous_range_of = std::ranges::contiguous_range<Rng> && std::same_as<std::ranges::range_value_t<Rng>, T>;


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
		std::memset(begin(), 0, size());
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


template <typename T>
constexpr std::size_t get_size(const T& arg){
	if constexpr (std::ranges::input_range<T>){
		static_assert(std::ranges::sized_range<T>, "T must be sized range");
		return std::ranges::size(arg) * sizeof(std::ranges::range_value_t<T>);
	}else{
		static_assert(std::is_trivially_copyable_v<T>);
		return sizeof(T);
	}
}

export
template <typename... Args>
constexpr std::size_t get_type_size_sum(const Args& ...args) noexcept{
	return ((instruction::get_size(args)) + ... + 0);
}

export
template <typename T>
[[nodiscard]] const T* start_lifetime_as(const void* p) noexcept{
	const auto mp = const_cast<void*>(p);
	const auto bytes = new(mp) std::byte[sizeof(T)];
	const auto ptr = reinterpret_cast<const T*>(bytes);
	(void)*ptr;
	return ptr;
}

export
struct user_data_indices{
	std::uint32_t local_index;
	std::uint32_t group_index;
};

static_assert(sizeof(user_data_indices) == 8);

export
template <typename T>
	requires (sizeof(T) <= 8)
union dispatch_info_payload{
	T draw;

	user_data_indices ubo;
};

export
template <typename EnumTy, typename DrawInfoTy>
	requires (std::is_enum_v<EnumTy>)
struct alignas(16)
generic_instruction_head{
	EnumTy type;
	std::uint32_t size; //size include head
	dispatch_info_payload<DrawInfoTy> payload;

	[[nodiscard]] constexpr std::ptrdiff_t get_instr_byte_size() const noexcept{
		return size;
	}

	[[nodiscard]] constexpr std::ptrdiff_t get_payload_byte_size() const noexcept{
		return size - sizeof(generic_instruction_head);
	}
};

export
struct batch_backend_interface{
	using host_impl_ptr = void*;

	using function_signature_buffer_acquire = void*(host_impl_ptr, std::size_t instruction_size);
	using function_signature_consume_all = void(host_impl_ptr);
	using function_signature_wait_idle = void(host_impl_ptr);

private:
	host_impl_ptr host;
	std::add_pointer_t<function_signature_buffer_acquire> fptr_acquire;
	std::add_pointer_t<function_signature_consume_all> fptr_consume;
	std::add_pointer_t<function_signature_wait_idle> fptr_wait_idle;

public:
	[[nodiscard]] constexpr batch_backend_interface() = default;

	template <typename HostT, std::invocable<HostT&, std::size_t> AcqFn, std::invocable<HostT&> ConsumeFn, std::invocable<HostT&> WaitIdleFn>
		requires (std::convertible_to<std::invoke_result_t<AcqFn, HostT&, std::size_t>, void*>)
	[[nodiscard]] constexpr batch_backend_interface(
		HostT& host,
		AcqFn,
		ConsumeFn,
		WaitIdleFn
	) noexcept : host(std::addressof(host)), fptr_acquire(+[](host_impl_ptr host, std::size_t instruction_size) static -> void* {
		return AcqFn::operator()(*static_cast<HostT*>(host), instruction_size);
	}), fptr_consume(+[](host_impl_ptr host) static {
		ConsumeFn::operator()(*static_cast<HostT*>(host));
	}), fptr_wait_idle(+[](host_impl_ptr host) static {
		WaitIdleFn::operator()(*static_cast<HostT*>(host));
	}){

	}

	explicit operator bool() const noexcept{
		return host;
	}

	template <typename HostTy>
	HostTy& get_host() const noexcept{
		CHECKED_ASSUME(host != nullptr);
		return *static_cast<HostTy*>(host);
	}

	template <typename T = std::byte>
	T* acquire(std::size_t instruction_size) const {
		CHECKED_ASSUME(fptr_acquire != nullptr);
		return static_cast<T*>(fptr_acquire(host, instruction_size));
	}

	void consume_all() const{
		CHECKED_ASSUME(fptr_consume != nullptr);
		fptr_consume(host);
	}

	void wait_idle() const{
		CHECKED_ASSUME(fptr_wait_idle != nullptr);
		fptr_wait_idle(host);
	}

	void flush() const{
		consume_all();
		wait_idle();
	}
};


}


namespace mo_yanxi::graphic::draw{
namespace instruction{

export
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

export enum struct instr_type : std::uint32_t{
	noop,
	uniform_update,

	triangle,
	quad,
	rectangle,

	line,
	line_segments,
	line_segments_closed,

	poly,
	poly_partial,

	constrained_curve,


	rect_ortho,
	rect_ortho_outline,
	row_patch,

	SIZE,
};

export struct draw_payload{
	std::uint32_t vertex_count;
	std::uint32_t primitive_count;
};

export using instruction_head = generic_instruction_head<instr_type, draw_payload>;

static_assert(std::is_standard_layout_v<instruction_head>);

export
template <typename Instr, typename Arg>
struct is_valid_consequent_argument : std::false_type{

};

template <typename Instr, typename Arg>
constexpr inline bool is_valid_consequent_argument_v = is_valid_consequent_argument<Instr, Arg>::value;

export
template <typename Instr, typename... Args>
concept valid_consequent_argument = (is_valid_consequent_argument_v<Instr, Args> && ...);

export
template <typename T>
constexpr inline instr_type instruction_type_of = instr_type::uniform_update;

export
template <typename T>
concept known_instruction = instruction_type_of<T> != instr_type::uniform_update;

export
template <typename T, typename... Args>
	requires (std::is_trivially_copyable_v<T> && valid_consequent_argument<T, Args...>)
constexpr std::size_t get_instr_size(const Args& ...args) noexcept{
	return sizeof(instruction_head)  + sizeof(T) + instruction::get_type_size_sum<Args...>(args...);
}

export
template <typename T, typename... Args>
	requires requires{
		requires !known_instruction<T> || requires(const T& instr, const Args&... args){
			{instr.get_vertex_count(args...)} -> std::convertible_to<std::uint32_t>;
		};
	}
constexpr instruction_head make_instruction_head(const T& instr, const Args&... args) noexcept{
	const auto required = instruction::get_instr_size<T, Args...>(args...);
	assert(required % 16 == 0);

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
		.size = static_cast<std::uint32_t>(required),
		.payload = {.draw = {.vertex_count = vtx, .primitive_count = pmt}}
	};
}


export
[[nodiscard]] FORCE_INLINE const instruction_head& get_instr_head(const void* p) noexcept{
	return *start_lifetime_as<instruction_head>(std::assume_aligned<16>(p));
}


template <typename T, typename... Args>
	requires (std::is_trivially_copyable_v<T> && valid_consequent_argument<T, Args...>)
[[nodiscard]] FORCE_INLINE std::byte* place_instr_at_impl(
	std::byte* const where,
	const instruction_head& head,
	const T& payload,
	const Args&... args
){
	static_assert(((sizeof(Args) % 16 == 0) && ...));

	const auto total_size = instruction::get_instr_size<T, Args...>(args...);

	auto pwhere = std::assume_aligned<16>(where);

	std::memcpy(pwhere, &head, sizeof(instruction_head));
	pwhere += sizeof(instruction_head);
	std::memcpy(pwhere, &payload, sizeof(payload));

	static constexpr auto place_at = []<typename Ty>(std::byte*& w, const Ty& arg){
		if constexpr (std::ranges::contiguous_range<Ty>){
			//no use uniti
			const auto byte_size = sizeof(std::ranges::range_value_t<Ty>) * std::ranges::size(arg);
			std::memcpy(w, std::ranges::data(arg), byte_size);
			return w = std::assume_aligned<16>(w + byte_size);
		}else{
			std::memcpy(w, &arg, sizeof(Ty));
			return w = std::assume_aligned<16>(w + sizeof(Ty));
		}
	};

	if constexpr(sizeof...(args) > 0){
		pwhere += sizeof(instruction_head);

		[&]<std::size_t ... Idx>(std::index_sequence<Idx...>) FORCE_INLINE{
			(place_at.template operator()<Args>(pwhere, args), ...);
		}(std::make_index_sequence<sizeof...(Args)>{});

		std::memset(pwhere, 0, sizeof(instruction_head));
		return pwhere;
	} else{
		const auto end = std::assume_aligned<16>(pwhere + total_size - sizeof(instruction_head));
		std::memset(end, 0, sizeof(instruction_head));
		return end;
	}
}

export
template <known_instruction T, typename... Args>
	requires (std::is_trivially_copyable_v<T> && valid_consequent_argument<T, Args...>)
[[nodiscard]] FORCE_INLINE std::byte* place_instruction_at(
	std::byte* const where,
	const T& payload,
	const Args&... args
) noexcept{
	return instruction::place_instr_at_impl(
		where, instruction::make_instruction_head(payload, args...), payload, args...);
}

export
template <typename T>
	requires (std::is_trivially_copyable_v<T>)
[[nodiscard]] FORCE_INLINE std::byte* place_ubo_update_at(
	std::byte* const where,
	const T& payload,
	const user_data_indices info
) noexcept{
	return instruction::place_instr_at_impl(
		where, instruction_head{
			.type = instr_type::uniform_update,
			.size = get_instr_size<T>(),
			.payload = {.ubo = info}
		}, payload);
}

export
[[nodiscard]] FORCE_INLINE inline std::span<const std::byte> get_ubo_data_span(const std::byte* ptr_to_instr) noexcept{
	const auto head = get_instr_head(ptr_to_instr);
	const std::size_t ubo_size = head.get_payload_byte_size();
	return std::span{ptr_to_instr + sizeof(instruction_head), ubo_size};
}

export
FORCE_INLINE inline image_set_result set_image_index(void* instruction, image_view_history& cache) noexcept{
	auto& generic = *static_cast<primitive_generic*>(instruction);

	const auto view = generic.image.get_image_view();
	assert(std::bit_cast<std::uintptr_t>(view) != 0x00000000'ffffffffULL);
	const auto idx = cache.try_push(view);
	if(idx == image_view_history::max_cache_count) return image_set_result{view, false};
	generic.image.set_index(idx);
	return image_set_result{view, true};
}



}
}
