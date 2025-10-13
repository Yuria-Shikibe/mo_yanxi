module;

#include <vulkan/vulkan.h>
#include "ext/adapted_attributes.hpp"

export module mo_yanxi.graphic.draw.instruction_draw:primitives;

import :facility;

namespace mo_yanxi::graphic::draw::instruction{

constexpr inline std::uint32_t MaxTaskDispatchPerTime = 32;
constexpr inline std::uint32_t MaxVerticesPerMesh = 64;

using float1 = float;
using float2 = math::vec2;


struct alignas(16) float4 : color{
};

constexpr inline float CircleVertPrecision{12};

export
FORCE_INLINE constexpr std::uint32_t get_circle_vertices(const float radius) noexcept{
	return math::clamp<std::uint32_t>(static_cast<std::uint32_t>(radius * math::pi / CircleVertPrecision), 10U, 256U);
}

export struct alignas(16) triangle{
	primitive_generic generic;
	float2 p0, p1, p2;
	float2 uv0, uv1, uv2;
	float4 c0, c1, c2;


	[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::uint32_t get_vertex_count(
		this const triangle& instruction) noexcept{ return 3; }

	[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::uint32_t get_primitive_count(
		this const triangle& instruction) noexcept{ return 1; }
};

export struct alignas(16) rectangle{
	primitive_generic generic;
	float2 pos;
	float angle;
	float scale; //TODO uses other?

	float4 c0, c1, c2, c3;
	float2 extent;
	float2 uv00, uv11;


	[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::uint32_t get_vertex_count(
		this const rectangle& instruction) noexcept{ return 4; }

	[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::uint32_t get_primitive_count(
		this const rectangle& instruction) noexcept{ return 2; }
};

export struct alignas(16) rectangle_ortho{
	primitive_generic generic;
	float2 v00, v11;
	float4 c0, c1, c2, c3;
	float2 uv00, uv11;


	[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::uint32_t get_vertex_count(
		this const rectangle& instruction) noexcept{ return 4; }

	[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::uint32_t get_primitive_count(
		this const rectangle& instruction) noexcept{ return 2; }
};

export struct alignas(16) line_segments{
	primitive_generic generic;

	[[nodiscard]] FORCE_INLINE CONST_FN static constexpr std::uint32_t get_vertex_count(
		std::size_t node_payload_size
	) noexcept{
		assert(node_payload_size >= 2);
		return node_payload_size * 2;
	}

	template <typename... Args>
	[[nodiscard]] FORCE_INLINE CONST_FN static constexpr std::uint32_t get_vertex_count(
		const Args&...
	) noexcept{
		return line_segments::get_vertex_count(sizeof...(Args));
	}

	[[nodiscard]] FORCE_INLINE CONST_FN static constexpr std::uint32_t get_primitive_count(
		std::size_t node_payload_size
	) noexcept{
		assert(node_payload_size >= 2);
		return (node_payload_size - 1) * 2;
	}


	template <typename... Args>
	[[nodiscard]] FORCE_INLINE CONST_FN static constexpr std::uint32_t get_primitive_count(
		const Args&...
	) noexcept{
		return line_segments::get_primitive_count(sizeof...(Args));
	}
};

export struct alignas(16) line_node{
	float2 pos;
	float stroke;
	float offset; //TODO ?
	float4 color;

	//TODO uv?
};

export struct alignas(16) poly{
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


	[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::uint32_t get_vertex_count(
		this const poly& instruction) noexcept{
		return instruction.radius.from == 0 ? instruction.segments + 2 : (instruction.segments + 1) * 2;
	}

	[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::uint32_t get_primitive_count(
		this const poly& instruction) noexcept{
		return instruction.segments << unsigned(instruction.radius.from != 0);
	}
};

export struct alignas(16) poly_partial{
	primitive_generic generic;
	float2 pos;
	std::uint32_t segments;
	std::uint32_t cap;

	math::range radius;
	math::based_section<float> range;
	float2 uv00, uv11;
	float4 inner, outer;

	[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::uint32_t get_vertex_count(
		this const poly_partial& instruction) noexcept{
		return instruction.radius.from == 0 ? instruction.segments + 2 : (instruction.segments + 1) * 2;
	}

	[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::uint32_t get_primitive_count(
		this const poly_partial& instruction) noexcept{
		return instruction.segments << unsigned(instruction.radius.from != 0);
	}
};

export struct curve_parameter{
	std::array<math::vec4, 2> constrain_vector;
	// std::array<math::vec4, 2> constrain_vector_derivative;
};

export struct alignas(16) constrained_curve{
	primitive_generic generic;
	curve_parameter param;

	math::based_section<float> factor_range;
	std::uint32_t segments;
	float stroke;

	float2 uv00, uv11;
	float4 src_color, dst_color;

	[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::uint32_t get_vertex_count(
		this const constrained_curve& instruction) noexcept{
		return instruction.segments * 2 + 2;
	}

	[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::uint32_t get_primitive_count(
		this const constrained_curve& instruction) noexcept{
		return instruction.segments * 2;
	}
};

export
using curve_ctrl_handle = std::array<math::vec2, 4>;

export
struct curve_trait_matrix{
	static constexpr math::matrix4 dt{
			{}, {0, 1}, {0, 0, 2}, {0, 0, 0, 3}
		};


	math::matrix4 trait;

	[[nodiscard]] curve_trait_matrix() = default;

	[[nodiscard]] explicit(false) constexpr curve_trait_matrix(const math::matrix4& trait)
		: trait(trait){
	}

	FORCE_INLINE constexpr friend curve_parameter operator*(const curve_trait_matrix& lhs,
	                                                        const curve_ctrl_handle& rhs) noexcept{
		std::array<math::vec4, 2> rst_0;
		rst_0[0] = lhs.trait.c0 * rhs[0].x + lhs.trait.c1 * rhs[1].x + lhs.trait.c2 * rhs[2].x + lhs.trait.c3 * rhs[3].
			x;
		rst_0[1] = lhs.trait.c0 * rhs[0].y + lhs.trait.c1 * rhs[1].y + lhs.trait.c2 * rhs[2].y + lhs.trait.c3 * rhs[3].
			y;

		return {rst_0};
	}
};

namespace curve_trait_mat{
export constexpr inline curve_trait_matrix bezier{
		{{1.f, -3.f, 3.f, -1.f}, {0, 3, -6, 3}, {0, 0, 3, -3}, {0, 0, 0, 1}}
	};

export constexpr inline curve_trait_matrix hermite{
		{{1, 0, -3, 2}, {0, 1, -2, 1}, {0, 0, 3, -2}, {0, 0, -1, 1}}
	};

export
template <float tau = .5f>
constexpr inline curve_trait_matrix catmull_rom{
		math::matrix4{
			{0, -tau, 2 * tau, -tau}, {1, 0, tau - 3, 2 - tau}, {0, tau, 3 - 2 * tau, tau - 2}, {0, 0, -tau, tau}
		}
	};

export
constexpr inline curve_trait_matrix b_spline{
		(1.f / 6.f) * math::matrix4{{1, -3, 3, -1}, {4, 0, -6, 3}, {1, 3, 3, -3}, {0, 0, 0, 1}}
	};
}

template <>
constexpr inline bool is_valid_consequent_argument<line_segments, line_node> = true;

template <>
constexpr inline instr_type instruction_type_of<triangle> = instr_type::triangle;

template <>
constexpr inline instr_type instruction_type_of<rectangle> = instr_type::rectangle;

template <>
constexpr inline instr_type instruction_type_of<line_segments> = instr_type::line_segments;

template <>
constexpr inline instr_type instruction_type_of<poly> = instr_type::poly;

template <>
constexpr inline instr_type instruction_type_of<poly_partial> =
	instr_type::poly_partial;

template <>
constexpr inline instr_type instruction_type_of<constrained_curve> =
	instr_type::constrained_curve;


[[nodiscard]] FORCE_INLINE CONST_FN std::uint32_t get_vertex_count(
	instr_type type,
	const std::byte* ptr_to_instr) noexcept{

	switch(type){
	case instr_type::triangle : return 3U;
	case instr_type::rectangle : return 4U;
	case instr_type::line_segments :{
		const auto size = get_instr_head(ptr_to_instr).get_instr_byte_size();
		const auto payloadByteSize = (size - get_instr_size<line_segments>());
		assert(payloadByteSize % sizeof(line_node) == 0);
		const auto payloadCount = payloadByteSize / sizeof(line_node);
		return line_segments::get_vertex_count(payloadCount);
	}
	case instr_type::poly : return reinterpret_cast<const poly*>(ptr_to_instr + sizeof(
			instruction_head))->get_vertex_count();
	case instr_type::poly_partial : return reinterpret_cast<const poly_partial*>(ptr_to_instr +
			sizeof(instruction_head))->get_vertex_count();
	case instr_type::constrained_curve : return reinterpret_cast<const constrained_curve*>(ptr_to_instr +
			sizeof(instruction_head))->get_vertex_count();
	default : std::unreachable();
	}
}

[[nodiscard]] FORCE_INLINE CONST_FN constexpr std::uint32_t get_primitive_count(std::uint32_t vtx) noexcept{
	return vtx < 3 ? 0 : vtx - 2;
}

}
