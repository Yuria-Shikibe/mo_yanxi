module;

#include <vulkan/vulkan.h>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

export module mo_yanxi.graphic.render_graph.post_process_pass;
export import mo_yanxi.graphic.render_graph.manager;

export import mo_yanxi.graphic.shader_reflect;
export import mo_yanxi.vk.shader;
export import mo_yanxi.vk.pipeline;
export import mo_yanxi.vk.pipeline.layout;
export import mo_yanxi.vk.resources;
export import mo_yanxi.vk.ext;
export import mo_yanxi.vk.context;
export import mo_yanxi.vk.uniform_buffer;
export import mo_yanxi.vk.descriptor_buffer;

import mo_yanxi.basic_util;
import std;

namespace mo_yanxi::graphic::render_graph{
	namespace resource_desc{
		constexpr VkFormat convertImageFormatToVkFormat(spv::ImageFormat imageFormat) noexcept{
			switch(imageFormat){
			case spv::ImageFormatUnknown : return VK_FORMAT_UNDEFINED;
			case spv::ImageFormatRgba8 : return VK_FORMAT_R8G8B8A8_UNORM;
			case spv::ImageFormatRgba8Snorm : return VK_FORMAT_R8G8B8A8_SNORM;
			case spv::ImageFormatRgba8ui : return VK_FORMAT_R8G8B8A8_UINT;
			case spv::ImageFormatRgba8i : return VK_FORMAT_R8G8B8A8_SINT;
			case spv::ImageFormatR32ui : return VK_FORMAT_R32_UINT;
			case spv::ImageFormatR32i : return VK_FORMAT_R32_SINT;
			case spv::ImageFormatRgba16 : return VK_FORMAT_R16G16B16A16_UNORM;
			case spv::ImageFormatRgba16Snorm : return VK_FORMAT_R16G16B16A16_SNORM;
			case spv::ImageFormatRgba16ui : return VK_FORMAT_R16G16B16A16_UINT;
			case spv::ImageFormatRgba16i : return VK_FORMAT_R16G16B16A16_SINT;
			case spv::ImageFormatRgba16f : return VK_FORMAT_R16G16B16A16_SFLOAT;
			case spv::ImageFormatR32f : return VK_FORMAT_R32_SFLOAT;
			case spv::ImageFormatRgba32ui : return VK_FORMAT_R32G32B32A32_UINT;
			case spv::ImageFormatRgba32i : return VK_FORMAT_R32G32B32A32_SINT;
			case spv::ImageFormatRgba32f : return VK_FORMAT_R32G32B32A32_SFLOAT;
			case spv::ImageFormatR8 : return VK_FORMAT_R8_UNORM;
			case spv::ImageFormatR8Snorm : return VK_FORMAT_R8_SNORM;
			case spv::ImageFormatR8ui : return VK_FORMAT_R8_UINT;
			case spv::ImageFormatR8i : return VK_FORMAT_R8_SINT;
			case spv::ImageFormatRg8 : return VK_FORMAT_R8G8_UNORM;
			case spv::ImageFormatRg8Snorm : return VK_FORMAT_R8G8_SNORM;
			case spv::ImageFormatRg8ui : return VK_FORMAT_R8G8_UINT;
			case spv::ImageFormatRg8i : return VK_FORMAT_R8G8_SINT;
			case spv::ImageFormatR16 : return VK_FORMAT_R16_UNORM;
			case spv::ImageFormatR16Snorm : return VK_FORMAT_R16_SNORM;
			case spv::ImageFormatR16ui : return VK_FORMAT_R16_UINT;
			case spv::ImageFormatR16i : return VK_FORMAT_R16_SINT;
			case spv::ImageFormatR16f : return VK_FORMAT_R16_SFLOAT;
			case spv::ImageFormatRg16 : return VK_FORMAT_R16G16_UNORM;
			case spv::ImageFormatRg16Snorm : return VK_FORMAT_R16G16_SNORM;
			case spv::ImageFormatRg16ui : return VK_FORMAT_R16G16_UINT;
			case spv::ImageFormatRg16i : return VK_FORMAT_R16G16_SINT;
			case spv::ImageFormatRg16f : return VK_FORMAT_R16G16_SFLOAT;
			case spv::ImageFormatR64ui : return VK_FORMAT_R64_UINT;
			case spv::ImageFormatR64i : return VK_FORMAT_R64_SINT;
			default : return VK_FORMAT_UNDEFINED;
			}
		}

		std::size_t get_buffer_size(
			const spirv_cross::Compiler& compiler,
			const spirv_cross::Resource& resource){
			const spirv_cross::SPIRType& type = compiler.get_type(resource.base_type_id);

			std::size_t size = compiler.get_declared_struct_size(type);

			if(!type.array.empty()){
				for(uint32_t array_size : type.array){
					if(array_size == 0){
						break;
					}
					size *= array_size;
				}
			}

			return size;
		}

		image_requirement extract_image_state(
				const spirv_cross::CompilerGLSL& complier,
				const spirv_cross::Resource& resource)
		{
			decoration decr{};
			auto& type = complier.get_type(resource.type_id);

			bool no_read = complier.get_decoration(resource.id, spv::DecorationNonReadable);
			bool no_write = complier.get_decoration(resource.id, spv::DecorationNonWritable);

			// if(type.image.sampled == 1){
			// 	(void)0;
			// }

			if(!no_read && !no_write){
				decr = decoration::read | decoration::write;
			} else if(no_read){
				decr = decoration::write;
			} else{
				decr = decoration::read;
			}

			return image_requirement{
				.decr = decr,
				.sample_count = type.image.sampled == 1,
				.format = convertImageFormatToVkFormat(type.image.format),
				.dim = static_cast<unsigned>(type.image.dim == spv::Dim1D
												 ? 1
												 : type.image.dim == spv::Dim2D
												 ? 2
												 : type.image.dim == spv::Dim3D
												 ? 3
												 : 0),
			};
		}
	}

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
				uniform_buffers_.push_back({binding, resource_desc::get_buffer_size(refl.compiler(), input)});
				if(binding.set != 0)continue;
				descriptor_layout_builder_.push(binding.binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
			}

			for(const auto& input : refl.resources().storage_buffers){
				auto binding = refl.binding_info_of(input);
				resources_.push_back({binding, resource_desc::buffer_requirement{resource_desc::get_buffer_size(refl.compiler(), input)}});
				if(binding.set != 0)continue;
				descriptor_layout_builder_.push(binding.binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
			}


			for(const auto& pass : inout_map.connection()){
				if(!std::ranges::contains(resources_, pass.binding)){
					throw std::invalid_argument("binding not match");
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
			for(const auto& connection : inout_map_.connection()){
				if(connection.slot.in == index){
					std::invoke(fn, (*this)[connection.binding].get<ArgTy>());
				}
			}
		}

		template <typename Fn>
		void visit_out(inout_index index, Fn fn){
			using ArgTy = std::remove_cvref_t<typename function_arg_at_last<Fn>::type>;
			for(const auto& connection : inout_map_.connection()){
				if(connection.slot.out == index){
					std::invoke(fn, (*this)[connection.binding].get<ArgTy>());
				}
			}
		}



		void set_format_at_in(std::uint32_t slot, VkFormat format) {
			sockets.at_in<resource_desc::image_requirement>(slot).format = format;
		}
		void set_format_at_out(std::uint32_t slot, VkFormat format) {
			sockets.at_out<resource_desc::image_requirement>(slot).format = format;
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
		friend render_graph_manager;

	protected:
		post_process_meta meta_{};

		vk::descriptor_layout descriptor_layout_{};
		vk::pipeline_layout pipeline_layout_{};
		vk::pipeline pipeline_{};


		vk::uniform_buffer uniform_buffer_{};
		std::vector<ubo_subrange> uniform_subranges_{};

		std::unordered_map<binding_info, VkSampler> samplers_{};

		vk::descriptor_buffer descriptor_buffer_{};

		std::vector<external_descriptor_usage> external_descriptor_usages_{};

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

		[[nodiscard]] const vk::uniform_buffer& ubo() const {
			return uniform_buffer_;
		}

		void add_external_descriptor(const external_descriptor& entry, std::uint32_t setIdx, VkDeviceSize offset = 0){
			if(setIdx == 0)throw std::invalid_argument{"[0] is reserved for local descriptors"};

			auto itr = std::ranges::lower_bound(external_descriptor_usages_, setIdx, {}, &external_descriptor_usage::set_index);
			if(itr == external_descriptor_usages_.end()){
				external_descriptor_usages_.emplace_back(&entry, setIdx, offset);
			}else{
				if(itr->set_index == setIdx){
					throw std::invalid_argument{"Duplicate descriptor set index"};
				}

				external_descriptor_usages_.emplace(itr, &entry, setIdx, offset);
			}
		}


	protected:
		void init_pipeline(const vk::context& ctx) noexcept{
			descriptor_layout_ = {ctx.get_device(), VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT, meta_.descriptor_layout_builder_};
			if(external_descriptor_usages_.empty()){
				pipeline_layout_ = vk::pipeline_layout{ctx.get_device(), 0, {descriptor_layout_}};
			}else{
				std::vector<VkDescriptorSetLayout> layouts{};
				layouts.reserve(1 + external_descriptor_usages_.size());
				layouts.push_back(descriptor_layout_);
				layouts.append_range(external_descriptor_usages_ | std::views::transform([](const external_descriptor_usage& u){
					return u.entry->layout;
				}));
				pipeline_layout_ = vk::pipeline_layout{ctx.get_device(), 0, layouts};
			}

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
				if(connection.binding.set != 0)continue;

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
			if(external_descriptor_usages_.empty()){
				descriptor_buffer_.bind_to(buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout_, 0);
			}else{
				std::array<std::byte, 1024> buf;
				std::pmr::monotonic_buffer_resource pool{buf.data(), buf.size(), std::pmr::new_delete_resource()};
				std::pmr::vector<VkDescriptorBufferBindingInfoEXT> infos(external_descriptor_usages_.size() + 1, &pool);

				infos[0] = descriptor_buffer_;

				std::pmr::vector<VkDeviceSize> offsets(infos.size(), &pool);
				std::pmr::vector<std::uint32_t> indices(std::from_range, std::views::iota(0U) | std::views::take(infos.size()), &pool);

				for(std::uint32_t i = 0; i < external_descriptor_usages_.size(); ++i){
					const auto setIdx = i + 1;
					const auto itr = std::ranges::lower_bound(external_descriptor_usages_, setIdx, {}, &external_descriptor_usage::set_index);
					if(itr == external_descriptor_usages_.end() || itr->set_index != setIdx){
						throw std::out_of_range("failed to find external descriptor usage, set index must be contiguous from 1");
					}
					infos[setIdx] = itr->entry->dbo_info;
					offsets[setIdx] = itr->offset;
				}

				vk::cmd::bindDescriptorBuffersEXT(buffer, infos.size(), infos.data());
				vk::cmd::setDescriptorBufferOffsetsEXT(buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout_, 0, infos.size(), indices.data(), offsets.data());
			}


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

		[[nodiscard]] post_process_meta& meta() {
			return meta_;
		}


	protected:
		[[nodiscard]] std::span<const inout_index> get_required_internal_buffer_slots() const noexcept override{
			return meta_.required_transient_buffer_input_slots_;
		}
	};


}