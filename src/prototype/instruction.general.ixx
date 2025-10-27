module;

#include "ext/adapted_attributes.hpp"

export module mo_yanxi.graphic.draw.instruction.general;
export import mo_yanxi.math.vector2;
export import mo_yanxi.graphic.color;

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

export using float1 = float;
export using float2 = math::vec2;


export struct alignas(16) float4 : color{
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
template <typename T>
	requires (sizeof(T) <= 8)
union dispatch_info_payload{
	T draw;

	struct{
		std::uint32_t index;
		std::uint32_t offset;
	} ubo;
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
}