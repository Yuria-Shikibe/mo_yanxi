module;

#include <vulkan/vulkan.h>

export module volume_draw;

export import :primitives;
import mo_yanxi.vk;

import mo_yanxi.seq_chunk;

namespace mo_yanxi::graphic::draw::instruction{
namespace volume{

constexpr VkDeviceSize instr_size = 1 << 14;
constexpr std::size_t max_volume_represent = 128;

struct batch_instr_buffer{
	std::pmr::unsynchronized_pool_resource pool{std::pmr::new_delete_resource()};

	using chunk_type =
	seq_chunk<
		std::pmr::vector<shockwave>
	>;

	chunk_type chunk{&pool};

	template <typename T, typename S>
	decltype(auto) at(this S&& self) noexcept {
		return std::forward_like<S>(self.chunk).template get<std::pmr::vector<T>>();
	}

	bool update(float current_time) noexcept {
		return [&, this]<std::size_t ...Idx>(std::index_sequence<Idx...>){
			return ([&, this]<std::size_t I>(){
				return std::erase_if(get<I>(chunk), [&](const auto& instr){
					return instruction::check_is_expired(instr, current_time);
				}) != 0;
			}.template operator()<Idx>() || ...);
		}(std::make_index_sequence<chunk_type::chunk_element_count>{});
	}

	[[nodiscard]] std::size_t get_total_instr_size() const noexcept {
		return [this]<std::size_t ...Idx>(std::index_sequence<Idx...>){
			return (std::ranges::size(get<Idx>(chunk)) + ...);
		}(std::make_index_sequence<chunk_type::chunk_element_count>{});
	}
};

struct volume_ubo_data{
	float current_time;
	std::uint32_t _cap1;
	std::uint32_t _cap2;
	std::uint32_t _cap3;
};

export struct batch{
private:
	batch_instr_buffer instr_buffer{};
	bool changed{};

	vk::context* context_{};
	vk::descriptor_layout descriptor_layout_{};
	vk::descriptor_buffer descriptor_buffer_{};

	vk::buffer gpu_instr_buffer_{};
	vk::uniform_buffer uniform_buffer_{};
	float current_time{};
	float last_update_time{};

public:

	[[nodiscard]] explicit batch(
		vk::context& context,
		VkSampler sampler = nullptr
		): context_(&context), descriptor_layout_{
			context.get_device(), VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
			[](vk::descriptor_layout_builder& builder){
				builder.push_seq(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);

				[&]<std::size_t ...Idx>(std::index_sequence<Idx...>){
					builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
				}(std::make_index_sequence<batch_instr_buffer::chunk_type::chunk_element_count>{});
			}
		}
	, descriptor_buffer_(context.get_allocator(), descriptor_layout_, descriptor_layout_.binding_count())
	, gpu_instr_buffer_(
			vk::templates::create_storage_buffer(context.get_allocator(),
			                                     instr_size,
			                                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT))
	, uniform_buffer_(
		context.get_allocator(),
		sizeof(volume_ubo_data) + ((batch_instr_buffer::chunk_type::chunk_element_count + 3) / 4 * 4) * sizeof(std::uint32_t),
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
	{
		(void)vk::descriptor_mapper{descriptor_buffer_}.set_uniform_buffer(0, uniform_buffer_);

	}

	void update(float CURRENT_time) noexcept{
		if(CURRENT_time < current_time){
			//TODO clear the batch
		}


		current_time = CURRENT_time;
		(void)vk::buffer_mapper{uniform_buffer_}.load(volume_ubo_data{
			current_time
		});

		const bool new_added = std::exchange(changed, false);
		const bool need_buffer_check = new_added || last_update_time + 10.f < current_time;
		const bool any_changed = (need_buffer_check && instr_buffer.update(current_time)) || new_added;

		if(need_buffer_check)last_update_time = current_time;
		if(!any_changed)return;

		flush();
	}

	template <typename T>
	void push_instr(const T& instr){
		auto& rst = instr_buffer.at<T>().emplace_back(instr);
		volume_generic& g = rst.generic;
		g.birth_time = current_time;

		changed = true;
	}

	void flush(){
		const vk::buffer_mapper bufMapper{gpu_instr_buffer_};
		const vk::descriptor_mapper dmper{descriptor_buffer_};


		std::array<std::uint32_t, (batch_instr_buffer::chunk_type::chunk_element_count + 3) / 4 * 4> instr_subranges{};

		std::byte* const begin = bufMapper.get_mapper_ptr();
		const std::byte* const sentinel = begin + gpu_instr_buffer_.get_size();
		std::byte* current = begin;

		[&, this]<std::size_t ...Idx>(std::index_sequence<Idx...>){
			(void)([&, this]<std::size_t I>(){
				const auto& rng = get<I>(instr_buffer.chunk);
				using range_ref = decltype(rng);
				using range_value_type = std::ranges::range_value_t<range_ref>;
				const auto range_size = std::ranges::size(rng);
				const auto max_size = ((sentinel - current) / sizeof(range_value_type)) * sizeof(range_value_type);
				const auto actual = std::min(max_size, range_size);

				if(actual){
					std::memcpy(current, std::ranges::data(rng), actual * sizeof(range_value_type));
					instr_subranges[I] = actual;

					(void)dmper.set_storage_buffer(
						1 + I,
						gpu_instr_buffer_.get_address() + (current - begin),
						actual * sizeof(range_value_type)
					);

					current += actual * sizeof(range_value_type);
				}

				return range_size == actual;
			}.template operator()<Idx>() && ...);
		}(std::make_index_sequence<batch_instr_buffer::chunk_type::chunk_element_count>{});

		(void)vk::buffer_mapper{uniform_buffer_}.load_range(instr_subranges, sizeof(volume_ubo_data));

	}

	[[nodiscard]] VkDescriptorSetLayout get_descriptor_layout() const noexcept{
		return descriptor_layout_;
	}

	[[nodiscard]] VkDescriptorBufferBindingInfoEXT get_descriptor_binding_info() const noexcept{
		return descriptor_buffer_;
	}
};
}

}