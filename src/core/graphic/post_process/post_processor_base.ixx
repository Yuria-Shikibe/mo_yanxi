module;

#include <cassert>
#include <vulkan/vulkan.h>

export module mo_yanxi.graphic.post_processor.base;

export import mo_yanxi.vk.context;
export import mo_yanxi.vk.command_buffer;
export import mo_yanxi.vk.uniform_buffer;
export import mo_yanxi.vk.descriptor_buffer;
export import mo_yanxi.vk.pipeline.layout;
export import mo_yanxi.vk.image_derives;
export import mo_yanxi.vk.pipeline;
export import mo_yanxi.vk.util.cmd.render;
export import mo_yanxi.vk.util;
export import mo_yanxi.math.vector2;
import mo_yanxi.vk.exception;
import std;

namespace mo_yanxi::graphic{
	export
	struct post_processor_socket_unmatch_error final : std::exception{
		[[nodiscard]] post_processor_socket_unmatch_error() = default;

		[[nodiscard]] explicit post_processor_socket_unmatch_error(std::size_t count, char const* Message = "Inputs Count Mismatch")
			: exception(Message){
		}

	};

	struct texel_pool{
		//TODO share texels in different post processors
	};

	export
	struct post_process_socket{
		VkImage image;
		VkImageView view;
		VkImageLayout layout;
		std::uint32_t queue_family_index;

		explicit(false) operator vk::image_handle() const noexcept{
			return {image, view};
		}
	};

	export
	struct post_processor{
		static constexpr math::u32size2 compute_group_unit_size2{16, 16};

		constexpr static math::u32size2 get_work_group_size(math::u32size2 image_size) noexcept{
			return image_size.add(compute_group_unit_size2.copy().sub(1u, 1u)).div(compute_group_unit_size2);
		}

		static constexpr VkExtent2D size_to_extent_2d(math::u32size2 sz) noexcept{
			return std::bit_cast<VkExtent2D>(sz);
		}

	protected:
		math::usize2 size_{};
		vk::context* context_{};

		vk::constant_layout constant_layout{};
		vk::descriptor_layout descriptor_layout{};
		vk::descriptor_buffer descriptor_buffer{};
		vk::pipeline_layout pipeline_layout{};
		vk::pipeline pipeline{};

		vk::command_buffer main_command_buffer{};

		virtual void record_commands() = 0;

		void default_init_descriptor_buffer(const std::uint32_t chunk_count = 1){
			descriptor_buffer = vk::descriptor_buffer{context_->get_allocator(), descriptor_layout, descriptor_layout.binding_count(), chunk_count};
		}

		void default_init_pipeline(
			const VkPipelineShaderStageCreateInfo& shader_info,
			std::initializer_list<VkDescriptorSetLayout> appendDescriptors = {}
		){
			if(appendDescriptors.size() == 0){
				pipeline_layout = vk::pipeline_layout{context().get_device(), 0, {descriptor_layout}, constant_layout.get_constant_ranges()};
			}else{
				std::vector<VkDescriptorSetLayout> arr{};
				arr.push_back(descriptor_layout);
				arr.append_range(appendDescriptors);
				pipeline_layout = vk::pipeline_layout{context().get_device(), 0, arr, constant_layout.get_constant_ranges()};
			}

			pipeline = vk::pipeline{context().get_device(), pipeline_layout, VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT, shader_info};
		}

	protected:
		static void throwExceptionOnCommandBufferRequirement(){
			throw vk::unqualified_error{"Command Buffer Required But Null Provided"};
		}

	public:
		std::vector<post_process_socket> inputs{};
		std::vector<post_process_socket> outputs{};

		[[nodiscard]] post_processor() = default;

		[[nodiscard]] explicit post_processor(
			vk::context& context, math::usize2 size) :
		size_(size), context_(&context), main_command_buffer(context.get_compute_command_pool().obtain())
		{

		}

		virtual ~post_processor() = default;

		virtual void run(
			VkFence fence, VkSemaphore wait, VkSemaphore to_signal){
			vk::cmd::submit_command(context().compute_queue(), main_command_buffer, fence, wait, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT);
		}

		virtual void resize(VkCommandBuffer command_buffer, math::usize2 size, bool record_command){
			this->size_ = size;
			if(record_command)record_commands();
		}

		virtual void set_input(std::initializer_list<post_process_socket> sockets){
			inputs = sockets;
		}

		virtual void set_output(std::initializer_list<post_process_socket> sockets){
			outputs = sockets;
		}

		[[nodiscard]] math::usize2 size() const noexcept{
			return size_;
		}

		[[nodiscard]] VkCommandBuffer get_main_command_buffer() const noexcept{
			return main_command_buffer;
		}

		[[nodiscard]] vk::context& context() const noexcept{
			assert(context_ != nullptr);
			return *context_;
		}

		post_processor(const post_processor& other) = delete;
		post_processor(post_processor&& other) noexcept = default;
		post_processor& operator=(const post_processor& other) = delete;
		post_processor& operator=(post_processor&& other) noexcept = default;
	};
}
