module;

#include <vulkan/vulkan.h>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_reflect.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <gch/small_vector.hpp>


export module mo_yanxi.graphic.post_process_graph.generic_post_process_pass;

export import mo_yanxi.graphic.post_process_graph;
export import mo_yanxi.graphic.shader_reflect;
export import mo_yanxi.vk.shader;
export import mo_yanxi.vk.pipeline;
export import mo_yanxi.vk.pipeline.layout;
export import mo_yanxi.vk.resources;
export import mo_yanxi.vk.context;
export import mo_yanxi.vk.uniform_buffer;

namespace mo_yanxi::graphic{
	export
	struct post_process_stage;

	export
	struct post_process_meta{
		friend post_process_stage;
	private:
		const vk::shader_module* shader_{};
		post_process_stage_inout_map inout_map_{};

		gch::small_vector<resource_desc::stage_ubo> uniform_buffers_{};
		gch::small_vector<resource_desc::bound_stage_resource> resources_{};

		vk::constant_layout constant_layout_{};
		vk::descriptor_layout_builder descriptor_layout_builder_{};


	public:
		inout_data sockets{};

		[[nodiscard]] post_process_meta() = default;

		[[nodiscard]] post_process_meta(const vk::shader_module& shader, const post_process_stage_inout_map& inout_map)
			: shader_(&shader),
			  inout_map_(inout_map){
			inout_map_.compact_check();



			const shader_reflection refl{shader.get_binary()};

			for(const auto& input : refl.resources().storage_images){
				const auto binding = refl.binding_info_of(input);
				resources_.push_back({resource_desc::extract_image_state(refl.compiler(), input), binding});
				if(binding.set != 0)continue;
				descriptor_layout_builder_.push(binding.binding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
			}

			for(const auto& input : refl.resources().sampled_images){
				auto binding = refl.binding_info_of(input);
				resources_.push_back({resource_desc::extract_image_state(refl.compiler(), input), binding});
				if(binding.set != 0)continue;
				descriptor_layout_builder_.push(binding.binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);
			}

			for(const auto& input : refl.resources().uniform_buffers){
				auto binding = refl.binding_info_of(input);
				uniform_buffers_.push_back({binding, get_buffer_size(refl.compiler(), input)});
				if(binding.set != 0)continue;
				descriptor_layout_builder_.push(binding.binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
			}

			for(const auto& input : refl.resources().storage_buffers){
				auto binding = refl.binding_info_of(input);
				resources_.push_back(
					{resource_desc::buffer_desc{get_buffer_size(refl.compiler(), input)}, binding});
				if(binding.set != 0)continue;
				descriptor_layout_builder_.push(binding.binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
			}

			for(const auto& pass : inout_map.connection){
				if(!std::ranges::contains(resources_, pass.binding)){
					throw std::invalid_argument("pass does not exist");
				}
			}

			sockets.data.reserve(resources_.size());
			sockets.uniform_buffers.reserve(uniform_buffers_.size());
			for(const auto& pass : inout_map_.connection){
				if(auto itr = std::ranges::find(resources_, pass.binding); itr != std::ranges::end(resources_)){
					sockets.add(pass, *itr);
				}
			}

			// spirv_cross::SpecializationConstant x, y, z;
			// auto v = refl.compiler().get_work_group_size_specialization_constants(x, y, z);
			// auto rstX = refl.compiler().get_constant(x.id);
			// auto rstY = refl.compiler().get_constant(y.id);
			//
			// auto rstZ = refl.compiler().get_constant(z.id);
		}

		resource_desc::stage_resource_desc& operator[](binding_info binding) noexcept{
			if(auto itr = std::ranges::find(resources_, binding); itr != resources_.end()){
				return *itr;
			}
			throw std::out_of_range("pass does not exist");
		}


		[[nodiscard]] std::string_view name() const noexcept{
			return shader_->get_name();
		}


		void visit_in(inout_index index, std::invocable<resource_desc::bound_stage_resource&> auto fn){
			for(const auto& connection : inout_map_.connection){
				if(connection.slot.in == index){
					std::invoke(fn, (*this)[connection.binding]);
				}
			}
		}

		void visit_out(inout_index index, std::invocable<resource_desc::bound_stage_resource&> auto fn){
			for(const auto& connection : inout_map_.connection){
				if(connection.slot.out == index){
					std::invoke(fn, (*this)[connection.binding]);
				}
			}
		}

		template <typename Fn>
		void visit_in(inout_index index, Fn fn){
			using ArgTy = std::remove_cvref_t<typename function_arg_at_last<Fn>::type>;
			for(const auto& connection : inout_map_.connection){
				if(connection.slot.in == index){
					std::invoke(fn, (*this)[connection.binding].get<ArgTy>());
				}
			}
		}

		template <typename Fn>
		void visit_out(inout_index index, Fn fn){
			using ArgTy = std::remove_cvref_t<typename function_arg_at_last<Fn>::type>;
			for(const auto& connection : inout_map_.connection){
				if(connection.slot.out == index){
					std::invoke(fn, (*this)[connection.binding].get<ArgTy>());
				}
			}
		}
	};

	export
	struct post_process_stage : pass{
		friend post_process_graph;

	private:
		post_process_meta meta_{};

		vk::descriptor_layout descriptor_layout_{};
		vk::pipeline_layout pipeline_layout_{};
		vk::uniform_buffer uniform_buffer_{};
		vk::pipeline pipeline_{};

	public:
		[[nodiscard]] explicit(false) post_process_stage(vk::context& ctx, post_process_meta&& meta)
			: pass(ctx),
			  meta_(std::move(meta)){

		}

		void post_init(vk::context& ctx) override{
			descriptor_layout_ = {ctx.get_device(), 0, meta_.descriptor_layout_builder_};
			pipeline_layout_ = vk::pipeline_layout{ctx.get_device(), 0, {descriptor_layout_}};
			pipeline_ = vk::pipeline{ctx.get_device(), pipeline_layout_, VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT, meta_.shader_->get_create_info()};
		}

		[[nodiscard]] explicit(false) post_process_stage(vk::context& ctx, const post_process_meta& meta)
			: post_process_stage(ctx, post_process_meta{meta}){
		}

		inout_data& sockets() noexcept override{
			return meta_.sockets;
		}

		[[nodiscard]] std::string_view get_name() const noexcept override{
			return meta_.name();
		}

		[[nodiscard]] const vk::shader_module& shader() const{
			return *meta_.shader_;
		}


		void add_dep(pass_dependency dep){
			dependencies.push_back(dep);
		}

		void add_dep(std::initializer_list<pass_dependency> dep){
			dependencies.append(dep);
		}
	};


}