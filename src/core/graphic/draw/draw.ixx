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
		static FORCE_INLINE constexpr Vtx& operator()(Vtx* ptr, std::size_t idx, math::vec2 pos, vk::vertices::texture_indices, color color_scl, color color_ovr, math::vec2 uv) = delete;
	};

	export
	template<>
	struct mapper<vk::vertices::vertex_world>{
		static FORCE_INLINE constexpr vk::vertices::vertex_world& operator()(
			vk::vertices::vertex_world* ptr,
			const std::size_t idx,
			math::vec2 pos,
			vk::vertices::texture_indices texture_indices,
			color color_scl,
			color color_ovr,
			math::vec2 uv
			) noexcept{
			return *std::construct_at(ptr + idx, pos, 0., texture_indices, color_scl, color_ovr, uv);
		}
	};

	export
	template<>
	struct mapper<vk::vertices::vertex_ui>{
		static FORCE_INLINE constexpr vk::vertices::vertex_ui& operator()(
			vk::vertices::vertex_ui* ptr,
			const std::size_t idx,
			math::vec2 pos,
			vk::vertices::texture_indices texture_indices,
			color color_scl,
			color color_ovr,
			math::vec2 uv
			) noexcept{
			return *std::construct_at(ptr + idx, pos, texture_indices, color_scl, color_ovr, uv);
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
		vk::vertices::mode_flag_bits mode_flag{};

		FORCE_INLINE constexpr vk::vertices::texture_indices operator()(const std::uint32_t index) const noexcept {
			return {static_cast<std::uint8_t>(index), 0, mode_flag};
		}
	};

	export
	struct world_vertex_proj : basic_batch_param_proj{
		float depth;

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

	protected:
		combined_image_region<UV> region{};

	public:
		ADAPTED_NO_UNIQUE_ADDRESS Proj proj{};

		[[nodiscard]] auto_acquirer_trait() = default;

		[[nodiscard]] explicit auto_acquirer_trait(const combined_image_region<UV>& region)
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

		void acquire(const std::size_t group_count) noexcept{
			rng = range_type{batch->acquire(group_count, this->region.view)};
			current = rng.begin();
			sentinel = rng.end();
		}

	public:
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
			assert(offed < sentinel.index);
			return batch_draw_param<Vtx, UV, Proj>{current[group_count].data(), this->region.uv, rng.get_image_index(offed), this->proj};
		}

		batch_draw_param<Vtx, UV, Proj> get() noexcept{
			return *this;
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

	export inline rect_region white_region{};

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
