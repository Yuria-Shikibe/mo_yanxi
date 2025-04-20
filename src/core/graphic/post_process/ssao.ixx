module;

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

export module mo_yanxi.graphic.post_processor.ssao;

export import mo_yanxi.graphic.post_processor.base;
import mo_yanxi.vk.image_derives;
import mo_yanxi.vk.uniform_buffer;
import mo_yanxi.vk.shader;
import mo_yanxi.vk.ext;
import mo_yanxi.math;
import mo_yanxi.math.vector2;
import std;

namespace mo_yanxi::graphic{
	export
	struct ssao_post_processor final : post_processor{


	protected:
		struct ssao_kernal_block {
	    	struct unit{
	    		math::vec2 off{};
	    		float weight{};
	    		float cap{};
	    	};

	        struct kernal_info {
	            std::size_t count{};
	            float distance{};
	            float weight{};
	        };

            static constexpr std::array SSAO_Params{
                    kernal_info{16, 0.15702702700f},
                    kernal_info{12, 0.32702702700f},
                    kernal_info{8, 0.55062162162f},
                    kernal_info{4, 0.83062162162f},
                };

	        static constexpr std::size_t def_kernal_size{[]{
	            std::size_t rst{};

	            for (const auto & ssao_param : SSAO_Params) {
	                rst += ssao_param.count;
	            }

	            return rst;
	        }()};

	        [[nodiscard]] constexpr ssao_kernal_block() = default;

	        [[nodiscard]] constexpr explicit ssao_kernal_block(
	        	const math::isize2 size, const float scl = 1.25f) : scale(scl){
	        	const auto screen_scale = ~size.as<float>();

	        	for(std::size_t count{}; const auto& param : SSAO_Params){
	        		for(std::size_t i = 0; i < param.count; ++i){
	        			kernal[count].off.set_polar_deg(
							(360.f / static_cast<float>(param.count)) * static_cast<float>(i), param.distance) *= screen_scale;
	        			kernal[count].weight = param.weight;
	        			count++;
	        		}
	        	}
	        }

	        static constexpr std::size_t kernal_max_size = 64;

	        static_assert(def_kernal_size <= kernal_max_size);

	        std::array<unit, kernal_max_size> kernal{};
	        std::int32_t kernalSize{def_kernal_size};
			float scale{};
	    };

		VkSampler sampler_{};

		vk::image ssao_result{};
		vk::image_view ssao_result_view{};

		vk::uniform_buffer uniform_buffer{};

		void record_commands() override{
			using namespace mo_yanxi::vk;

			scoped_recorder scoped_recorder{main_command_buffer, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT};
			const auto& input = inputs[0];

			cmd::dependency_gen dependency_gen{};

			dependency_gen.push(
				input.image,
				VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
				VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_READ_BIT,
				input.layout,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				image::depth_image_subrange,
				input.queue_family_index,
				context().compute_family()
			);

			dependency_gen.push(
				ssao_result,
			    VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			    VK_ACCESS_2_SHADER_READ_BIT,
			    VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			    VK_ACCESS_2_SHADER_WRITE_BIT,
			    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			    VK_IMAGE_LAYOUT_GENERAL,
			    image::default_image_subrange
			);

			dependency_gen.apply(scoped_recorder, true);

			pipeline.bind(scoped_recorder, VK_PIPELINE_BIND_POINT_COMPUTE);
			descriptor_buffer.bind_to(scoped_recorder, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0);

			auto groups = get_work_group_size(size());
			vkCmdDispatch(scoped_recorder, groups.x, groups.y, 1);

			dependency_gen.swap_stages();
			dependency_gen.apply(scoped_recorder, true);
		}


	public:
		[[nodiscard]] const auto& get_output() const noexcept{
			return outputs.front();
		}

		[[nodiscard]] ssao_post_processor() = default;

		void set_input(std::initializer_list<post_process_socket> sockets) override{
			if(sockets.size() != 1){
				throw post_processor_socket_unmatch_error{1};
			}
			inputs = sockets;

			vk::descriptor_mapper mapper{descriptor_buffer};
			mapper.set_image(0, inputs[0].view, 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler_);

			record_commands();
		}

		[[nodiscard]] explicit ssao_post_processor(
			vk::context& context, math::usize2 size,
			const vk::shader_module& bloom_shader,
			VkSampler sampler
		) :
			post_processor(context, size),
			sampler_(sampler),
			uniform_buffer(context.get_allocator(), sizeof(ssao_kernal_block))
			{

			vk::buffer_mapper{uniform_buffer}.load(ssao_kernal_block{this->size().as<math::isize2>()});
			descriptor_layout = vk::descriptor_layout(context.get_device(), [](vk::descriptor_layout_builder& builder){
				builder.push_seq(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);
				builder.push_seq(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
				builder.push_seq(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
			});

			default_init_pipeline(bloom_shader.get_create_info());
			default_init_descriptor_buffer();


			(void)vk::descriptor_mapper{descriptor_buffer}.set_uniform_buffer(2, uniform_buffer);

			init_image_state(context.get_transient_compute_command_buffer());
		}

		void resize(VkCommandBuffer command_buffer, const math::usize2 size, bool record_command) override{
			init_image_state(command_buffer);
			(void)vk::buffer_mapper{uniform_buffer}.load(size);
			if(record_command)record_commands();
		}

		void set_scale(const float scale){
			vk::buffer_mapper{uniform_buffer}.load(scale, std::bit_cast<std::uint32_t>(&ssao_kernal_block::scale));
		}

	private:
		void init_image_state(VkCommandBuffer command_buffer){
			static constexpr VkFormat Fmt = VK_FORMAT_R16_UNORM;
			ssao_result = vk::image{
				context().get_allocator(),
				VkImageCreateInfo{
					.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
					.imageType = VK_IMAGE_TYPE_2D,
					.format = Fmt,
					.extent = {size_.x, size_.y, 1},
					.mipLevels = 1,
					.arrayLayers = 1,
					.samples = VK_SAMPLE_COUNT_1_BIT,
					.tiling = VK_IMAGE_TILING_OPTIMAL,
					.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
				}, VmaAllocationCreateInfo{
					.usage = VMA_MEMORY_USAGE_GPU_ONLY
				}
			};

			ssao_result.init_layout(command_buffer,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);

			ssao_result_view = vk::image_view{context().get_device(), {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = ssao_result,
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = Fmt,
				.components = {},
				.subresourceRange = vk::image::default_image_subrange
			}};

			outputs.resize(1);
			outputs[0] = {
				.image = ssao_result,
				.view = ssao_result_view,
				.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				.queue_family_index = context_->compute_family()
			};

			update_descriptors();
		}


		void update_descriptors(){
			(void)vk::descriptor_mapper{descriptor_buffer}
				.set_storage_image(1, ssao_result_view, VK_IMAGE_LAYOUT_GENERAL);
		}

	};
}

