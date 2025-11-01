module;

#include <vulkan/vulkan.h>

export module mo_yanxi.graphic.render_graph.volume;

export import mo_yanxi.graphic.render_graph.post_process_pass;
export import volume_draw;
import mo_yanxi.vk;
import mo_yanxi.math.vector2;
import std;

namespace mo_yanxi::graphic::render_graph{

export
struct volume_pass : post_process_stage{
	draw::instruction::volume::batch batch;

private:
	external_descriptor external_descriptor{};

public:
	[[nodiscard]] volume_pass(vk::context& ctx, post_process_meta&& meta)
		: post_process_stage(ctx, meta)
		  , batch(ctx), external_descriptor(batch.get_descriptor_layout(), batch.get_descriptor_binding_info()){

		add_external_descriptor(external_descriptor, 1, 0);
	}

	[[nodiscard]] volume_pass(vk::context& ctx, const post_process_meta& meta)
		: volume_pass(ctx, post_process_meta{meta}){
	}

	template <typename S>
	auto operator->(this S& self) noexcept{
		return std::addressof(self.batch);
	}
};


}
