module;

#include <vulkan/vulkan.h>
#include "../src/ext/adapted_attributes.hpp"

export module mo_yanxi.graphic.draw.transparent;

export import mo_yanxi.vk.vertex_info;
export import mo_yanxi.graphic.draw;
export import mo_yanxi.graphic.draw.transparent.buffer;
export import mo_yanxi.math.vector4;
import std;

namespace mo_yanxi::graphic::draw{

	export
	template<>
	struct mapper<vk::vertices::vertex_world_transparent>{
		static FORCE_INLINE constexpr vk::vertices::vertex_world_transparent& operator()(
			transparent_vertex_group* ptr,
			const std::size_t idx,
			math::vec2 pos,
			VkImageView v,
			color color_scl,
			color color_ovr,
			math::vec2 uv
			) noexcept{
			ptr->view = v;
			return *std::construct_at(ptr->vertices.data() + idx, pos, color_scl, color_ovr, uv);
		}
	};

	struct transparent_proj{
		float depth{};
		vk::vertices::texture_indices indices{};

		void operator()(per_primitive_t, transparent_vertex_group& group) const noexcept{
			group.depth = depth;
			group.indices = indices;
		}
	};

	export
	template <std::derived_from<uniformed_rect_uv> UV, typename Proj>
	struct batch_draw_param<vk::vertices::vertex_world_transparent, UV, Proj>{

		transparent_vertex_group* target;
		UV uv;
		VkImageView texture;
		ADAPTED_NO_UNIQUE_ADDRESS Proj proj;

		[[nodiscard]] FORCE_INLINE constexpr VkImageView get_texture() const noexcept{
			return texture;
		}
	};

	export
	template <std::derived_from<uniformed_rect_uv> UV, typename Proj>
	struct auto_batch_acquirer<vk::vertices::vertex_world_transparent, UV, Proj> : auto_acquirer_trait<vk::vertices::vertex_world_transparent, UV, Proj>{
	private:
		using Vtx = vk::vertices::vertex_world_transparent;
		transparent_drawer_buffer* batch{};

		transparent_drawer_buffer::vertices_container::difference_type index{};

		void acquire(const std::size_t group_count) const noexcept{
			batch->vertex_groups.resize(std::max(index + group_count, batch->vertex_groups.size()));
		}

	public:

		[[nodiscard]] auto_batch_acquirer(transparent_drawer_buffer& batch, const combined_image_region<UV>& region)
			: auto_acquirer_trait<Vtx, UV, Proj>(region), batch(&batch), index(batch.vertex_groups.size()){
		}

		using auto_acquirer_trait<Vtx, UV, Proj>::operator<<;

		auto_batch_acquirer& operator <<(const combined_image_region<UV>& region) noexcept{
			this->region = region;
			return *this;
		}

		auto_batch_acquirer& operator <<(const VkImageView view) noexcept{
			this->region.view = view;

			return *this;
		}

		auto_batch_acquirer& operator >>(const std::size_t group_count) noexcept{
			index += group_count;
			assert(index <= batch->vertex_groups.size());

			return *this;
		}

		batch_draw_param<Vtx, UV, Proj> operator*() const noexcept{

			assert(index != batch->vertex_groups.size());
			return batch_draw_param<Vtx, UV, Proj>{batch->vertex_groups.data() + index, this->region.uv, this->region.view, this->proj};
		}

		auto_batch_acquirer& operator++() noexcept{
			if(index == batch->vertex_groups.size()){
				acquire(4);
			}

			++index;
			return *this;
		}

		auto_batch_acquirer& operator+=(const std::size_t group_count){
			acquire(group_count);
			return *this;
		}

		batch_draw_param<Vtx, UV, Proj> operator[](const std::ptrdiff_t group_count){
			auto offed = index + group_count;
			assert(offed < batch->vertex_groups.size());
			return batch_draw_param<Vtx, UV, Proj>{batch->vertex_groups.data() + offed, this->region.uv, this->region.view, this->proj};
		}

		batch_draw_param<Vtx, UV, Proj> get() noexcept{
			return *this;
		}

		explicit(false) operator batch_draw_param<Vtx, UV, Proj> () noexcept{
			if(index == batch->vertex_groups.size()){
				acquire(4);
			}

			auto rst = this->operator*();

			++index;

			return rst;
		}
	};

	export
	template </*std::derived_from<uniformed_rect_uv> UV = uniformed_rect_uv, */typename Proj = transparent_proj>
	using transparent_acquirer = auto_batch_acquirer<vk::vertices::vertex_world_transparent, uniformed_rect_uv, transparent_proj>;

	export
	using default_transparent_acquirer = auto_batch_acquirer<vk::vertices::vertex_world_transparent, uniformed_rect_uv, transparent_proj>;
}
