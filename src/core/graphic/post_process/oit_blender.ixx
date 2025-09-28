module;

#include <vulkan/vulkan.h>

export module mo_yanxi.graphic.post_processor.oit_blender;

import std;

namespace mo_yanxi::graphic{

	export struct oit_statistic{
		std::uint32_t capacity;
		std::uint32_t size;

		std::uint32_t cap1;
		std::uint32_t cap2;
	};

	export struct oit_node{ // Desc Only
		using color_type = std::array<std::uint16_t, 4>;
		color_type color_base;
		color_type color_light;
		color_type color_normal;

		float depth;
		std::uint32_t next;
	};

	export std::uint32_t get_oit_buffer_count(VkExtent2D ext){
		return ext.width * ext.height * 6u;
	}

	export VkDeviceSize get_oit_buffer_size(VkExtent2D ext){
		return sizeof(oit_statistic) + get_oit_buffer_count(ext) * sizeof(oit_node);
	}
}