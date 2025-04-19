module;

#include "../src/ext/adapted_attributes.hpp"
#include <immintrin.h>

export module mo_yanxi.math.quad;

export import mo_yanxi.math.vector2;
export import mo_yanxi.math.rect_ortho;
export import mo_yanxi.math.trans2;
export import mo_yanxi.math.intersection;
export import mo_yanxi.math;

import std;

namespace mo_yanxi::math{
	template<bool IsMin>
	FORCE_INLINE float horizontal_extreme(__m128 v) noexcept {
		__m128 v1 = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2,3,0,1)); // swap 0-1, 2-3
		__m128 m1 = IsMin ? _mm_min_ps(v, v1) : _mm_max_ps(v, v1);
		__m128 m2 = _mm_shuffle_ps(m1, m1, _MM_SHUFFLE(1,0,3,2)); // swap pairs
		__m128 res = IsMin ? _mm_min_ps(m1, m2) : _mm_max_ps(m1, m2);
		return _mm_cvtss_f32(_mm_shuffle_ps(res, res, _MM_SHUFFLE(0,0,0,0)));
	}

	template <typename T = unsigned>
	constexpr inline T vertex_mask{0b11};
	template <typename D, typename Base>
	concept any_derived_from = std::is_base_of_v<Base, D>;

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

		auto a = Idx;

		if constexpr (Idx < LK::count){
			return LK::template get<Idx>(l);
		}else{
			return RK::template get<Idx - LK::count>(r);
		}
	}
	//
	template <typename L, typename R>
	FORCE_INLINE constexpr vec2 get_axis(std::size_t Idx, const L& l, const R& r) noexcept {
		using LK = overlay_axis_keys<L>;
		using RK = overlay_axis_keys<R>;

		if (Idx < LK::count){
			return l.edge_normal_at(Idx);
		}else{
			return r.edge_normal_at(Idx - LK::count);
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
			switch (idx & vertex_mask<Idx>){
				case 0: return v0;
				case 1: return v1;
				case 2: return v2;
				case 3: return v3;
				default: std::unreachable();
			}
		}

		template <std::integral Idx>
		FORCE_INLINE constexpr vec_t operator[](const Idx idx) const noexcept{
			switch (idx & vertex_mask<Idx>){
				case 0: return v0;
				case 1: return v1;
				case 2: return v2;
				case 3: return v3;
				default: std::unreachable();
			}
		}

		constexpr friend bool operator==(const quad& lhs, const quad& rhs) noexcept = default;

		FORCE_INLINE constexpr void move(typename vec_t::const_pass_t vec) noexcept{
			if constexpr (std::same_as<float, value_type>){
				if (!std::is_constant_evaluated()){
					/* 创建重复的vec.x/vec.y模式 */
					const __m256 vec_x = ::_mm256_broadcast_ss(&vec.x); // [x,x,x,x,x,x,x,x]
					const __m256 vec_y = ::_mm256_broadcast_ss(&vec.y); // [y,y,y,y,y,y,y,y]

					// 通过解包操作生成[x,y,x,y,x,y,x,y]模式
					const __m256 vec_pattern = _mm256_unpacklo_ps(vec_x, vec_y);

					/* 一次性加载全部4个vec_t（8个float） */
					auto base_ptr = reinterpret_cast<float*>(this);
					__m256 data = _mm256_loadu_ps(base_ptr);

					/* 执行向量加法 */
					data = _mm256_add_ps(data, vec_pattern);

					/* 存回内存 */
					_mm256_storeu_ps(base_ptr, data);
					return;
				}
			}


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

		template <any_derived_from<quad> S, any_derived_from<quad> O>
		[[nodiscard]] FORCE_INLINE constexpr bool overlap_rough(this const S& self, const O& o) noexcept{
			return self.get_bound().overlap_inclusive(o.get_bound());
		}

		template <any_derived_from<quad> S>
		[[nodiscard]] FORCE_INLINE constexpr bool overlap_rough(this const S& self, const rect_t& other) noexcept{
			return self.get_bound().overlap_inclusive(other);
		}

		constexpr vec_t expand(const T length) noexcept{
			auto center = avg();

			v0 += (v0 - center).set_length(length);
			v1 += (v1 - center).set_length(length);
			v2 += (v2 - center).set_length(length);
			v3 += (v3 - center).set_length(length);

			return center;
		}

		/**
		 * @return normal on edge[idx, idx + 1], pointing to outside
		 */
		template <any_derived_from<quad> S, std::integral Idx>
		[[nodiscard]] FORCE_INLINE constexpr vec_t edge_normal_at(this const S& self, const Idx edgeIndex) noexcept{
			const auto begin = self[edgeIndex];
			const auto end   = self[edgeIndex + 1];

			return static_cast<vec_t&>((end - begin).rotate_rt_clockwise()).normalize();
		}

		template <any_derived_from<quad> S, any_derived_from<quad> O>
		[[nodiscard]] FORCE_INLINE constexpr value_type axis_proj_dst(this const S& sbj, const O& obj, const vec_t axis) noexcept{
			const auto [min1, max1] = math::minmax(sbj[0].dot(axis), sbj[1].dot(axis), sbj[2].dot(axis), sbj[3].dot(axis));
			const auto [min2, max2] = math::minmax(obj[0].dot(axis), obj[1].dot(axis), obj[2].dot(axis), obj[3].dot(axis));

			const auto overlap_min = math::max(min1, min2);
			const auto overlap_max = math::min(max1, max2);
			return overlap_max - overlap_min;
		}

		template <any_derived_from<quad> S, typename O>
		[[nodiscard]] FORCE_INLINE constexpr bool axis_proj_overlaps(this const S& sbj, const O& obj, const vec_t axis) noexcept{
			if constexpr (std::same_as<value_type, float> && any_derived_from<O, quad>){
				if (!std::is_constant_evaluated()){
					static constexpr std::int32_t PERMUTE_IDX[8] = {0, 2, 4, 6, 1, 3, 5, 7};
					// 加载并重组数据
					const __m256i idx = _mm256_load_si256(reinterpret_cast<const __m256i*>(PERMUTE_IDX));

					// 同时加载sbj和obj数据
					const __m256 sbj_data = _mm256_loadu_ps(reinterpret_cast<const float*>(&sbj));
					const __m256 obj_data = _mm256_loadu_ps(reinterpret_cast<const float*>(&obj));

					// 构造axis向量
					const __m256 axis_xy = _mm256_set_m128(
						_mm_set1_ps(axis.y),  // 高128位: yyyyyyyy
						_mm_set1_ps(axis.x)   // 低128位: xxxxxxxx
					);

					// 重组并计算点积
					auto compute_proj = [&](__m256 data) -> __m128 {
						const __m256 permuted = _mm256_permutexvar_ps(idx, data);
						const __m256 products = _mm256_mul_ps(permuted, axis_xy);
						return _mm_add_ps(_mm256_extractf128_ps(products, 0),
										 _mm256_extractf128_ps(products, 1));
					};

					// 并行计算两个对象的投影
					const __m128 sbj_dots = compute_proj(sbj_data);
					const __m128 obj_dots = compute_proj(obj_data);

					// 快速极值计算
					const float min1 = horizontal_extreme<true>(sbj_dots);
					const float max1 = horizontal_extreme<false>(sbj_dots);
					const float min2 = horizontal_extreme<true>(obj_dots);
					const float max2 = horizontal_extreme<false>(obj_dots);
					// 检查重叠
					return (max1 >= min2) && (max2 >= min1);
				}
			}
			const auto [min1, max1] = math::minmax(sbj[0].dot(axis), sbj[1].dot(axis), sbj[2].dot(axis), sbj[3].dot(axis));
			const auto [min2, max2] = math::minmax(obj[0].dot(axis), obj[1].dot(axis), obj[2].dot(axis), obj[3].dot(axis));

			return max1 >= min2 && max2 >= min1;
		}

		template <any_derived_from<quad> S, typename O, typename ...Args>
			requires (std::convertible_to<Args, vec_t> && ...) && (sizeof...(Args) > 0)
		[[nodiscard]] FORCE_INLINE bool overlap_exact(this const S& sbj, const O& obj,const Args&... args) noexcept{
			static_assert(sizeof...(Args) > 0 && sizeof...(Args) <= 8);

			return
				(sbj.axis_proj_overlaps(obj, args) && ...);
		}

		template <any_derived_from<quad> S, typename O>
			requires requires{
				requires overlay_axis_keys<S>::count > 0;
				requires overlay_axis_keys<O>::count > 0;
			}
		[[nodiscard]] constexpr FORCE_INLINE bool overlap_exact(this const S& sbj, const O& obj) noexcept{
			using SbjKeys = overlay_axis_keys<S>;
			using ObjKeys = overlay_axis_keys<O>;

			return [&] <std::size_t... Idx> (std::index_sequence<Idx...>) noexcept {
				return sbj.overlap_exact(obj, math::get_axis<Idx>(sbj, obj) ...);
			}(std::make_index_sequence<SbjKeys::count + ObjKeys::count>{});
		}

		template <any_derived_from<quad> S, any_derived_from<quad> O>
			requires requires{
				requires overlay_axis_keys<S>::count > 0;
				requires overlay_axis_keys<O>::count > 0;
			}
		[[nodiscard]] FORCE_INLINE vec_t depart_vector_to(this const S& sbj, const O& obj) noexcept{
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
				return vectors::constant2<value_type>::zero_vec2;
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

		template <any_derived_from<quad> S, any_derived_from<quad> O>
			requires requires{
				requires overlay_axis_keys<S>::count > 0;
				requires overlay_axis_keys<O>::count > 0;
			}
		[[nodiscard]] FORCE_INLINE vec_t depart_vector_to_on(this const S& sbj, const O& obj, vec2 line_to_align) noexcept{
			line_to_align.normalize();

			auto proj = sbj.axis_proj_dst(obj, line_to_align);
			auto vec = line_to_align * proj;

			auto approach = obj.avg() - sbj.avg();
			if(vec.dot(approach) >= 0){
				return -vec;
			}else{
				return vec;
			}
		}

		template <any_derived_from<quad> S, any_derived_from<quad> O>
			requires requires{
				requires overlay_axis_keys<S>::count > 0;
				requires overlay_axis_keys<O>::count > 0;
			}
		[[nodiscard]] FORCE_INLINE vec_t depart_vector_to_on_vel(this const S& sbj, const O& obj, vec_t v) noexcept{
			v.normalize();

			const auto [min_sbj, max_sbj] = math::minmax(sbj[0].dot(v), sbj[1].dot(v), sbj[2].dot(v), sbj[3].dot(v));
			const auto [min_obj, max_obj] = math::minmax(obj[0].dot(v), obj[1].dot(v), obj[2].dot(v), obj[3].dot(v));

			if(max_obj > min_sbj && min_obj < max_sbj){
				return v * (min_obj - max_sbj);
			}

			if(max_sbj > min_obj && min_sbj < max_obj){
				return v * (max_obj - min_sbj);
			}

			return {};
		}

		template <any_derived_from<quad> S, any_derived_from<quad> O>
			requires requires{
				requires overlay_axis_keys<S>::count > 0;
				requires overlay_axis_keys<O>::count > 0;
			}
		[[nodiscard]] FORCE_INLINE vec_t depart_vector_to_on_vel_rough_min(this const S& sbj, const O& obj, vec_t v) noexcept{
			const auto depart_vector_on_vec = sbj.depart_vector_to_on_vel(obj, v);
			const auto depart_vector = sbj.depart_vector_to(obj);

			if(depart_vector.equals({}) || depart_vector_on_vec.equals({}))return {};

			return math::lerp(depart_vector_on_vec, depart_vector,
				std::sqrt(math::curve(depart_vector_on_vec.copy().normalize().dot(depart_vector.copy().normalize()), -0.995f, 1.25f))
			);

		}

		template <any_derived_from<quad> S, any_derived_from<quad> O>
			requires requires{
				requires overlay_axis_keys<S>::count > 0;
				requires overlay_axis_keys<O>::count > 0;
			}
		[[nodiscard]] FORCE_INLINE vec_t depart_vector_to_on_vel_exact(this const S& sbj, const O& obj, vec_t v) noexcept{
			v.normalize();

			const vec_t base_depart = sbj.depart_vector_to_on_vel(obj, v) * 1.005f;

			value_type min = std::numeric_limits<value_type>::infinity();

			for(int i = 0; i < 4; ++i){
				for(int j = 0; j < 4; ++j){
					auto dst1 = math::ray_seg_intersection_dst(sbj[i] + base_depart, v, obj[j], obj[j + 1]);
					auto dst2 = math::ray_seg_intersection_dst(obj[i] - base_depart, -v, sbj[j], sbj[j + 1]);
					min = math::min(min, math::min(dst1, dst2));
				}
			}

			if(std::isinf(min)){
				return {};
			}

			assert(!std::isnan(min));
			assert(!v.is_NaN());
			assert(!base_depart.is_NaN());

			return base_depart + v * min;
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

		[[nodiscard]] FORCE_INLINE constexpr vec_t v00() const noexcept{
			return v0;
		}

		[[nodiscard]] FORCE_INLINE constexpr vec_t v10() const noexcept{
			return v1;
		}

		[[nodiscard]] FORCE_INLINE constexpr vec_t v11() const noexcept{
			return v2;
		}

		[[nodiscard]] FORCE_INLINE constexpr vec_t v01() const noexcept{
			return v3;
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


	export
	//TODO redesign the invariants of the class?
	template <typename T>
	struct quad_bounded : protected quad<T>{
		using base = quad<T>;

	protected:
		typename base::rect_t bounding_box{};

	public:
		[[nodiscard]] constexpr quad_bounded() = default;


		[[nodiscard]] constexpr explicit(false) quad_bounded(const base& base)
			: base{base}, bounding_box{base::get_bound()}{
		}

		[[nodiscard]] constexpr explicit(false) quad_bounded(typename base::rect_t& rect)
			: base{rect.vert_00(), rect.vert_10(), rect.vert_11(), rect.vert_01()}, bounding_box{rect}{
		}

		[[nodiscard]] constexpr explicit(false) quad_bounded(
			typename base::vec_t::const_pass_t v0,
			typename base::vec_t::const_pass_t v1,
			typename base::vec_t::const_pass_t v2,
			typename base::vec_t::const_pass_t v3

		)
			: base{v0, v1, v2, v3}, bounding_box{base::get_bound()}{
		}

		[[nodiscard]] constexpr FORCE_INLINE typename base::rect_t get_bound() const noexcept{
			return bounding_box;
		}

		FORCE_INLINE constexpr void move(typename base::vec_t::const_pass_t vec) noexcept{
			base::move(vec);

			bounding_box.src += vec;
		}

		constexpr typename base::vec_t expand(const T length) noexcept{
			auto rst = base::expand(length);

			bounding_box = base::get_bound();

			return rst;
		}

		/**
		 * @warning Breaks Invariant! For Fast Only
		 */
		constexpr typename base::vec_t expand_unchecked(const T length) noexcept{
			return base::expand(length);
		}

		constexpr explicit(false) operator base() const noexcept{
			return base{*this};
		}

		const base& view_as_quad() const noexcept{
			return *this;
		}

		template <std::integral Idx>
		FORCE_INLINE constexpr typename base::vec_t operator[](const Idx idx) const noexcept{
			switch (idx & vertex_mask<Idx>){
			case 0: return this->v0;
			case 1: return this->v1;
			case 2: return this->v2;
			case 3: return this->v3;
			default: std::unreachable();
			}
		}

		using base::avg;
		using base::axis_proj_dst;
		using base::axis_proj_overlaps;
		using base::contains;
		using base::expand;
		using base::edge_normal_at;
		using base::move;
		using base::overlap_rough;
		using base::overlap_exact;
		using base::depart_vector_to;
		using base::depart_vector_to_on;
		using base::depart_vector_to_on_vel;
		using base::depart_vector_to_on_vel_rough_min;
		using base::depart_vector_to_on_vel_exact;
		using base::v00;
		using base::v10;
		using base::v11;
		using base::v01;
	};

	export
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

	export
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

		/*constexpr*/ void updateNormal() noexcept{
			//TODO constexpr normal in c++26
			normalU = (this->v0 - this->v3).normalize();
			normalV = (this->v1 - this->v0).normalize();
		}

	public:
		[[nodiscard]] rect_box() = default;



		[[nodiscard]] explicit rect_box(const base& base)
			: base(base), normalU((this->v0 - this->v3).normalize()), normalV((this->v1 - this->v0).normalize()){
		}
		[[nodiscard]] explicit rect_box(const typename base::rect_t& rect)
			: base(rect), normalU((this->v0 - this->v3).normalize()), normalV((this->v1 - this->v0).normalize()){
		}

		// [[nodiscard]] RectBoxBrief() = default;
		//
		[[nodiscard]] constexpr rect_box(
			const typename base::vec_t::const_pass_t v0,
			const typename base::vec_t::const_pass_t v1,
			const typename base::vec_t::const_pass_t v2,
			const typename base::vec_t::const_pass_t v3) noexcept
			: base{v0, v1, v2, v3}, normalU((this->v0 - this->v3).normalize()), normalV((this->v1 - this->v0).normalize()){
			assert(math::abs(normalU.dot(normalV)) < 1E-2);
		}

		/**
		 * @brief normal at vtx[idx] - vtx[idx + 1]
		 */
		FORCE_INLINE typename base::vec_t edge_normal_at(const std::integral auto idx) const noexcept{
			switch(idx & vertex_mask<decltype(idx)>) {
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

	export
	struct rect_box_posed : rect_box<float>, rect_box_identity<float>{
		using rect_idt_t = rect_box_identity<float>;
		using base = rect_box<float>;
		using trans_t = trans2;
		/**
		 * \brief Box Origin Point
		 * Should Be Mass Center if possible!
		 */
		// trans_t transform{};

		[[nodiscard]] constexpr rect_box_posed() = default;

		[[nodiscard]] explicit(false) rect_box_posed(const rect_idt_t& idt, const trans_t transform = {})
			: rect_box_identity{idt}{
			update(transform);
			updateNormal();
		}

		[[nodiscard]] explicit(false) rect_box_posed(const vec_t size, const trans_t transform = {})
			: rect_box_posed{{size, size / -2.f}, transform}{
		}

		constexpr rect_box_posed& copy_identity_from(const rect_box_posed& other) noexcept{
			this->rect_idt_t::operator=(other);
			// transform = other.transform;

			return *this;
		}

		[[nodiscard]] constexpr float get_rotational_inertia(const float mass, const float scale = 1 / 12.0f, const float lengthRadiusRatio = 0.25f) const noexcept {
			return size.length2() * (scale + lengthRadiusRatio) * mass;
		}

		void update(const trans_t transform) noexcept{
			value_type rot = transform.rot;
			auto [cos, sin] = cos_sin_deg(rot);

			v0.set(offset).rotate(cos, sin);
			v1.set(size.x, 0).rotate(cos, sin);
			v3.set(0, size.y).rotate(cos, sin);
			v2 = v1 + v3;

			// if(this->transform.rot != transform.rot){
			normalU = v3;
			normalV = v1;

			normalU.normalize();
			normalV.normalize();

			// 	this->transform.rot = transform.rot;
			// }

			v0 += transform.vec;
			v1 += v0;
			v2 += v0;
			v3 += v0;


			bounding_box = quad<float>::get_bound();
		}

		// void rotate(const trans_t::angle_t delta_deg) noexcept{
		// 	update({transform.vec, transform.rot + delta_deg});
		// }
	};


	export
	template <>
	struct overlay_axis_keys<rect_box_posed>{
		static constexpr std::size_t count = 2;

		template <std::size_t Idx>
		FORCE_INLINE static constexpr auto get(const rect_box_posed& quad) noexcept{
			return quad.edge_normal_at(Idx);
		}
	};

	using fp_t = float;

	export
	template <typename T>
	concept quad_like = std::is_base_of_v<quad<fp_t>, T>;

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
	template <quad_like T>
	[[nodiscard]] constexpr vector2<fp_t> nearest_edge_normal(const T& rectangle, const vector2<fp_t> p) noexcept {
		fp_t minDistance = std::numeric_limits<fp_t>::max();
		vector2<fp_t> closestEdgeNormal{};

		for (int i = 0; i < 4; i++) {
			auto a = rectangle[i];
			auto b = rectangle[i + 1];

			const fp_t d = math::dst2_to_segment(p, a, b);

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

			normals[i].weight = dst_to_segment(where, va, vb) * va.dst(vb);
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

}
