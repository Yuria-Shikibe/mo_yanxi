module;

#include "../src/ext/adapted_attributes.hpp"

export module mo_yanxi.math.quad;

export import mo_yanxi.math.vector2;
export import mo_yanxi.math.rect_ortho;
export import mo_yanxi.math.trans2;
export import mo_yanxi.math;

import std;

namespace mo_yanxi::math{
	template <typename T>
	struct overlay_axis_keys{
		static constexpr std::size_t count = 0;

		template <std::size_t Idx>
		static constexpr vec2 get(const T&) noexcept = delete;
	};

	template <typename T>
	struct overlay_axis_keys<rect_ortho<T>>{
		static constexpr std::size_t count = 2;

		template <std::size_t Idx>
		FORCE_INLINE static constexpr auto get(const rect_ortho<T>&) noexcept{
			if constexpr (Idx == 0){
				return vectors::constant2<T>::x_vec2;
			}else{
				return vectors::constant2<T>::y_vec2;
			}
		}
	};

	template <std::size_t Idx, typename L, typename R>
	FORCE_INLINE constexpr vec2 get_axis(const L& l, const R& r) noexcept {
		using LK = overlay_axis_keys<L>;
		using RK = overlay_axis_keys<R>;

		static_assert(LK::count + RK::count >= Idx, "Idx Out Of Bound");

		if constexpr (Idx < LK::count){
			return LK::template get<Idx>(l);
		}else{
			return RK::template get<Idx - LK::count>(r);
		}
	}

	template <typename L, typename R>
	FORCE_INLINE constexpr vec2 get_axis(std::size_t Idx, const L& l, const R& r) noexcept {
		using LK = overlay_axis_keys<L>;
		using RK = overlay_axis_keys<R>;

		static_assert(LK::count + RK::count >= Idx, "Idx Out Of Bound");

		if (Idx < LK::count){
			return LK::template get<Idx>(l);
		}else{
			return RK::template get<Idx - LK::count>(r);
		}
	}


	/**
	 * \brief Mainly Used For Continous Collidsion Test
	 * @code
	 * v3 +-----+ v2
	 *    |     |
	 *    |     |
	 * v0 +-----+ v1
	 * @endcode
	 */
	export
	template <typename T>
	struct quad{
		using value_type = T;
		using vec_t = vector2<value_type>;
		using rect_t = rect_ortho<value_type>;

		vec_t v0; //v00
		vec_t v1; //v10
		vec_t v2; //v11
		vec_t v3; //v01

		template <std::integral Idx>
		FORCE_INLINE constexpr vec_t& operator[](const Idx idx) noexcept{
			switch (idx & static_cast<Idx>(4)){
				case 0: return v0;
				case 1: return v1;
				case 2: return v2;
				case 3: return v3;
				default: std::unreachable();
			}
		}

		template <std::integral Idx>
		FORCE_INLINE constexpr vec_t operator[](const Idx idx) const noexcept{
			switch (idx & static_cast<Idx>(4)){
				case 0: return v0;
				case 1: return v1;
				case 2: return v2;
				case 3: return v3;
				default: std::unreachable();
			}
		}

		constexpr friend bool operator==(const quad& lhs, const quad& rhs) noexcept = default;

		FORCE_INLINE constexpr void move(typename vec_t::const_pass_t vec) noexcept{
			v0 += vec;
			v1 += vec;
			v2 += vec;
			v3 += vec;
		}

		[[nodiscard]] FORCE_INLINE constexpr rect_t get_bound() const noexcept{
			auto [xmin, xmax] = math::minmax(v0.x, v1.x, v2.x, v3.x);
			auto [ymin, ymax] = math::minmax(v0.y, v1.y, v2.y, v3.y);
			return rect_ortho<T>{tags::unchecked, vec_t{xmin, ymin}, vec_t{xmax, ymax}};
		}

		[[nodiscard]] FORCE_INLINE constexpr vec_t avg() const noexcept{
			return (v0 + v1 + v2 + v3) / static_cast<value_type>(4);
		}

		template <std::derived_from<quad> S, std::derived_from<quad> O>
		[[nodiscard]] FORCE_INLINE constexpr bool overlap_rough(this const S& self, const O& o) noexcept{
			return self.get_bound().overlap_inclusive(o.get_bound());
		}

		template <std::derived_from<quad> S>
		[[nodiscard]] FORCE_INLINE constexpr bool overlap_rough(this const S& self, const rect_t& other) noexcept{
			return self.get_bound().overlap_inclusive(other);
		}

		constexpr void expand(const T length) noexcept{
			auto center = avg();

			v0 += (v0 - center).set_length(length);
			v1 += (v1 - center).set_length(length);
			v2 += (v2 - center).set_length(length);
			v3 += (v3 - center).set_length(length);
		}

		/**
		 * @warning Not Normalized
		 * @return normal on edge[idx, idx + 1], pointing to outside
		 */
		template <std::derived_from<quad> S, std::integral Idx>
		[[nodiscard]] FORCE_INLINE constexpr vec_t edge_normal_at(this const S& self, const Idx edgeIndex) noexcept{
			const auto begin = self[edgeIndex];
			const auto end   = self[edgeIndex + 1];

			return (end - begin).rotate_rt_clockwise();
		}

		template <std::derived_from<quad> S, std::derived_from<quad> O>
		[[nodiscard]] FORCE_INLINE constexpr value_type axis_proj_dst(this const S& sbj, const O& obj, const vec_t axis) noexcept{
			const auto [min1, max1] = math::minmax(sbj[0].dot(axis), sbj[1].dot(axis), sbj[2].dot(axis), sbj[3].dot(axis));
			const auto [min2, max2] = math::minmax(obj[0].dot(axis), obj[1].dot(axis), obj[2].dot(axis), obj[3].dot(axis));

			const auto overlap_min = math::max(min1, min2);
			const auto overlap_max = math::min(max1, max2);
			return overlap_max - overlap_min;
		}

		template <std::derived_from<quad> S, std::derived_from<quad> O>
		[[nodiscard]] FORCE_INLINE constexpr bool axis_proj_overlaps(this const S& sbj, const O& obj, const vec_t axis) noexcept{
			const auto [min1, max1] = math::minmax(sbj[0].dot(axis), sbj[1].dot(axis), sbj[2].dot(axis), sbj[3].dot(axis));
			const auto [min2, max2] = math::minmax(obj[0].dot(axis), obj[1].dot(axis), obj[2].dot(axis), obj[3].dot(axis));

			return max1 >= min2 && max2 >= min1;
		}

		template <std::derived_from<quad> S, typename O, typename ...Args>
			requires (std::convertible_to<Args, vec_t> && ...)
		[[nodiscard]] FORCE_INLINE bool overlap_exact(this const S& sbj, const O& obj,const Args&... args) noexcept{
			static_assert(sizeof...(Args) > 0 && sizeof...(Args) <= 8);

			return
				(sbj.axis_proj_overlaps(obj, args) && ...);
		}

		template <std::derived_from<quad> S, typename O>
			requires requires{
				requires overlay_axis_keys<S>::count > 0;
				requires overlay_axis_keys<O>::count > 0;
			}
		[[nodiscard]] FORCE_INLINE bool overlap_exact(this const S& sbj, const O& obj) noexcept{
			using SbjKeys = overlay_axis_keys<S>;
			using ObjKeys = overlay_axis_keys<O>;

			return [&] <std::size_t... Idx> (std::index_sequence<Idx...>) noexcept {
				return sbj.overlap_exact(obj, math::get_axis<Idx>(sbj, obj) ...);
			}(std::make_index_sequence<SbjKeys::count + ObjKeys::count>{});
		}

		template <std::derived_from<quad> S, typename O>
			requires requires{
				requires overlay_axis_keys<S>::count > 0;
				requires overlay_axis_keys<O>::count > 0;
			}
		[[nodiscard]] FORCE_INLINE vec_t overlap_distance(this const S& sbj, const O& obj) noexcept{
			using SbjKeys = overlay_axis_keys<S>;
			using ObjKeys = overlay_axis_keys<O>;

			std::size_t index{std::numeric_limits<std::size_t>::max()};
			value_type value{std::numeric_limits<value_type>::max()};

			auto update = [&](const value_type dst, const std::size_t Idx){
				if(dst > 0 && dst < value){
					value = dst;
					index = Idx;
				}
			};

			[&] <std::size_t... Idx> (std::index_sequence<Idx...>) noexcept {
				(update(sbj.axis_proj_dst(obj, math::get_axis<Idx>(sbj, obj)), Idx), ...);
			}(std::make_index_sequence<SbjKeys::count + ObjKeys::count>{});

			if(index == std::numeric_limits<std::size_t>::max()){
				return vectors::constant2<value_type>::SNaN;
			}else{
				auto approach = math::get_axis(index, sbj, obj).set_length(value);

				if constexpr (SbjKeys::count + ObjKeys::count == 8){
					return approach;
				}else{
					auto avgSbj = sbj.avg();
					auto avgObj = obj.avg();

					if(approach.dot(avgObj - avgSbj) > 0){
						return -approach;
					}

					return approach;
				}

			}
		}

		[[nodiscard]] constexpr bool contains(const vec_t point) const noexcept{
			if(!get_bound().contains_loose(point)){
				return false;
			}

			bool oddNodes = false;

			for(int i = 0; i < 4; ++i){
				const auto vertice = this->operator[](i);
				const auto lastVertice = this->operator[](i + 1);
				if(
					(vertice.y < point.y && lastVertice.y >= point.y) ||
					(lastVertice.y < point.y && vertice.y >= point.y)){
					if(vertice.x + (point.y - vertice.y) * (lastVertice - vertice).slope_inv() < point.x){
						oddNodes = !oddNodes;
					}
				}
			}

			return oddNodes;
		}

	};

	export using fquad = quad<float>;

	template <typename T>
	struct overlay_axis_keys<quad<T>>{
		static constexpr std::size_t count = 4;

		template <std::size_t Idx>
		FORCE_INLINE static constexpr auto get(const quad<T>& quad) noexcept{
			return quad.edge_normal_at(Idx);
		}
	};

	export
	template <typename T>
	constexpr quad<T> rect_ortho_to_quad(
		const rect_ortho<T>& rect, typename rect_ortho<T>::vec_t::const_pass_t center, float cos, float sin) noexcept{
		return quad<T>{
			center + (rect.vert_00() - center).rotate(cos, sin),
			center + (rect.vert_10() - center).rotate(cos, sin),
			center + (rect.vert_11() - center).rotate(cos, sin),
			center + (rect.vert_01() - center).rotate(cos, sin),
		};
	}


	//TODO redesign the invariants of the class?
	template <typename T>
	struct quad_bounded : protected quad<T>{
		using base = quad<T>;

	protected:
		typename base::rect_t bounding_box{};

	public:
		[[nodiscard]] quad_bounded() = default;

		[[nodiscard]] explicit(false) quad_bounded(const base& base)
			: base{base}, bounding_box{base::get_bound()}{
		}

		[[nodiscard]] explicit(false) quad_bounded(typename base::rect_t& rect)
			: base{rect.vert_00(), rect.vert_10(), rect.vert_11(), rect.vert_01()}, bounding_box{rect}{
		}

		[[nodiscard]] constexpr FORCE_INLINE typename base::rect_t get_bound() const noexcept{
			return bounding_box;
		}

		FORCE_INLINE constexpr void move(typename base::vec_t::const_pass_t vec) noexcept{
			base::move(vec);

			bounding_box.src += vec;
		}

		constexpr void expand(const T length) noexcept{
			base::expand(length);

			bounding_box = base::get_bound();
		}

		constexpr explicit(false) operator base() const noexcept{
			return *this;
		}

		using base::operator[];

		using base::avg;
		using base::axis_proj_dst;
		using base::axis_proj_overlaps;
		using base::contains;
		using base::expand;
		using base::edge_normal_at;
		using base::move;
		using base::overlap_rough;
		using base::overlap_exact;
	};

	template <typename T>
	struct rect_box_identity{
		/**
		 * \brief x for rect width, y for rect height, static
		 */
		vector2<T> size;

		/**
		 * \brief Center To Bottom-Left Offset
		 */
		vector2<T> offset;

	};

	template <typename T>
	struct rect_box : quad_bounded<T>{
		using base = quad_bounded<T>;
	protected:
		/**
		 * \brief
		 * Normal Vector for v0-v1, v2-v3
		 * Edge Vector for v1-v2, v3-v0
		 */
		typename base::vec_t normalU{};

		/**
		 * \brief
		 * Normal Vector for v1-v2, v3-v0
		 * Edge Vector for v0-v1, v2-v3
		 */
		typename base::vec_t normalV{};

		constexpr void updateNormal() noexcept{
			normalU = (this->v0 - this->v3)/*.normalize()*/;
			normalV = (this->v1 - this->v2)/*.normalize()*/;
		}

	public:
		[[nodiscard]] rect_box() = default;

		[[nodiscard]] explicit rect_box(const base& base)
			: base(base), normalU(this->v0 - this->v3), normalV(this->v1 - this->v2){
		}
		[[nodiscard]] explicit rect_box(const typename base::rect_t& rect)
			: base(rect), normalU(this->v0 - this->v3), normalV(this->v1 - this->v2){
		}

		// [[nodiscard]] RectBoxBrief() = default;
		//
		// [[nodiscard]] constexpr RectBoxBrief(const Vec2 v0, const Vec2 v1, const Vec2 v2, const Vec2 v3) noexcept
		// 	: QuadBox{v0, v1, v2, v3}{
		// 	updateNormal();
		// }
		//
		// [[nodiscard]] explicit(false) constexpr RectBoxBrief(const OrthoRectFloat rect) noexcept
		// 	: QuadBox{rect}{
		// 	updateNormal();
		// 	}

		// [[nodiscard]] constexpr Vec2 getNormalU() const noexcept{
		// 	return normalU;
		// }
		//
		// [[nodiscard]] constexpr Vec2 getNormalV() const noexcept{
		// 	return normalV;
		// }

		// [[nodiscard]] constexpr float projLen2(const Vec2 axis) const noexcept{
		// 	const Vec2 diagonal = v2 - v0;
		// 	const float dot = diagonal.dot(axis);
		// 	return dot * dot / axis.length2();
		// }
		//
		// [[nodiscard]] float projLen(const Vec2 axis) const noexcept {
		// 	return std::sqrt(projLen2(axis));
		// }

		typename base::vec_t edge_normal_at(const std::integral auto idx) const noexcept{
			switch(idx & 4) {
				case 0 : return -normalU;
				case 1 : return normalV;
				case 2 : return normalU;
				case 3 : return -normalV;
				default: std::unreachable();
			}
		}
	};


	template <typename T>
	struct overlay_axis_keys<rect_box<T>>{
		static constexpr std::size_t count = 2;

		template <std::size_t Idx>
		FORCE_INLINE static constexpr auto get(const rect_box<T>& quad) noexcept{
			return quad.edge_normal_at(Idx);
		}
	};

	struct rect_box_posed : rect_box<float>, rect_box_identity<float>{
		using idt_t = rect_box_identity<float>;
		using base = rect_box<float>;
		using trans_t = trans2;
		/**
		 * \brief Box Origin Point
		 * Should Be Mass Center if possible!
		 */
		trans_t transform{};

		[[nodiscard]] constexpr rect_box_posed() = default;

		[[nodiscard]] explicit rect_box_posed(const idt_t& idt, const trans_t transform = {})
			: rect_box_identity{idt}{
			update(transform);
		}

		[[nodiscard]] explicit rect_box_posed(const typename base::vec_t size, const trans_t transform = {})
			: rect_box_posed{{size, size / -2.f}, transform}{
		}

		constexpr rect_box_posed& copy_identity_from(const rect_box_posed& other) noexcept{
			this->idt_t::operator=(other);
			transform = other.transform;

			return *this;
		}
		//
		// /**
		//  *	TODO move this to other place?
		//  * \param mass
		//  * \param scale Manually assign a correction scale
		//  * \param lengthRadiusRatio to decide the R(radius) scale for simple calculation
		//  * \brief From: [mr^2/4 + ml^2 / 12]
		//  * \return Rotational Inertia Estimation
		//  */
		[[nodiscard]] constexpr float get_rotational_inertia(const float mass, const float scale = 1 / 12.0f, const float lengthRadiusRatio = 0.25f) const noexcept {
			return size.length2() * (scale + lengthRadiusRatio) * mass;
		}


		constexpr void update(const trans_t transform) noexcept{
			typename base::value_type rot = transform.rot;
			//
			float cos = math::cos_deg(rot);
			float sin = math::sin_deg(rot);

			v0.set(offset).rotate(cos, sin);
			v1.set(size.x, 0).rotate(cos, sin);
			v3.set(0, size.y).rotate(cos, sin);
			v2 = v1 + v3;

			if(this->transform.rot != transform.rot){
				normalU = v3;
				normalV = v1;

				this->transform.rot = transform.rot;
			}

			v0 += transform.vec;
			v1 += v0;
			v2 += v0;
			v3 += v0;


			bounding_box = get_bound();
		}
	};

}
