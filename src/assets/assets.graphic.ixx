module;

#include <vulkan/vulkan.h>

export module mo_yanxi.assets.graphic;

export import mo_yanxi.vk.sampler;
export import mo_yanxi.vk.shader;

namespace mo_yanxi::vk{
	export class context;
}

namespace mo_yanxi::assets::graphic{

	namespace shaders{
		namespace vert{
			export vk::shader_module world{};
			export vk::shader_module ui{};
		}

		namespace frag{
			export vk::shader_module world{};
			export vk::shader_module ui{};

		}

		namespace comp{
			export vk::shader_module ui_blit{};
			export vk::shader_module ui_merge{};
			export vk::shader_module resolve{};
			export vk::shader_module bloom{};
			export vk::shader_module ssao{};
			export vk::shader_module world_merge{};
			export vk::shader_module result_merge{};
			export vk::shader_module oit_blend{};
		}
	}

	namespace samplers{
		export vk::sampler texture_sampler{};
		export vk::sampler ui_sampler{};
		export vk::sampler blit_sampler{};
	}

	namespace buffers{
		export inline VkBuffer indices_buffer{};
	}

	export void load(vk::context& context);

	export void dispose();
}
