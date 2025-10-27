export module mo_yanxi.game.graphic.lightning;

import mo_yanxi.math;
import mo_yanxi.math.rand;
import mo_yanxi.math.vector2;
import std;


namespace mo_yanxi::game::fx{

	std::pmr::synchronized_pool_resource res{std::pmr::new_delete_resource()};


	struct lightning_node{

	};

	export
	struct lightning_segment{
		math::rand& rand;
		math::vec2 pos;

		float factor_local;
		float factor_global;
	};

	export
	template <template<typename> typename Generator, std::invocable<lightning_segment> Fn>
	[[nodiscard]] Generator<std::invoke_result_t<Fn, lightning_segment>> lightning_generator(
		const math::vec2 start, const math::vec2 end,
		const math::range spacing,
		const float factor,
		math::rand rand,
		Fn&& consumer
	) noexcept(std::is_nothrow_invocable_v<Fn, lightning_segment>){
		const auto approach = end - start;
		const auto full_dst = approach.length();
		const auto dst = full_dst * factor;
		float moved = 0;

		co_yield consumer(lightning_segment{rand, start, 1.f, 0.f});

		while(true){
			const auto cur_mov = rand.random(spacing.from, spacing.to);
			if(moved + cur_mov >= dst){
				if(factor >= 1){
					co_yield consumer(lightning_segment{rand, end, 1.f, 1.f});
					co_return;
				}

				const auto lfact = (dst - moved) / cur_mov;
				const auto fac = (moved + cur_mov) / full_dst;

				const auto original_next = math::lerp(start, end, fac);
				co_yield consumer(lightning_segment{rand, original_next, lfact, fac});
				co_return;
			}

			const auto actual_pos = moved + cur_mov;
			const auto fac = actual_pos / full_dst;
			const auto next = math::lerp(start, end, fac);
			co_yield consumer(lightning_segment{rand, next, 1.f, actual_pos / full_dst});

			moved = actual_pos;
		}
	}

	struct lightning_static{
	private:
		std::pmr::vector<math::vec2> seq_{&res};

		void generateLightning_impl(const math::vec2 start, const math::vec2 end, const unsigned segments, const float displacement, math::rand& rand) {
			if (segments <= 0) {
				seq_.push_back(end);
				return;
			}

			const auto delta = end - start;
			const auto cur_displacement = rand.range(displacement);
			const auto normal = delta.copy().rotate_rt_counter_clockwise().normalize_or_zero();
			const auto next_mid = math::fma(normal, cur_displacement, math::fma(delta, rand.random(.35f, .65f), start));

			generateLightning_impl(start, next_mid, segments - 1, displacement / 2, rand);
			generateLightning_impl(next_mid, end, segments - 1, displacement / 2, rand);
		}
	public:
		[[nodiscard]] lightning_static() = default;

		[[nodiscard]] lightning_static(
			const math::vec2 start, const math::vec2 end,
			const unsigned segments, const float displacement,
			const std::uint64_t seed = math::rand::get_default_seed()){
			seq_.reserve(segments * 2 + 1);
			seq_.push_back(end);
			math::rand rand{seed};
			generateLightning_impl(start, end, segments, displacement, rand);
		}
	};
}