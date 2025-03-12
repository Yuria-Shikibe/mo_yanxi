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
	export
	struct renderer_export{

		string_open_addr_hash_map<vk::image_handle> results{};
	};

	export
	struct renderer{
	protected:
		renderer_export* export_{};
		std::string name{};

		math::irect region{};

		void set_export(vk::image_handle handle) const{
			export_->results.insert_or_assign(name, handle);
		}

	public:

		[[nodiscard]] renderer() = default;

		[[nodiscard]] explicit renderer(
			vk::context& context,
			renderer_export& export_target,
			const std::string_view name
		)
			: export_(&export_target), name(name){
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
	};
}
