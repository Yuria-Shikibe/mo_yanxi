module;

#include <vulkan/vulkan.h>
#include "../src/ext/adapted_attributes.hpp"

export module mo_yanxi.graphic.draw.transparent.buffer;

import std;
import mo_yanxi.vk.vertex_info;
import mo_yanxi.vk.batch;
import mo_yanxi.algo.timsort;

namespace mo_yanxi::graphic::draw{
	using floating_type = float;

	export
	struct transparent_vertex_group{
		std::array<vk::vertices::vertex_world_transparent, 4> vertices{};
		floating_type depth{};
		VkImageView view{};
		vk::vertices::texture_indices indices{};
	};

	export
	struct transparent_drawer_buffer{
		using vertices_container = std::vector<transparent_vertex_group>;
		vertices_container vertex_groups{};

		void sort() noexcept{
			algo::timsort(vertex_groups, {}, &transparent_vertex_group::depth);
		}

		void dump(vk::batch& batch) const{

			const auto begin = std::ranges::find_if(vertex_groups, std::identity{}, &transparent_vertex_group::view);
			if(begin == vertex_groups.end()){
				return;
			}

			const auto max_group = batch.get_vertex_group_count_per_chunk();

			VkImageView lastView{begin->view};
			vertices_container::const_iterator last{begin};

			for(auto itr = begin; itr != vertex_groups.end(); ++itr){
				if(itr->view != lastView){
					flush(batch, last, itr, lastView);
					lastView = itr->view;
					last = itr;
					continue;
				}

				if(itr - last > max_group){
					flush(batch, last, itr, lastView);
					last = itr;
				}
			}

			flush(batch, last, vertex_groups.end(), lastView);
		}

		void clear() noexcept{
			vertex_groups.clear();
		}

	private:
		FORCE_INLINE static void flush(vk::batch& batch, const vertices_container::const_iterator src, const vertices_container::const_iterator dst, VkImageView view){
			const auto acr_count = dst - src;
			const auto rng = batch.acquire(acr_count, view);

			auto* dst_pre = reinterpret_cast<vk::vertices::vertex_world*>(rng.pre.data);
			auto count = rng.pre.get_vertex_group_count(sizeof(vk::vertices::vertex_world));

			for(std::size_t i = 0; i < count; ++i){
				const auto& group = src[i];
				for(std::size_t j = 0; j < 4; ++j){
					std::construct_at(
						dst_pre + i * 4 + j,
						group.vertices[j].pos,
						group.depth,
						vk::vertices::texture_indices{
							static_cast<std::uint8_t>(rng.pre.view_index),
							group.indices.texture_layer,
							group.indices.mode_flag,
							group.indices.reserved2,
						},
						group.vertices[j].color,
						group.vertices[j].override_color,
						group.vertices[j].uv
					);
				}
			}

			if(!rng.post.size_in_bytes)return;
			auto post_count = rng.post.get_vertex_group_count(sizeof(vk::vertices::vertex_world));
			auto* dst_post = reinterpret_cast<vk::vertices::vertex_world*>(rng.post.data);

			for(std::size_t i = 0; i < post_count; ++i){
				const auto& group = src[i + count];
				for(std::size_t j = 0; j < 4; ++j){
					std::construct_at(
						dst_post + i * 4 + j,
						group.vertices[j].pos,
						group.depth,
						vk::vertices::texture_indices{
							static_cast<std::uint8_t>(rng.post.view_index),
							group.indices.texture_layer,
							group.indices.mode_flag,
							group.indices.reserved2,
						},
						group.vertices[j].color,
						group.vertices[j].override_color,
						group.vertices[j].uv
					);
				}
			}
		}
	};

}
