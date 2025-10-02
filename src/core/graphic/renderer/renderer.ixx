module;

#include <vulkan/vulkan.h>

export module mo_yanxi.graphic.renderer;

export import mo_yanxi.math.rect_ortho;
export import mo_yanxi.heterogeneous.open_addr_hash;
export import mo_yanxi.vk.context;
export import mo_yanxi.vk.batch;
export import mo_yanxi.graphic.batch_proxy;
import std;

namespace mo_yanxi::graphic{
	export constexpr math::u32size2 compute_group_unit_size2{16, 16};

	export constexpr math::u32size2 get_work_group_size(math::u32size2 image_size) noexcept{
		return image_size.add(compute_group_unit_size2.copy().sub(1u, 1u)).div(compute_group_unit_size2);
	}

	export constexpr VkExtent2D size_to_extent_2d(math::u32size2 sz) noexcept{
		return std::bit_cast<VkExtent2D>(sz);
	}

	export
	struct renderer{
	protected:
		std::string name{};

		math::irect region{};

	public:

		[[nodiscard]] renderer() = default;

		[[nodiscard]] explicit renderer(
			vk::context& context,
			const std::string_view name
		) : name(name){
		}

		template <std::derived_from<renderer> T>
			requires (!std::is_const_v<T>)
		vk::batch& get_batch(this T& self) noexcept{
			return static_cast<vk::batch&>(self.batch);
		}

		template <std::derived_from<renderer> T>
		const vk::batch& get_batch(this const T& self) noexcept{
			return static_cast<const vk::batch&>(self.batch);
		}

		template <std::derived_from<renderer> T>
		[[nodiscard]] vk::context& context(this const T& self) noexcept{
			return *self.get_batch().get_context();
		}

		[[nodiscard]] std::string_view get_name() const noexcept{
			return name;
		}

		renderer(const renderer& other) = delete;
		renderer(renderer&& other) noexcept = default;
		renderer& operator=(const renderer& other) = delete;
		renderer& operator=(renderer&& other) noexcept = default;
	};
}
