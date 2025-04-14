module;

#include <limits>

#include "../src/ext/adapted_attributes.hpp"

export module mo_yanxi.math.intersection;

export import mo_yanxi.math;
export import mo_yanxi.math.vector2;
export import mo_yanxi.math.quad;

import std;

namespace mo_yanxi::math{
	using fp_t = float;

	template <typename T>
	concept quad_like = std::is_base_of_v<quad<fp_t>, T>;
	export
	/** Returns a point on the segment nearest to the specified point. */
	[[nodiscard]] FORCE_INLINE constexpr vec2 nearest_segment_point(const vec2 where, const vec2 start, const vec2 end) noexcept{
		const auto length2 = start.dst2(end);
		if(length2 == 0) [[unlikely]]{
			 return start;
		}

		const auto t = ((where.x - start.x) * (end.x - start.x) + (where.y - start.y) * (end.y - start.y)) / length2;
		if(t <= 0) return start;
		if(t >= 1) return end;
		return {start.x + t * (end.x - start.x), start.y + t * (end.y - start.y)};
	}

	// constexpr vector2<fp_t> intersectCenterPoint(const QuadBox& subject, const QuadBox& object) {
	// 	const fp_t x0 = Math::max(subject.v0.x, object.v0.x);
	// 	const fp_t y0 = Math::max(subject.v0.y, object.v0.y);
	// 	const fp_t x1 = Math::min(subject.v1.x, object.v1.x);
	// 	const fp_t y1 = Math::min(subject.v1.y, object.v1.y);
	//
	// 	return { (x0 + x1) * 0.5f, (y0 + y1) * 0.5f };
	// }

	export
	struct possible_point{
		vector2<fp_t> pos;
		bool valid;

		constexpr explicit operator bool() const noexcept{
			return valid;
		}

		[[nodiscard]] FORCE_INLINE constexpr vector2<fp_t> value_or(vector2<fp_t> vec) const noexcept{
			return valid ? pos : vec;
		}
	};

	export
	rect_box<fp_t> wrap_striped_box(const vector2<fp_t> move, const rect_box<fp_t>& box) noexcept{
		if(move.equals({}, std::numeric_limits<fp_t>::epsilon() * 64)) {
			return box;
		}

		const auto ang = move.angle();
		auto [cos, sin] = cos_sin_deg(-ang);
		fp_t minX = std::numeric_limits<fp_t>::max();
		fp_t minY = std::numeric_limits<fp_t>::max();
		fp_t maxX = std::numeric_limits<fp_t>::lowest();
		fp_t maxY = std::numeric_limits<fp_t>::lowest();

		rect_box<fp_t> rst = box;

		for(std::size_t i = 0; i < 4; ++i){
			auto [x, y] = rst[i].rotate(cos, sin);
			minX = min(minX, x);
			minY = min(minY, y);
			maxX = max(maxX, x);
			maxY = max(maxY, y);
		}

		rst.move(move);

		for(std::size_t i = 0; i < 4; ++i){
			auto [x, y] = rst[i].rotate(cos, sin);
			minX = min(minX, x);
			minY = min(minY, y);
			maxX = max(maxX, x);
			maxY = max(maxY, y);
		}

		CHECKED_ASSUME(minX <= maxX);
		CHECKED_ASSUME(minY <= maxY);

		maxX += move.length();

		rst = rect_box<fp_t>{
			vector2<fp_t>{minX, minY}.rotate(cos, -sin),
			vector2<fp_t>{maxX, minY}.rotate(cos, -sin),
			vector2<fp_t>{maxX, maxY}.rotate(cos, -sin),
			vector2<fp_t>{minX, maxY}.rotate(cos, -sin)
		};

		return rst;
	}

	export
	FORCE_INLINE constexpr fp_t dst2_to_line(vector2<fp_t> where, const vector2<fp_t> pointOnLine, const vector2<fp_t> lineDirVec) noexcept {
		if(lineDirVec.is_zero()) return where.dst2(pointOnLine);
		where.sub(pointOnLine);
		const auto dot  = where.dot(lineDirVec);
		const auto porj = dot * dot / lineDirVec.length2();

		return where.length2() - porj;
	}


	export
	FORCE_INLINE fp_t dst_to_line(const vector2<fp_t> vec2, const vector2<fp_t> pointOnLine, const vector2<fp_t> directionVec) noexcept {
		return std::sqrt(dst2_to_line(vec2, pointOnLine, directionVec));
	}

	export
	FORCE_INLINE fp_t dst_to_line_by_seg(const vector2<fp_t> vec2, const vector2<fp_t> p1_on_line, vector2<fp_t> p2_on_line) {
		p2_on_line -= p1_on_line;
		return dst_to_line(vec2, p1_on_line, p2_on_line);
	}

	export
	FORCE_INLINE fp_t dst2_to_line_by_seg(const vector2<fp_t> vec2, const vector2<fp_t> p1_on_line, vector2<fp_t> p2_on_line) {
		p2_on_line -= p1_on_line;
		return dst2_to_line(vec2, p1_on_line, p2_on_line);
	}

	export
	FORCE_INLINE fp_t dst_to_segment(const vector2<fp_t> p, const vector2<fp_t> a, const vector2<fp_t> b) noexcept {
		const auto nearest = nearest_segment_point(p, a, b);

		return nearest.dst(p);
	}

	export
	FORCE_INLINE constexpr fp_t dst2_to_segment(const vector2<fp_t> p, const vector2<fp_t> a, const vector2<fp_t> b) noexcept {
		const auto nearest = nearest_segment_point(p, a, b);

		return nearest.dst2(p);
	}

	[[deprecated]] vector2<fp_t> arrive(const vector2<fp_t> position, const vector2<fp_t> dest, const vector2<fp_t> curVel, const fp_t smooth, const fp_t radius, const fp_t tolerance) {
		auto toTarget = vector2<fp_t>{ dest - position };

		const fp_t distance = toTarget.length();

		if(distance <= tolerance) return {};
		fp_t targetSpeed = curVel.length();
		if(distance <= radius) targetSpeed *= distance / radius;

		return toTarget.sub(curVel.x / smooth, curVel.y / smooth).limit_max_length(targetSpeed);
	}

	export
	constexpr vector2<fp_t> intersection_line(
		const vector2<fp_t> p11, const vector2<fp_t> p12,
		const vector2<fp_t> p21, const vector2<fp_t> p22) {
		const fp_t x1 = p11.x, x2 = p12.x, x3 = p21.x, x4 = p22.x;
		const fp_t y1 = p11.y, y2 = p12.y, y3 = p21.y, y4 = p22.y;

		const fp_t dx1 = x1 - x2;
		const fp_t dy1 = y1 - y2;

		const fp_t dx2 = x3 - x4;
		const fp_t dy2 = y3 - y4;

		const fp_t det = dx1 * dy2 - dy1 * dx2;

		if (det == 0.0f) {
			return vector2<fp_t>{(x1 + x2) * 0.5f, (y1 + y2) * 0.5f};  // Return the midpoint of overlapping lines
		}

		const fp_t pre = x1 * y2 - y1 * x2, post = x3 * y4 - y3 * x4;
		const fp_t x   = (pre * dx2 - dx1 * post) / det;
		const fp_t y   = (pre * dy2 - dy1 * post) / det;

		return vector2<fp_t>{x, y};
	}

	// constexpr auto i = intersectionLine({-1, 0}, {1, 0}, {0, -1}, {0, 1});

	 [[deprecated]] std::optional<vector2<fp_t>>
	intersect_segments(const vector2<fp_t> p1, const vector2<fp_t> p2, const vector2<fp_t> p3, const vector2<fp_t> p4){
		const fp_t x1 = p1.x, y1 = p1.y, x2 = p2.x, y2 = p2.y, x3 = p3.x, y3 = p3.y, x4 = p4.x, y4 = p4.y;

		const fp_t dx1 = x2 - x1;
		const fp_t dy1 = y2 - y1;

		const fp_t dx2 = x4 - x3;
		const fp_t dy2 = y4 - y3;

		const fp_t d = dy2 * dx1 - dx2 * dy1;
		if(d == 0) return std::nullopt;

		const fp_t yd = y1 - y3;
		const fp_t xd = x1 - x3;

		const fp_t ua = (dx2 * yd - dy2 * xd) / d;
		if(ua < 0 || ua > 1) return std::nullopt;

		const fp_t ub = (dx1 * yd - dy1 * xd) / d;
		if(ub < 0 || ub > 1) return std::nullopt;

		return vector2<fp_t>{x1 + dx1 * ua, y1 + dy1 * ua};
	}

	export
	FORCE_INLINE constexpr bool
	intersect_segments(const vector2<fp_t> p1, const vector2<fp_t> p2, const vector2<fp_t> p3, const vector2<fp_t> p4, vector2<fp_t>& out) noexcept{
		const fp_t x1 = p1.x, y1 = p1.y, x2 = p2.x, y2 = p2.y, x3 = p3.x, y3 = p3.y, x4 = p4.x, y4 = p4.y;

		const fp_t dx1 = x2 - x1;
		const fp_t dy1 = y2 - y1;

		const fp_t dx2 = x4 - x3;
		const fp_t dy2 = y4 - y3;

		const fp_t d = dy2 * dx1 - dx2 * dy1;
		if(d == 0) return false;

		const fp_t yd = y1 - y3;
		const fp_t xd = x1 - x3;
		const fp_t ua = (dx2 * yd - dy2 * xd) / d;
		if(ua < 0 || ua > 1) return false;

		const fp_t ub = (dx1 * yd - dy1 * xd) / d;
		if(ub < 0 || ub > 1) return false;

		out.set(x1 + dx1 * ua, y1 + dy1 * ua);
		return true;
	}

	export
	template <quad_like T>
		// requires requires(const T& t){
		// 	{ t[int{}] } -> std::convertible_to<vector2<fp_t>>;
		// }
	[[nodiscard]] constexpr vector2<fp_t> nearest_edge_normal(const T& rectangle, const vector2<fp_t> p) noexcept {
		fp_t minDistance = std::numeric_limits<fp_t>::max();
		vector2<fp_t> closestEdgeNormal{};

		for (int i = 0; i < 4; i++) {
			auto a = rectangle[i];
			auto b = rectangle[i + 1];

			const fp_t d = ::mo_yanxi::math::dst2_to_segment(p, a, b);

			if (d < minDistance) {
				minDistance = d;
				closestEdgeNormal = rectangle.edge_normal_at(i);
			}
		}

		return closestEdgeNormal;
	}

	struct weighted_vector{
		fp_t weight;
		vector2<fp_t> normal;
	};

	/**
	 * @warning Return Value Is NOT Normalized!
	 */
	export
	template <quad_like T>
	[[nodiscard]] constexpr vector2<fp_t> avg_edge_normal(const T& quad, const vector2<fp_t> where) noexcept {

		std::array<weighted_vector, 4> normals{};

		for (int i = 0; i < 4; i++) {
			const vector2<fp_t> va = quad[i];
			const vector2<fp_t> vb = quad[i + 1];

			normals[i].weight = math::dst_to_segment(where, va, vb) * va.dst(vb);
			normals[i].normal = quad.edge_normal_at(i);
		}

		const fp_t total = (normals[0].weight + normals[1].weight + normals[2].weight + normals[3].weight);
		assert(total != 0.f);

		vector2<fp_t> closestEdgeNormal{};

		for(const auto& [weight, normal] : normals) {
			closestEdgeNormal.sub(normal * math::pow_integral<16>(weight / total));
		}

		assert(!closestEdgeNormal.is_NaN());
		if(closestEdgeNormal.is_zero()) [[unlikely]] {
			closestEdgeNormal = -std::ranges::max_element(normals, {}, &weighted_vector::weight)->normal;
		}

		return closestEdgeNormal;
	}

	// struct quad_intersection_result{
	// 	struct info{
	// 		vector2<fp_t> pos;
	// 		unsigned short edgeSbj;
	// 		unsigned short edgeObj;
	//
	// 		static bool onSameEdge(const unsigned short edgeIdxN_1, const unsigned short edgeIdxN_2) noexcept {
	// 			return edgeIdxN_1 == edgeIdxN_2;
	// 		}
	//
	// 		static bool onNearEdge(const unsigned short edgeIdxN_1, const unsigned short edgeIdxN_2) noexcept {
	// 			return (edgeIdxN_1 + 1) % 4 == edgeIdxN_2 || (edgeIdxN_2 + 1) % 4 == edgeIdxN_1;
	// 		}
	//
	// 	};
	// 	array_stack<info, 4, unsigned> points{};
	//
	// 	explicit operator bool() const noexcept{
	// 		return !points.empty();
	// 	}
	//
	// 	[[nodiscard]] constexpr vector2<fp_t> avg() const noexcept{
	// 		assert(!points.empty());
	//
	// 		vector2<fp_t> rst{};
	// 		for (const auto & intersection : points){
	// 			rst += intersection.pos;
	// 		}
	//
	// 		return rst / static_cast<fp_t>(points.size());
	// 	}
	// };

	// export
	// template <quad_like L, quad_like R>
	// auto rectExactAvgIntersection(const L& subjectBox, const R& objectBox) noexcept {
	// 	quad_intersection_result result{};
	//
	// 	vector2<fp_t> rst{};
	// 	for(unsigned i = 0; i < 4; ++i) {
	// 		for(unsigned j = 0; j < 4; ++j) {
	// 			if(intersect_segments(
	// 				subjectBox[i], subjectBox[(i + 1) % 4],
	// 				objectBox[j], objectBox[(j + 1) % 4],
	// 				rst)
	// 			) {
	// 				if(!result.points.full()){
	// 					result.points.push(quad_intersection_result::info{rst, static_cast<unsigned short>(i), static_cast<unsigned short>(j)});
	// 				}else{
	// 					return result;
	// 				}
	// 			}
	// 		}
	// 	}
	//
	// 	return result;
	// }


	export
	template <quad_like L, quad_like R>
	possible_point rect_rough_avg_intersection(const L& subject, const R& object) noexcept {
		vector2<fp_t> intersections{};
		unsigned count = 0;

		vector2<fp_t> rst{};
		for(int i = 0; i < 4; ++i) {
			for(int j = 0; j < 4; ++j) {
				if(math::intersect_segments(subject[i], subject[(i + 1)], object[j], object[(j + 1)], rst)) {
					count++;
					intersections += rst;
				}
			}
		}

		if(count > 0) {
			return {intersections.div(static_cast<fp_t>(count)), true};
		}

		return {};
	}


	// OrthoRectFloat maxContinousBoundOf(const std::vector<QuadBox>& traces) {
	// 	const auto& front = traces.front().getMaxOrthoBound();
	// 	const auto& back  = traces.back().getMaxOrthoBound();
	// 	auto [minX, maxX] = std::minmax({ front.getSrcX(), front.getEndX(), back.getSrcX(), back.getEndX() });
	// 	auto [minY, maxY] = std::minmax({ front.getSrcY(), front.getEndY(), back.getSrcY(), back.getEndY() });
	//
	// 	return OrthoRectFloat{ minX, minY, maxX - minX, maxY - minY };
	// }
	//
	// template <ext::number T>
	// bool overlap(const T x, const T y, const T radius, const Rect_Orthogonal<T>& rect) {
	// 	T closestX = std::clamp<T>(x, rect.getSrcX(), rect.getEndX());
	// 	T closestY = std::clamp<T>(y, rect.getSrcY(), rect.getEndY());
	//
	// 	T distanceX       = x - closestX;
	// 	T distanceY       = y - closestY;
	// 	T distanceSquared = distanceX * distanceX + distanceY * distanceY;
	//
	// 	return distanceSquared <= radius * radius;
	// }

	// template <ext::number T>
	// bool overlap(const Circle<T>& circle, const Rect_Orthogonal<T>& rect) {
	// 	return Geom::overlap<T>(circle.getCX(), circle.getCY(), circle.getRadius(), rect);
	// }
}
