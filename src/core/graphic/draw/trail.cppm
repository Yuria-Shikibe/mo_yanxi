module;

#include <cassert>
#include "../src/ext/adapted_attributes.hpp"

export module mo_yanxi.graphic.trail;

export import mo_yanxi.math.vector2;
export import mo_yanxi.math.rect_ortho;
import mo_yanxi.math;
import mo_yanxi.math.interpolation;
import ext.timer;
import mo_yanxi.concepts;
import mo_yanxi.circular_queue;

import std;

namespace mo_yanxi::graphic{
	export
	struct trail{
		using vec_t = math::vec2;
		
		struct node{
			vec_t pos;
			float scale;

			FORCE_INLINE constexpr friend node lerp(const node lhs, const node rhs, const float p) noexcept{
				return node_type{
					math::lerp(lhs.pos, rhs.pos, p),
					math::lerp(lhs.scale, rhs.scale, p)};
			}

		};

		using node_type = node;
		using data_type = circular_queue<node_type, false, unsigned>;
		// using DataType = std::deque<NodeType>;

		using size_type = data_type::size_type;

	protected:
		data_type points{};

	public:
		/**
		 * @return Distance between head and tail
		 */
		[[nodiscard]] float get_dst() const noexcept{
			return head().pos.dst(tail().pos);
		}

		/**
		 * @brief Not accurate, but enough
		 */
		[[nodiscard]] math::frect get_bound() const noexcept{
			return math::frect{head().pos, tail().pos};
		}

		explicit operator bool() const noexcept{
			return points.capacity() > 0;
		}

		[[nodiscard]] trail() = default;

		[[nodiscard]] explicit trail(const data_type::size_type length)
			: points(length){
		}

		[[nodiscard]] float lastScale() const noexcept{
			return head().scale;
		}

		[[nodiscard]] node_type head() const noexcept{
			return points.back_or({});
		}

		[[nodiscard]] node_type tail() const noexcept{
			return points.front_or({});
		}

		[[nodiscard]] float head_angle() const noexcept{
			const auto sz = points.size();
			if(sz < 2) return 0;

			const vec_t secondPoint = points[sz - 2].pos;
			return secondPoint.angle_to_rad(head().pos);
		}

		void push(const vec_t pos, float scale = 1.0f){
			assert(points.capacity() > 0);
			// if(lastPos.dst2(x, y) < minSpacingSqr)return;

			if(points.full()){
				points.pop_front();
			}

			points.emplace_back(pos, scale);
		}

		void pop() noexcept{
			if(!points.empty()){
				points.pop_front();
			}
		}

		data_type drop() && noexcept{
			return std::move(points);
		}

		void clear() noexcept{
			points.clear();
		}

		[[nodiscard]] auto size() const noexcept{
			return points.size();
		}

		[[nodiscard]] auto capacity() const noexcept{
			return points.capacity();
		}

		[[nodiscard]] vec_t get_head_pos() const noexcept{
			return head().pos;
		}

		template <typename Func>
		FORCE_INLINE void each(const float radius, Func consumer, float percent = 1.f) const noexcept{
			//TODO make it a co-routine to maintain better reserve and callback?
			if(points.empty()) return;
			percent = math::clamp(percent);

			float lastAngle{};
			const float capSize = static_cast<float>(this->size() - 1) * percent;

			auto drawImpl = [&](
				int index,
				const node_type prev,
				const node_type next,
				const float prevProg,
				const float nextProg

			) FORCE_INLINE -> float{
				const auto dst = next.pos - prev.pos;
				const auto scl = math::curve(dst.length(), 0.f, 0.5f) * radius * prev.scale / capSize;
				const float z2 = -(dst).angle_rad();

				const float z1 = lastAngle;

				const vec_t c = vec_t::from_polar_rad(math::pi_half - z1, scl * prevProg);
				const vec_t n = vec_t::from_polar_rad(math::pi_half - z2, scl * nextProg);

				if(n.equals({}, 0.01f) && c.equals({}, 0.01f)){
					return z2;
				}

				if constexpr(std::invocable<Func, int, vec_t, vec_t, vec_t, vec_t, float, float>){
					float progressFormer = prevProg / capSize;
					float progressLatter = nextProg / capSize;

					consumer(index, prev.pos - c, prev.pos + c, next.pos + n, next.pos - n, progressFormer, progressLatter);
				} else if constexpr(std::invocable<Func, int, vec_t, vec_t, vec_t, vec_t>){
					consumer(index, prev.pos - c, prev.pos + c, next.pos + n, next.pos - n);
				} else if constexpr(std::invocable<Func, vec_t, vec_t, vec_t, vec_t, float, float>){
					float progressFormer = prevProg / capSize;
					float progressLatter = nextProg / capSize;

					consumer(prev.pos - c, prev.pos + c, next.pos + n, next.pos - n, progressFormer, progressLatter);
				} else if constexpr(std::invocable<Func, vec_t, vec_t, vec_t, vec_t>){
					consumer(prev.pos - c, prev.pos + c, next.pos + n, next.pos - n);
				} else{
					static_assert(static_assert_trigger<Func>::value, "consumer not supported");
				}

				return z2;
			};

			const float pos = (1 - percent) * size();
			const float ceil = std::floor(pos) + 1;
			const auto initial = static_cast<size_type>(ceil);
			const auto initialFloor = initial - 1;
			const auto prog = ceil - pos;
			data_type::size_type i = initial;

			if(i >= points.size()) return;

			node_type nodeNext = points[i];
			node_type nodeCurrent = lerp(nodeNext, points[initialFloor], prog);

			lastAngle = drawImpl(0, nodeCurrent, nodeNext, 0, prog);

			for(; i < points.size() - 1; ++i){
				nodeCurrent = points[i];
				nodeNext = points[i + 1];

				if(nodeCurrent.scale <= 0.001f && nodeNext.scale <= 0.001f) continue;

				const float cur = i - initial + prog;
				lastAngle = drawImpl(i - initialFloor, nodeCurrent, nodeNext, cur, cur + 1.f);
			}
		}

		/**
		 * @tparam Func void(CapPos, radius, angle)
		 */
		template <std::invocable<vec_t, float, float> Func>
		void cap(const float width, Func consumer) const noexcept{
			if(size() > 0){
				const auto size = this->size();
				auto [pos, w] = points.back();
				w = w * width * (1.0f - 1.0f / static_cast<float>(size)) * 2.0f;
				if(w <= 0.001f) return;

				std::invoke(consumer, pos, w, head_angle());
			}
		}
	};

	export
	struct uniformed_trail : trail{
	private:
		timer<> interval{};

	public:
		using trail::trail;

		[[nodiscard]] uniformed_trail(const data_type::size_type length, const float spacing_in_tick)
			: trail(length),
			  spacing(spacing_in_tick){
		}

		float spacing{1.f};

		[[nodiscard]] float estimate_duration(float speed) const noexcept{
			return get_dst() / speed * spacing * size() * 16;
		}

		void update(const float delta_tick, const vec_t pos, float scale = 1.0f){
			//TODO when delta > spacing, fetch missing points?
			if(!spacing || interval.update_and_get(0, spacing, delta_tick)){
				push(pos, scale);
			}
		}
	};
}
