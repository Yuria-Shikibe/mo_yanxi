module;

#include <vulkan/vulkan.h>
#include "../src/ext/adapted_attributes.hpp"

export module mo_yanxi.graphic.draw;

export import mo_yanxi.vk.batch;
export import mo_yanxi.vk.vertex_info;
export import mo_yanxi.graphic.color;
export import mo_yanxi.graphic.image_region;

import std;

namespace mo_yanxi::graphic::draw{

	export using mode_flags = vk::vertices::mode_flag_bits;

	export struct per_primitive_t{};
	export per_primitive_t per_primitive;


	export
	template <typename Vtx>
	struct mapper{
		static FORCE_INLINE constexpr Vtx& operator()(Vtx* ptr, std::size_t idx, math::vec2 pos, vk::vertices::texture_indices, color color_scl, math::vec2 uv) = delete;
	};

	template<>
	struct mapper<vk::vertices::vertex_world>{
		static FORCE_INLINE constexpr vk::vertices::vertex_world& operator()(
			vk::vertices::vertex_world* ptr,
			const std::size_t idx,
			math::vec2 pos,
			vk::vertices::texture_indices texture_indices,
			color color_scl,
			math::vec2 uv
			) noexcept{
			const auto p = std::assume_aligned<std::bit_ceil(sizeof(vk::vertices::vertex_world))>(ptr);
			return *std::construct_at(p + idx, pos, 0., texture_indices, color_scl, uv);
		}
	};

	template<>
	struct mapper<vk::vertices::vertex_ui>{
		static FORCE_INLINE constexpr vk::vertices::vertex_ui& operator()(
			vk::vertices::vertex_ui* ptr,
			const std::size_t idx,
			math::vec2 pos,
			vk::vertices::texture_indices texture_indices,
			color color_scl,
			math::vec2 uv
			) noexcept{
			const auto p = std::assume_aligned<std::bit_ceil(sizeof(vk::vertices::vertex_ui))>(ptr);
			return *std::construct_at(p + idx, pos, texture_indices, color_scl, uv);
		}
	};

	export
	struct basic_batch_param_proj{
		FORCE_INLINE constexpr static vk::vertices::texture_indices operator()(const std::uint32_t index) noexcept {
			return {static_cast<std::uint8_t>(index)};
		}

	};

	export
	struct mode_bit_proj{
		std::uint8_t layer{};
		vk::vertices::mode_flag_bits mode_flag{};

		template <typename T>
		FORCE_INLINE void set_layer(const T& val) noexcept{
			layer = static_cast<std::uint8_t>(val);
		}

		FORCE_INLINE constexpr vk::vertices::texture_indices operator()(const std::uint32_t index) const noexcept {
			return {static_cast<std::uint8_t>(index), layer, mode_flag};
		}
	};

	export
	struct world_vertex_proj : basic_batch_param_proj{
		float depth;

		void slightly_incr_depth(){
			depth += std::numeric_limits<float>::epsilon();//std::nextafter(depth, std::numeric_limits<float>::max());
		}

		void slightly_decr_depth(){
			depth -= std::numeric_limits<float>::epsilon();//std::nextafter(depth, std::numeric_limits<float>::lowest());
		}

		FORCE_INLINE constexpr void operator()(vk::vertices::vertex_world& vertex) const noexcept {
			vertex.depth = depth;
		}

		using basic_batch_param_proj::operator();
	};

	export
	template <typename Vtx, std::derived_from<uniformed_rect_uv> UV = uniformed_rect_uv, typename Proj = basic_batch_param_proj>
	struct batch_draw_param{

		Vtx* target;
		UV uv;
		std::uint32_t tex_index;
		ADAPTED_NO_UNIQUE_ADDRESS Proj proj;

		[[nodiscard]] FORCE_INLINE constexpr vk::vertices::texture_indices get_texture() const noexcept{
			return proj(tex_index);
		}
	};


	export
	template <typename Vtx, typename  UV, typename Proj>
	struct auto_acquirer_trait{
		using param_type = batch_draw_param<Vtx, UV, Proj>;
		using vertex_type = Vtx;
		using uv_type = UV;
		using projection = Proj;
		using region_type = combined_image_region<UV>;

	protected:
		region_type region{};

	public:
		ADAPTED_NO_UNIQUE_ADDRESS Proj proj{};

		[[nodiscard]] auto_acquirer_trait() = default;

		[[nodiscard]] explicit auto_acquirer_trait(const region_type& region)
			: region(region){
		}

		template <std::derived_from<auto_acquirer_trait> Ty, std::derived_from<UV> UVTy>
		Ty& operator <<(this Ty& self, const UVTy& uv) noexcept{
			self.region.uv = static_cast<UV>(uv);
			return self;
		}

		[[nodiscard]] VkImageView get_view() const noexcept{
			return region.view;
		}

		[[nodiscard]] region_type get_region() const noexcept{
			return region;
		}
	};

	export
	template <typename Vtx, std::derived_from<uniformed_rect_uv> UV = uniformed_rect_uv, typename Proj = basic_batch_param_proj>
	struct auto_batch_acquirer : auto_acquirer_trait<Vtx, UV, Proj>{
	private:
		vk::batch* batch{};

		using range_type = vk::batch_allocation_range<Vtx>;
		range_type rng{};
		typename range_type::iterator current{};
		typename range_type::sentinel sentinel{};


	public:
		void acquire(const std::size_t group_count) noexcept{
			rng = range_type{batch->acquire(group_count, this->region.view)};
			current = rng.begin();
			sentinel = rng.end();
		}

		[[nodiscard]] explicit auto_batch_acquirer(vk::batch& batch)
			: batch(&batch){
		}

		[[nodiscard]] auto_batch_acquirer(vk::batch& batch, const combined_image_region<UV>& region)
			: auto_acquirer_trait<Vtx, UV, Proj>(region),
			  batch(&batch){
		}

		auto_batch_acquirer& operator <<(const combined_image_region<UV>& region) noexcept{
			if(this->region.view != region.view){
				rng = {};
				current = {};
				sentinel = {};
			}

			this->region = region;
			return *this;
		}

		auto_batch_acquirer& operator <<(const VkImageView view) noexcept{
			if(this->region.view != view){
				this->region.view = view;
				rng = {};
				current = {};
				sentinel = {};
			}

			return *this;
		}

		using auto_acquirer_trait<Vtx, UV, Proj>::operator<<;

		batch_draw_param<Vtx, UV, Proj> operator*() const noexcept{
			assert(current != sentinel);
			return batch_draw_param<Vtx, UV, Proj>{(*current).data(), this->region.uv, rng.get_image_index(current.index), this->proj};
		}

		auto_batch_acquirer& operator++() noexcept{
			if(current == sentinel){
				acquire(1);
			}

			++current;
			return *this;
		}

		auto_batch_acquirer& operator+=(const std::size_t group_count){
			acquire(group_count);
			return *this;
		}


		auto_batch_acquirer& operator >>(const std::size_t group_count) noexcept{
			current += group_count;
			assert(current <= sentinel);

			return *this;
		}

		batch_draw_param<Vtx, UV, Proj> operator[](const std::ptrdiff_t group_count){
			auto offed = current.index + group_count;
			assert(std::cmp_less(offed, sentinel.index));
			return batch_draw_param<Vtx, UV, Proj>{current[group_count].data(), this->region.uv, rng.get_image_index(offed), this->proj};
		}

		batch_draw_param<Vtx, UV, Proj> get() noexcept{
			return *this;
		}

		batch_draw_param<Vtx, UV, Proj> get_reserved(std::size_t reserve = 8) noexcept{
			if(current == sentinel){
				acquire(1);
			}

			auto rst = this->operator*();

			++current;

			return rst;
		}

		explicit(false) operator batch_draw_param<Vtx, UV, Proj> () noexcept{
			if(current == sentinel){
				acquire(1);
			}

			auto rst = this->operator*();

			++current;

			return rst;
		}
	};

	export
	template <typename UV = uniformed_rect_uv>
	using world_acquirer = auto_batch_acquirer<vk::vertices::vertex_world, UV, world_vertex_proj>;

	export
	using ui_acquirer = auto_batch_acquirer<vk::vertices::vertex_ui, uniformed_rect_uv, mode_bit_proj>;

	export constexpr inline image_rect_region white_region{{}, nullptr};

	export
	template <typename Acq>
	struct acquirer_guard{
	private:
		Acq& acq{};
		std::size_t acquired{};
	public:
		[[nodiscard]] acquirer_guard(Acq& acq, std::size_t acquired)
			: acq(acq),
			  acquired(acquired){
			acq += acquired;
		}

		~acquirer_guard(){
			acq >> acquired;
		}
	};

}
