module;

#include <vulkan/vulkan.h>

export module mo_yanxi.assets.graphic;
import mo_yanxi.vk;


namespace mo_yanxi::assets::graphic{

	namespace shaders{
		namespace vert{
			export inline vk::shader_module world{};
			export inline vk::shader_module ui{};
		}

		namespace frag{
			export inline vk::shader_module world{};
			export inline vk::shader_module ui{};
			export inline vk::shader_module ui_grid{};

		}

		namespace comp{
			export inline vk::shader_module ui_blit{};
			export inline vk::shader_module ui_merge{};
			export inline vk::shader_module resolve{};
			export inline vk::shader_module bloom{};
			export inline vk::shader_module ssao{};
			export inline vk::shader_module world_merge{};
			export inline vk::shader_module result_merge{};
			export inline vk::shader_module oit_blend{};

			export inline vk::shader_module anti_aliasing{};


			namespace smaa{
				export inline vk::shader_module edge_detection{};
				export inline vk::shader_module weight_calculate{};
				export inline vk::shader_module neighborhood_blending{};
			}
		}
	}

	namespace samplers{
		export inline vk::sampler texture_sampler{};
		export inline vk::sampler ui_sampler{};
		export inline vk::sampler blit_sampler{};
	}

	namespace buffers{
		export inline VkBuffer indices_buffer{};
	}

	export void load(vk::context& context);

	export void dispose();
}
