module;

#include "ext/adapted_attributes.hpp"

export module volume_draw:primitives;

import mo_yanxi.math.rect_ortho;
import mo_yanxi.hlsl_alias;
import std;

namespace mo_yanxi::graphic::draw::instruction{


struct volume_generic{
	std::uint32_t blend_priority;
	float birth_time;
	float lifetime;
	float depth;
};

namespace volume{

export struct shockwave{
	volume_generic generic;

	float2 pos;
	float radius;
	float stroke{15.f};

	float intensity{12};

	float fade_factor{4.f};
	float blend_factor{1.f};
	float time_effect_factor{0.1f};

	float4 color_from;
	float4 color_to;



	FORCE_INLINE CONST_FN [[nodiscard]] constexpr math::frect get_bound() const noexcept{
		return {pos, radius};
	}
};

}

template <typename T>
	requires requires(T t){
		requires std::is_pointer_interconvertible_with_class(&T::generic);
		{ t.generic } -> std::same_as<volume_generic&>;
	}
[[nodiscard]] FORCE_INLINE constexpr bool check_is_expired(const T& instr, float current_time) noexcept {
	const volume_generic& generic = instr.generic;
	return generic.lifetime + generic.birth_time < current_time;
}

template <typename T>
	requires requires(T t){
		{ t.get_bound() } noexcept -> std::same_as<math::frect>;
	}
[[nodiscard]] FORCE_INLINE constexpr bool get_bound(const T& instr) noexcept {
	return instr.get_bound();
}

}