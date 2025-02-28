module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk.vertex_info;

import std;
import mo_yanxi.math.vector2;
import mo_yanxi.math.vector4;

namespace mo_yanxi::vk{
	namespace vertices{
		using floating_type = float;

		struct vertex_attribute{
			std::uint32_t offset;
			VkFormat format;
		};

		export
		struct vertex_bind_info{
			std::array<vertex_attribute, 16> attributes{};
			std::uint32_t count{};
			std::uint32_t stride{};

			[[nodiscard]] constexpr vertex_bind_info(
				const std::uint32_t size, const std::initializer_list<vertex_attribute> attribs) : stride(size){
				for (const auto & attrib : attribs){
					attributes[count++] = attrib;
				}
			}

		public:
			[[nodiscard]] VkVertexInputBindingDescription get_bind_desc(
				const std::uint32_t binding = 0,
				const VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX
			) const noexcept{
				return {
						.binding = binding,
						.stride = stride,
						.inputRate = inputRate
					};
			}

			[[nodiscard]] std::vector<VkVertexInputAttributeDescription> get_attr_desc(
				const std::uint32_t binding = 0
			) const{
				return attributes
				| std::views::take(count)
				| std::views::enumerate
				| std::views::transform(
					[&](std::tuple<decltype(attributes)::size_type, vertex_attribute> attr){
						auto [idx, atr] = attr;
						return VkVertexInputAttributeDescription{
								.location = static_cast<std::uint32_t>(idx),
								.binding = binding,
								.format = atr.format,
								.offset = atr.offset,
							};
					})
				| std::ranges::to<std::vector>();
			}
		};

		export
		struct texture_indices{
			std::uint8_t texture_index{};
			std::uint8_t texture_layer{};
			std::uint8_t reserved1{};
			std::uint8_t reserved2{};
		};

		export
		struct vertex_world {
			math::vector2<floating_type> pos{};
			floating_type depth{};
			texture_indices tex_indices{};
			math::vector4<floating_type> color{};
			math::vector4<floating_type> override_color{};
			math::vector2<floating_type> uv{};
		};

		export
		struct vertex_ui {
			math::vector2<floating_type> pos{};
			texture_indices tex_indices{};
			math::vector4<floating_type> color{};
			math::vector4<floating_type> override_color{};
			math::vector2<floating_type> uv{};
		};

		export
		constexpr vertex_bind_info vertex_world_info{
			sizeof(vertex_world),
			{
				{0, VK_FORMAT_R32G32B32_SFLOAT},
				{12, VK_FORMAT_R8G8B8A8_UINT},
				{16, VK_FORMAT_R32G32B32A32_SFLOAT},
				{32, VK_FORMAT_R32G32B32A32_SFLOAT},
				{48, VK_FORMAT_R32G32_SFLOAT},
			}
		};

		export
		constexpr vertex_bind_info vertex_ui_info{
			sizeof(vertex_world),
			{
				{0, VK_FORMAT_R32G32_SFLOAT},
				{8, VK_FORMAT_R8G8B8A8_UINT},
				{12, VK_FORMAT_R32G32B32A32_SFLOAT},
				{28, VK_FORMAT_R32G32B32A32_SFLOAT},
				{44, VK_FORMAT_R32G32_SFLOAT},
			}
		};

		// struct InstanceDesignator{
		//     std::int8_t offset{};
		// };
		//
		// using WorldVertBindInfo = Core::Vulkan::Util::VertexBindInfo<Vertex_World, 0, 0, VK_VERTEX_INPUT_RATE_VERTEX,
		// 	std::pair{&Vertex_World::pos, VK_FORMAT_R32G32B32_SFLOAT},
		// 	std::pair{&Vertex_World::tex_indices, VK_FORMAT_R8G8B8A8_UINT},
		// 	std::pair{&Vertex_World::uv, VK_FORMAT_R32G32_SFLOAT},
		// 	std::pair{&Vertex_World::color, VK_FORMAT_R32G32B32A32_SFLOAT}
		// >;

		// using InstanceBindInfo = Core::Vulkan::Util::VertexBindInfo<InstanceDesignator, 0, WorldVertBindInfo::size, VK_VERTEX_INPUT_RATE_INSTANCE,
		// 	std::pair{&InstanceDesignator::offset, VK_FORMAT_R8_SINT}
		// >;

		// struct Vertex_UI {
		// 	Geom::Vec2 position{};
		// 	texture_indices textureParam{};
		// 	Graphic::Color color{};
		// 	Geom::Vec2 texCoord{};
		// };
	}
	namespace indices{
		export
		using default_index_t = std::uint32_t;

		export
		inline constexpr std::array<default_index_t, 6> standard_index_base{0, 1, 2, 2, 3, 0};

		//TODO is constexpr here meaningful?
		export
		template <std::size_t repeats, typename Ty = default_index_t, typename BaseTy, std::size_t baseSize>
		constexpr std::array<Ty, repeats * baseSize> generate_index_references(const std::array<BaseTy, baseSize> base) noexcept{
			std::array<Ty, repeats * baseSize> result{};

			const auto [min, max] = std::ranges::minmax(base);
			const auto data = std::ranges::data(base);
			const Ty stride = static_cast<Ty>(max - min + 1);

			//TODO overflow check

			for(std::size_t i = 0; i < repeats; ++i){
				for(std::size_t j = 0; j < baseSize; ++j){
					result.data()[j + i * baseSize] = static_cast<Ty>(data[j] + i * stride);
				}
			}

			return result;
		}

		export
		template <std::size_t size = std::bit_ceil(10000u)>
		constexpr inline std::array standard_indices_group = generate_index_references<size, std::uint32_t>(standard_index_base);

		export
		constexpr inline auto default_indices_group = standard_indices_group<>;
	}
}
