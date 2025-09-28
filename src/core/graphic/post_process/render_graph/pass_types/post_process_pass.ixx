module;

#include <vulkan/vulkan.h>

export module mo_yanxi.graphic.render_graph.post_process_pass;
export import mo_yanxi.graphic.render_graph.manager;

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

namespace mo_yanxi::graphic::render_graph{
	template <typename T>
	constexpr T ceil_div(T x, T div) noexcept{
		return (x + div - 1) / div;
	}

	export struct stage_ubo : binding_info{
		VkDeviceSize size;
	};

	struct bound_stage_resource : binding_info, resource_desc::resource_requirement{
	};

	export
	struct post_process_stage;

	export
	struct post_process_meta{
		friend post_process_stage;
	private:
		const vk::shader_module* shader_{};
		post_process_stage_inout_map inout_map_{};

		std::vector<stage_ubo> uniform_buffers_{};
		std::vector<bound_stage_resource> resources_{};

		vk::constant_layout constant_layout_{};
		vk::descriptor_layout_builder descriptor_layout_builder_{};
		std::vector<inout_index> required_transient_buffer_input_slots_{};

	public:
		pass_inout_connection sockets{};

		[[nodiscard]] post_process_meta() = default;

		[[nodiscard]] post_process_meta(const vk::shader_module& shader, const post_process_stage_inout_map& inout_map)
			: shader_(&shader),
			  inout_map_(inout_map){
			inout_map_.compact_check();

			const shader_reflection refl{shader.get_binary()};

			for(const auto& input : refl.resources().storage_images){
				const auto binding = refl.binding_info_of(input);
				resources_.push_back({binding, resource_desc::extract_image_state(refl.compiler(), input)});
				if(binding.set != 0)continue;
				descriptor_layout_builder_.push(binding.binding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
			}

			for(const auto& input : refl.resources().sampled_images){
				auto binding = refl.binding_info_of(input);
				resources_.push_back({binding, resource_desc::extract_image_state(refl.compiler(), input)});
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
				resources_.push_back({binding, resource_desc::buffer_requirement{get_buffer_size(refl.compiler(), input)}});
				if(binding.set != 0)continue;
				descriptor_layout_builder_.push(binding.binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
			}

			for(const auto& pass : inout_map.connection()){
				if(!std::ranges::contains(resources_, pass.binding)){
					throw std::invalid_argument("pass does not exist");
				}
			}

			sockets.data.reserve(resources_.size());
			for(const auto& pass : inout_map_.connection()){
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

		bound_stage_resource& operator[](binding_info binding) noexcept{
			if(auto itr = std::ranges::find(resources_, binding); itr != resources_.end()){
				return *itr;
			}
			throw std::out_of_range("pass does not exist");
		}


		[[nodiscard]] std::string_view name() const noexcept{
			return shader_->get_name();
		}


		void visit_in(inout_index index, std::invocable<bound_stage_resource&> auto fn){
			for(const auto& connection : inout_map_.connection){
				if(connection.slot.in == index){
					std::invoke(fn, (*this)[connection.binding]);
				}
			}
		}

		void visit_out(inout_index index, std::invocable<bound_stage_resource&> auto fn){
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
			return uniform_buffers_ | std::views::filter([](const stage_ubo& ubo){
				return ubo.set == 0;
			});
		}
	};


	struct ubo_subrange{
		std::uint32_t binding;
		std::uint32_t offset;
		std::uint32_t size;
	};

	export
	struct post_process_stage : pass_meta{
		friend post_process_graph;

	protected:
		post_process_meta meta_{};

		vk::descriptor_layout descriptor_layout_{};
		vk::pipeline_layout pipeline_layout_{};
		vk::pipeline pipeline_{};


		vk::uniform_buffer uniform_buffer_{};
		std::vector<ubo_subrange> uniform_subranges_{};

		std::unordered_map<binding_info, VkSampler> samplers_{};

		vk::descriptor_buffer descriptor_buffer_{};

	public:
		[[nodiscard]] explicit(false) post_process_stage(vk::context& ctx, post_process_meta&& meta)
			: pass_meta(ctx),
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
			return uniform_buffer_;
		}

	protected:
		void init_pipeline(const vk::context& ctx) noexcept{
			descriptor_layout_ = {ctx.get_device(), VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT, meta_.descriptor_layout_builder_};
			pipeline_layout_ = vk::pipeline_layout{ctx.get_device(), 0, {descriptor_layout_}};
			pipeline_ = vk::pipeline{ctx.get_device(), pipeline_layout_, VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT, meta_.shader_->get_create_info()};
		}

		void reset_descriptor_buffer(vk::context& ctx, std::uint32_t chunk_count = 1){
			descriptor_buffer_ = vk::descriptor_buffer{ctx.get_allocator(), descriptor_layout_, descriptor_layout_.binding_count(), chunk_count};
		}

		void init_uniform_buffer(vk::context& ctx){
			std::uint32_t ubo_size{};
			uniform_subranges_.clear();

			for (const auto & desc : meta_.get_local_uniform_buffer()){
				uniform_subranges_.push_back(ubo_subrange{desc.binding, ubo_size, static_cast<std::uint32_t>(desc.size)});
				ubo_size += ceil_div<std::uint32_t>(desc.size, 16) * 16;
			}

			if(ubo_size == 0)return;

			uniform_buffer_ = vk::uniform_buffer{ctx.get_allocator(), ubo_size};
		}

		void default_bind_uniform_buffer(){
			vk::descriptor_mapper mapper{descriptor_buffer_};

			for (const auto & uniform_offset : uniform_subranges_){
				(void)mapper.set_uniform_buffer(uniform_offset.binding, uniform_buffer_.get_address() + uniform_offset.offset, uniform_offset.size);
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
			for(const auto& connection : meta_.inout_map_.connection()){
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

		void reset_resources(vk::context& context, const pass_resource_reference& resources, const math::u32size2 extent) override{
			default_bind_uniform_buffer();
			default_bind_resources(resources);
		}

		void record_command(
			vk::context& context,
			const pass_resource_reference& resources, math::u32size2 extent, VkCommandBuffer buffer) override{

			pipeline_.bind(buffer, VK_PIPELINE_BIND_POINT_COMPUTE);
			descriptor_buffer_.bind_to(buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout_, 0);

			auto groups = get_work_group_size(extent);
			vkCmdDispatch(buffer, groups.x, groups.y, 1);
		}

		[[nodiscard]] const pass_inout_connection& sockets() const noexcept final{
			return meta_.sockets;
		}

		[[nodiscard]] pass_inout_connection& sockets() noexcept {
			return meta_.sockets;
		}

		[[nodiscard]] std::string_view get_name() const noexcept override{
			return meta_.name();
		}

		[[nodiscard]] const vk::shader_module& shader() const{
			return *meta_.shader_;
		}

	protected:
		[[nodiscard]] std::span<const inout_index> get_required_internal_buffer_slots() const noexcept override{
			return meta_.required_transient_buffer_input_slots_;
		}
	};


}