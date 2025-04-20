// // ReSharper disable CppDFAConstantConditions
// ReSharper disable CppDFAConstantConditions
module;

#include "../src/ext/adapted_attributes.hpp"

export module mo_yanxi.game.graphic.effect:gen_func;

export import mo_yanxi.math;
export import mo_yanxi.math.rand;
export import mo_yanxi.math.vector2;
export import mo_yanxi.math.trans2;
import std;

namespace mo_yanxi::game::fx{
	using seed_type = math::rand::seed_t;

	export
	struct FxParam{
		unsigned count{};
		float progress{};
		float direction{};
		float tolerance_angle{math::pi};
		float base_range{};
		math::range range{};
	};

	export
	template <typename Fn>
	FORCE_INLINE void splash_vec(
		const seed_type seed,
		const FxParam& param,
		Fn fn){

		math::rand rand{seed};
		math::vec2 curVec{};

		for(unsigned i = 0; i < param.count; ++i){
			const auto ang = param.direction + rand(param.tolerance_angle);
			curVec.set_polar_rad(ang, param.base_range + rand.random(param.range.from, param.range.to) * param.progress);

			if constexpr (std::invocable<Fn, math::trans2, math::rand&>){
				std::invoke(fn, math::trans2{curVec, ang}, rand);
			}else if constexpr (std::invocable<Fn, math::vec2, math::rand&>){
				std::invoke(fn, curVec, rand);
			}else if constexpr (std::invocable<Fn, math::trans2>){
				std::invoke(fn, math::trans2{curVec, ang});
			}else if constexpr (std::invocable<Fn, math::vec2>){
				std::invoke(fn, curVec);
			}else{
				static_assert(false, "unsupported function type");
			}
		}
	}

	// export
	// template <std::invocable<Math::Rand&, Geom::Transform, float> Fn>
	// void splashSubOut(const seet_type seed, const float subScale, const FxParam& param, Fn fn){
	// 	Math::Rand rand{seed};
	//
	//
	// 	for(int current = 0; current < param.count; current++){
	// 		const float sBegin = rand.random(1 - subScale);
	// 		float
	// 			fin = (param.progress - sBegin) / subScale;
	//
	// 		if(fin < 0 || fin > 1)continue;
	//
	// 		Math::Rand rand2{seed + current};
	// 		const float theta = rand2.random(param.direction - param.tolerance_angle, param.direction + param.tolerance_angle);
	// 		const auto vec2 = Geom::Vec2::byPolar(theta, param.radius.from + rand.random(param.radius.length()) * fin);
	//
	// 		fn(rand2, Geom::Transform{vec2, theta}, fin);
	// 	}
	// }
}
