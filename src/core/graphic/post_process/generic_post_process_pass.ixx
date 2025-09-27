module;

#include <vulkan/vulkan.h>
#include <spirv_cross/spirv_cross.hpp>
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
export import mo_yanxi.vk.descriptor_buffer;

import mo_yanxi.basic_util;
import std;

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

		auto get_local_uniform_buffer() const noexcept{
			return uniform_buffers_ | std::views::filter([](const resource_desc::stage_ubo& ubo){
				return ubo.set == 0;
			});
		}
	};

	export
	struct post_process_stage : pass{
		friend post_process_graph;

	protected:
		post_process_meta meta_{};

		vk::descriptor_layout descriptor_layout_{};
		vk::pipeline_layout pipeline_layout_{};
		vk::pipeline pipeline_{};


		std::vector<vk::uniform_buffer> uniform_buffers_{};

		std::unordered_map<binding_info, VkSampler> samplers_{};

		vk::descriptor_buffer descriptor_buffer_{};

	public:
		[[nodiscard]] explicit(false) post_process_stage(vk::context& ctx, post_process_meta&& meta)
			: pass(ctx),
			  meta_(std::move(meta)){
		}

		[[nodiscard]] explicit(false) post_process_stage(vk::context& ctx, const post_process_meta& meta)
			: post_process_stage(ctx, post_process_meta{meta}){
		}

		void set_sampler_at(binding_info binding_info, VkSampler sampler){
			samplers_.insert_or_assign(binding_info, sampler);
		}


		void set_sampler_at_binding(std::uint32_t binding_at_set_0, VkSampler sampler){
			samplers_.insert_or_assign({binding_at_set_0, 0}, sampler);
		}

		[[nodiscard]] VkSampler get_sampler_at(binding_info binding_info) const {
			return samplers_.at(binding_info);
		}

		vk::uniform_buffer& ubo(){
			if(uniform_buffers_.size() != 1){
				throw std::out_of_range("unspecified uniform buffer");
			}

			return uniform_buffers_.front();
		}

		vk::uniform_buffer& ubo_at(unsigned local_binding){
			for (const auto & [idx, desc] : meta_.get_local_uniform_buffer() |  std::views::enumerate){
				if(desc.binding == local_binding){
					return uniform_buffers_[idx];
				}
			}

			throw std::out_of_range("pass does not exist");
		}

	protected:
		auto& meta(this auto& self) noexcept{
			return self.meta_;
		}

		void init_pipeline(const vk::context& ctx) noexcept{
			descriptor_layout_ = {ctx.get_device(), VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT, meta_.descriptor_layout_builder_};
			pipeline_layout_ = vk::pipeline_layout{ctx.get_device(), 0, {descriptor_layout_}};
			pipeline_ = vk::pipeline{ctx.get_device(), pipeline_layout_, VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT, meta_.shader_->get_create_info()};
		}

		void reset_descriptor_buffer(vk::context& ctx, std::uint32_t chunk_count = 1){
			descriptor_buffer_ = vk::descriptor_buffer{ctx.get_allocator(), descriptor_layout_, descriptor_layout_.binding_count(), chunk_count};
		}

		void init_uniform_buffer(vk::context& ctx){
			for (const auto & desc : meta_.get_local_uniform_buffer()){
				uniform_buffers_.push_back(vk::uniform_buffer{ctx.get_allocator(), desc.desc.size});
			}
		}

		void default_bind_uniform_buffer(){
			vk::descriptor_mapper mapper{descriptor_buffer_};

			for (const auto & [desc, entity] : std::views::zip(meta_.get_local_uniform_buffer(), uniform_buffers_)){
				(void)mapper.set_uniform_buffer(desc.binding, entity);
			}
		}

		void default_bind_resources(const pass_resource_reference& resources){
			vk::descriptor_mapper mapper{descriptor_buffer_};
			auto bind = [&, this](const resource_map_entry& connection, const resource_desc::resource_entity* res, const resource_desc::resource_requirement& requirement){
				std::visit(overload{
						[&](const resource_desc::image_entity& entity){
							auto& req = std::get<resource_desc::image_requirement>(requirement.desc);
							if(req.is_sampled_image()){
								mapper.set_image(
									connection.binding.binding,
									entity.image.image_view,
									0,
									VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, get_sampler_at(connection.binding));
							}else{
								mapper.set_storage_image(connection.binding.binding, entity.image.image_view, VK_IMAGE_LAYOUT_GENERAL);
							}
						},
						[&](const resource_desc::buffer_entity& entity){
							(void)mapper.set_storage_buffer(connection.binding.binding, entity.buffer.begin(), entity.buffer.size);
						},
					}, res->resource);
			};
			for(const auto& connection : meta_.inout_map_.connection){
				if(auto res = resources.get_in_if(connection.slot.in)){
					bind(connection, res, meta_.sockets.at_in(connection.slot.in));
					continue;
				}

				if(auto res = resources.get_out_if(connection.slot.out)){
					bind(connection, res, meta_.sockets.at_out(connection.slot.out));
					continue;
				}

				throw std::out_of_range("failed to find resource");
			}

		}

	public:
		void post_init(vk::context& ctx, const math::u32size2 extent) override{
			init_pipeline(ctx);
			init_uniform_buffer(ctx);
			reset_descriptor_buffer(ctx);
		}

		void reset_resources(vk::context& context, const pass_resource_reference* resources, const math::u32size2 extent) override{
			default_bind_uniform_buffer();
			if(resources)default_bind_resources(*resources);
		}

		void record_command(
			vk::context& context,

			const pass_resource_reference* resources, math::u32size2 extent, VkCommandBuffer buffer) override{

			pipeline_.bind(buffer, VK_PIPELINE_BIND_POINT_COMPUTE);
			descriptor_buffer_.bind_to(buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout_, 0);

			auto groups = get_work_group_size(extent);
			vkCmdDispatch(buffer, groups.x, groups.y, 1);
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


	};


}