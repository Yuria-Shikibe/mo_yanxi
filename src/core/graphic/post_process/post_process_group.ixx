module;

#include <vulkan/vulkan.h>

#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include "plf_hive.h"

// #include
// #include <gch/small_vector.hpp>

#include "../src/ext/enum_operator_gen.hpp"

export module mo_yanxi.graphic.post_process_graph;



export import mo_yanxi.vk.context;
import mo_yanxi.vk.shader;
import mo_yanxi.vk.image_derives;
import mo_yanxi.vk.command_buffer;
import mo_yanxi.vk.resources;
import mo_yanxi.graphic.shader_reflect;
import mo_yanxi.meta_programming;
import mo_yanxi.basic_util;

import mo_yanxi.math.vector2;
import mo_yanxi.referenced_ptr;

import std;

namespace mo_yanxi::graphic{

	 constexpr VkPipelineStageFlags2 deduce_stage(VkImageLayout layout) noexcept {
		switch(layout){
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : return VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL : return VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : return VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : return VK_PIPELINE_STAGE_2_TRANSFER_BIT;

		default : return VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		}
	}

	constexpr VkAccessFlags2 deduce_external_image_access(VkPipelineStageFlags2 stage) noexcept {
	 	//TODO use bit_or to merge, instead of individual test?
	 	switch(stage){
	 		case VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT : return VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	 		case VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT : return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	 		default: return VK_ACCESS_2_NONE;
	 	}
	}


	export constexpr VkFormat convertImageFormatToVkFormat(spv::ImageFormat imageFormat) noexcept{
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

	export std::size_t get_buffer_size(
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


	// using post_process_id = unsigned;
	using shader_binary = std::span<const std::uint32_t>;

	export using inout_index = unsigned;
	export constexpr inout_index no_slot = std::numeric_limits<inout_index>::max();


	namespace resource_desc{
		export
		enum struct resource_type : unsigned{
			image, buffer, unknown
		};

		export
		enum struct image_decr{
			unknown,
			sampler = 1,
			read = 1 << 1,
			write = 1 << 2,
		};

		BITMASK_OPS(export, image_decr);

		export
		enum struct ownership_type{
			shared,
			exclusive,
			external
		};

		export constexpr unsigned dependent = std::numeric_limits<unsigned>::max();


		export
		using image_req_index = unsigned;

		export
		constexpr inline image_req_index no_req_spec = std::numeric_limits<image_req_index>::max();

		export
		template <typename T>
		T value_or(T val, T alternative) noexcept{
			if(val == no_req_spec){
				return alternative;
			}
			return val;
		}

		export
		struct image_desc{
			image_decr decoration{};
			VkFormat format{VK_FORMAT_UNDEFINED};
			unsigned dim{no_req_spec};

			[[nodiscard]] VkImageUsageFlags get_required_usage() const noexcept{
				VkImageUsageFlags rst{};
				if(decoration == image_decr::sampler){
					rst |= VK_IMAGE_USAGE_SAMPLED_BIT;
				} else{
					rst |= VK_IMAGE_USAGE_STORAGE_BIT;
				}

				return rst;
			}
		};


		export
		struct ubo_desc{
			std::size_t size;
		};

		export
		struct buffer_desc{
			std::size_t element_size;
		};

		export
		struct stage_resource_desc{
			using variant_tuple = std::tuple<image_desc, buffer_desc>;
			using variant_t = std::variant<image_desc, buffer_desc>;
			variant_t desc{};

			template <typename T>
				requires std::constructible_from<variant_t, T&&>
			[[nodiscard]] explicit(false) stage_resource_desc(T&& desc)
				: desc(std::forward<T>(desc)){
			}

			bool is(resource_type type) const noexcept{
				return desc.index() == std::to_underlying(type);
			}

			resource_type type() const noexcept{
				return resource_type{static_cast<std::underlying_type_t<resource_type>>(desc.index())};
			}

			bool is_type_valid_equal_to(const stage_resource_desc& other) const noexcept{
				return desc.index() == other.desc.index();
			}

			template <typename T>
				requires contained_in<T, variant_tuple>
			auto get_if() const noexcept{
				return std::get_if<T>(&desc);
			}

			template <typename T>
				requires contained_in<T, variant_tuple>
			auto& get() const noexcept{
				return std::get<T>(desc);
			}
		};

		export
		template <typename T, std::predicate<const T&, const T&> Pr = std::less<T>>
		T get_optional_max(T l, T r, T optional_spec_value, Pr pred = {}) noexcept {
			if(l == optional_spec_value && r == optional_spec_value)return optional_spec_value;
			if(l == optional_spec_value) return r;
			if(r == optional_spec_value) return l;
			return std::max(l, r, pred);
		}

		export
		struct image_requirement{
			image_desc desc{};
			VkImageUsageFlags usage{};

			VkImageAspectFlags aspect_flags{};
			/**
			 * @brief explicitly specfied expected initial layout of the image
			 */
			VkImageLayout override_layout{};

			/**
			 * @brief explicitly specfied expected final layout of the image
			 */
			VkImageLayout override_output_layout{};


			unsigned mip_levels{no_req_spec};
			unsigned scaled_times{no_req_spec};

			VkImageAspectFlags get_aspect() const noexcept{
				return aspect_flags ? aspect_flags : VK_IMAGE_ASPECT_COLOR_BIT;
			}

			void promote(const image_requirement& other){
				desc.dim = std::max(desc.dim, other.desc.dim);
				desc.decoration |= other.desc.decoration;

				mip_levels = get_optional_max(mip_levels, other.mip_levels, no_req_spec);
				if(desc.format == VK_FORMAT_UNDEFINED)desc.format = other.desc.format;
				assert(desc.format == other.desc.format || other.desc.format == VK_FORMAT_UNDEFINED);
				// ownership = ownership_type{std::max(std::to_underlying(ownership), std::to_underlying(other.ownership))};
				usage |= other.usage;
				aspect_flags |= other.aspect_flags;
			}

			bool is_sampled_image() const noexcept{
				return desc.decoration == image_decr::sampler;
			}

			[[nodiscard]] VkImageLayout get_expected_layout() const noexcept{
				if(override_layout)return override_layout;
				return desc.decoration == image_decr::sampler
						   ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
						   : VK_IMAGE_LAYOUT_GENERAL;
			}

			[[nodiscard]] VkImageLayout get_expected_layout_on_output() const noexcept{
				if(override_output_layout)return override_output_layout;
				return get_expected_layout();
			}


			[[nodiscard]] VkAccessFlags2 get_image_access(VkPipelineStageFlags2 pipelineStageFlags2) const noexcept{
				switch(pipelineStageFlags2){
				case VK_PIPELINE_STAGE_2_TRANSFER_BIT : switch(desc.decoration){
					case image_decr::read : return VK_ACCESS_2_TRANSFER_READ_BIT;
					case image_decr::write : return VK_ACCESS_2_TRANSFER_WRITE_BIT;
					case image_decr::read | image_decr::write : return VK_ACCESS_2_TRANSFER_READ_BIT |
							VK_ACCESS_2_TRANSFER_WRITE_BIT;
					default : return VK_ACCESS_2_NONE;
					}


				default : switch(desc.decoration){
					case image_decr::sampler : return VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
					case image_decr::read : return VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
					case image_decr::write : return VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
					case image_decr::read | image_decr::write : return VK_ACCESS_2_SHADER_STORAGE_READ_BIT |
							VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
					default : return VK_ACCESS_2_NONE;
					}
				}

			}

			[[nodiscard]] image_requirement max(const image_requirement& other_in_same_partition) const noexcept{
				image_requirement cpy{*this};
				cpy.promote(other_in_same_partition);
				return cpy;
			}

			[[nodiscard]] std::strong_ordering compatibility_three_way_compare(const image_requirement& other_in_same_partition) const noexcept{
				return connect_three_way_result(
					compare_bitflags(usage, other_in_same_partition.usage),
					compare_bitflags(aspect_flags, other_in_same_partition.aspect_flags),
					mip_levels <=> other_in_same_partition.mip_levels,
					desc.dim <=> other_in_same_partition.desc.dim
				);
			}
		};

		export
		struct buffer_requirement{
			VkDeviceSize size;
			VkAccessFlagBits2 access{VK_ACCESS_2_NONE};

			void promote(const buffer_requirement& other){
				size = std::max(size, other.size);
			}


			[[nodiscard]] std::strong_ordering compatibility_three_way_compare(const buffer_requirement& other_in_same_partition) const noexcept{
				return size <=> other_in_same_partition.size;
			}
		};

		export
		struct resource_requirement{
			using variant_tuple = std::tuple<image_requirement, buffer_requirement>;
			using variant_t = std::variant<image_requirement, buffer_requirement>;
			variant_t desc{};

			[[nodiscard]] resource_type type() const noexcept{
				return resource_type{static_cast<std::underlying_type_t<resource_type>>(desc.index())};
			}

			[[nodiscard]] std::string_view type_name() const noexcept{
				return type() == resource_type::buffer ? "Buffer" : "Image";
			}

			[[nodiscard]] bool is_type_valid_equal_to(const resource_requirement& req) const noexcept{
				return desc.index() == req.desc.index();
			}

			/**
			 * @brief
			 * @return error message if any, or nullopt if compatible
			 */
			[[nodiscard]] std::optional<std::string_view> get_incompatible_info(const resource_requirement& req) const noexcept{
				if(!is_type_valid_equal_to(req)){
					return "Type Incompatible";
				}
				using ret = std::optional<std::string_view>;
				return std::visit<ret>(overload_def_noop{
					std::in_place_type<ret>,
					[](const image_requirement& l, const image_requirement& r) -> ret {
						if(l.aspect_flags != r.aspect_flags && (l.aspect_flags != 0 && r.aspect_flags != 0)){
							return "Aspect Incompatible";
						}

						if(
							l.scaled_times != r.scaled_times && (l.scaled_times != no_req_spec && r.scaled_times != no_req_spec)
						){
							return "Size Incompatible";
						}

						//TODO format compatibility check, consider sampler and other...
						if(l.desc.format != r.desc.format){
							return "Format Incompatible";
						}

						return std::nullopt;
					},
					[](const buffer_requirement& l, const buffer_requirement& r) -> ret {
						return std::nullopt;
					}
				}, desc, req.desc);
			}

			void promote(const resource_requirement& other){
				std::visit(overload_narrow{
					[](image_requirement& l, const image_requirement& r) {
						l.promote(r);
					},
					[](buffer_requirement& l, const buffer_requirement& r) {
						l.promote(r);
					}
				}, desc, other.desc);
			}



			[[nodiscard]] resource_requirement() = default;

			template <typename T>
				requires std::constructible_from<variant_t, T&&>
			[[nodiscard]] explicit(false) resource_requirement(T&& desc)
				: desc(std::forward<T>(desc)){
			}

			[[nodiscard]] explicit(false) resource_requirement(const stage_resource_desc& sdesc){
				std::visit(overload{
					[](std::monostate){},
					[this](const image_desc& idesc){
						auto& ty = desc.emplace<image_requirement>(idesc);
						ty.usage = idesc.get_required_usage();
					},
					[this](const buffer_desc&){
						desc.emplace<buffer_requirement>();
					}
				}, sdesc.desc);
			}

			bool is(resource_type type) const noexcept{
				return desc.index() == std::to_underlying(type);
			}

			template <typename T>
				requires contained_in<T, variant_tuple>
			auto get_if() const noexcept{
				return std::get_if<T>(&desc);
			}

			template <typename T, typename S>
				requires contained_in<T, variant_tuple>
			auto& get(this S& self) noexcept{
				return std::get<T>(self.desc);
			}

			bool is_compatibility_part_equal_to(const resource_requirement& other_in_same_partition) const noexcept{
				return std::visit<bool>(overload_def_noop{
					std::in_place_type<bool>,
					[](const image_requirement& l, const image_requirement& r) static {
						return std::is_eq(l.compatibility_three_way_compare(r));
					},
					[](const buffer_requirement& l, const buffer_requirement& r) static  {
						return std::is_eq(l.compatibility_three_way_compare(r));
					}
				}, this->desc, other_in_same_partition.desc);
			}

		};

		export
		struct resource_requirement_partial_greater_comp{

			 static bool operator()(const resource_requirement& l, const resource_requirement& o) noexcept{
				if(l.type() != o.type()){
					return l.type() > o.type();
				}

			 	return std::is_gt(std::visit<std::strong_ordering>(overload_def_noop{
					 std::in_place_type<std::strong_ordering>,
					 [](const image_requirement& l, const image_requirement& r) static {
						 return l.compatibility_three_way_compare(r);
					 },
					 [](const buffer_requirement& l, const buffer_requirement& r) static  {
						 return l.compatibility_three_way_compare(r);
					 }
				 }, l.desc, o.desc));
			}

			// static bool operator==(const resource_requirement& l, const resource_requirement& o) noexcept{
			// 	return l.is_compatibility_part_equal_to(o);
			// }
		};


		export
		image_desc extract_image_state(const spirv_cross::CompilerGLSL& complier,
		                               const spirv_cross::Resource& resource){
			image_decr decr{};
			auto& type = complier.get_type(resource.type_id);

			bool no_read = complier.get_decoration(resource.id, spv::DecorationNonReadable);
			bool no_write = complier.get_decoration(resource.id, spv::DecorationNonWritable);

			if(type.image.sampled != 1){
				if(!no_read && !no_write){
					decr = image_decr::read | image_decr::write;
				} else if(no_read){
					decr = image_decr::write;
				} else{
					decr = image_decr::read;
				}
			} else{
				decr = image_decr::sampler;
			}

			return image_desc{
					.decoration = decr,
					// .format = convertImageFormatToVkFormat(type.image.format),
					.dim = static_cast<unsigned>(type.image.dim == spv::Dim1D
						                             ? 1
						                             : type.image.dim == spv::Dim2D
						                             ? 2
						                             : type.image.dim == spv::Dim3D
						                             ? 3
						                             : 0),
				};
		}

		export
		template <typename T>
		struct stage_data : binding_info{
			T desc;
		};

		export using stage_image = stage_data<image_desc>;
		export using stage_ubo = stage_data<ubo_desc>;
		export using stage_sbo = stage_data<ubo_desc>;




		export
		enum struct inout_type : unsigned{
			in, out, inout
		};

		export
		struct bound_stage_resource : stage_resource_desc, binding_info{
		};


		export
		struct image_entity{
			vk::image_handle image{};
			vk::storage_image handle{};

			[[nodiscard]] bool is_local() const noexcept{
				return static_cast<bool>(handle);
			}

			void allocate(vk::context& context, VkExtent2D extent2D, const image_requirement& requirement){
				auto div_times = value_or<std::uint32_t>(requirement.scaled_times, 0);
				extent2D.width >>= div_times;
				extent2D.height >>= div_times;

				handle = vk::storage_image{
					context.get_allocator(),
					extent2D,
					vk::get_recommended_mip_level(extent2D, value_or<std::uint32_t>(requirement.mip_levels, 1u)),
					requirement.desc.format == VK_FORMAT_UNDEFINED ? VK_FORMAT_R8G8B8A8_UNORM : requirement.desc.format,
					requirement.usage
				};

				image = handle;
			}

		};

		export
		struct buffer_entity{
			vk::storage_buffer handle{};
			vk::buffer_borrow buffer{};

			void allocate(vk::context& context, VkDeviceSize size){
				handle = vk::storage_buffer{context.get_allocator(), size, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT};

				buffer = handle.borrow();
			}
		};

		export
		struct resource_entity{
			using variant_t = std::variant<image_entity, buffer_entity>;
			resource_requirement overall_req{};
			variant_t resource{};

			[[nodiscard]] resource_entity() = default;

			[[nodiscard]] explicit resource_entity(const resource_requirement& requirement)
				: overall_req(requirement){
				switch(requirement.type()){
				case resource_type::image : resource.emplace<image_entity>();
					break;
				case resource_type::buffer : resource.emplace<buffer_entity>();
					break;
				default: throw std::runtime_error("Incompatible resource type");
				}
			}

			auto& to_image(this auto& self){
				return std::get<resource_desc::image_entity>(self.resource);
			}

			void allocate_buffer(vk::context& context, VkDeviceSize size){
				if(auto res = std::get_if<buffer_entity>(&resource)){
					res->allocate(context, size);
				}
			}

			void allocate_image(vk::context& context, VkExtent2D extent){
				if(auto res = std::get_if<image_entity>(&resource)){
					res->allocate(context, extent, overall_req.get<image_requirement>());
				}
			}

			[[nodiscard]] resource_type type() const noexcept{
				return overall_req.type();
			}

			image_entity& as_image(){
				return std::get<image_entity>(resource);
			}

			buffer_entity& as_buffer(){
				return std::get<buffer_entity>(resource);
			}
		};


		export
		struct external_image{
			VkImageLayout expected_layout{};
			vk::image_handle handle{};
		};

		export
		struct external_buffer{
			vk::buffer_borrow handle{};
		};

		export
		struct external_resource_dependency{
			VkPipelineStageFlags2    src_stage;
			VkAccessFlags2           src_access;
			VkPipelineStageFlags2    dst_stage;
			VkAccessFlags2           dst_access;
		};

		export
		struct explicit_resource : referenced_object_base{
			using variant_t = std::variant<external_image, external_buffer, std::monostate>;

			variant_t desc{std::monostate{}};
			external_resource_dependency dependency{};

			[[nodiscard]] explicit_resource() = default;

			[[nodiscard]] explicit explicit_resource(const variant_t& desc)
				: desc(desc){
			}

			[[nodiscard]] explicit_resource(const variant_t& desc, const external_resource_dependency& dependency)
				: desc(desc),
				  dependency(dependency){
			}

			[[nodiscard]] bool is_external_spec() const noexcept{
				return type() != resource_type::unknown;
			}

			[[nodiscard]] resource_type type() const noexcept{
				return resource_type{static_cast<std::underlying_type_t<resource_type>>(desc.index())};
			}

			[[nodiscard]] VkImageLayout get_layout() const noexcept{
				if(auto img = std::get_if<external_image>(&desc)){
					return img->expected_layout;
				}
				return VK_IMAGE_LAYOUT_UNDEFINED;
			}

			// void set_from_entity(const resource_entity& entity){
			// 	std::visit(narrow_overload{
			// 		[](external_image& image, const image_entity& entity){
			// 			image.handle = entity.image;
			// 		},
			// 		[](external_buffer& buffer, const buffer_entity& entity){
			// 			buffer.handle = entity.buffer;
			// 		}
			// 	}, desc, entity.resource);
			// }
		};

		export
		struct explicit_resource_usage{
			referenced_ptr<explicit_resource, false> resource{};
			inout_index slot{no_slot};

			[[nodiscard]] explicit_resource_usage() = default;

			[[nodiscard]] explicit_resource_usage(explicit_resource& resource,
				inout_index slot)
				: resource(resource),
				  slot(slot){
			}

			[[nodiscard]] ownership_type get_ownership() const noexcept{
				if(resource->is_external_spec()){
					return ownership_type::external;
				}else{
					return resource->get_ref_count() > 1 ? ownership_type::shared : ownership_type::exclusive;
				}
			}

			bool is_local() const noexcept{
				return !resource->is_external_spec();
			}
		};

		export
		struct image_entity_state{
			VkImageLayout current_layout{};
		};

		export
		struct buffer_entity_state{

		};

		export
		struct entity_state{
			using variant_t = std::variant<image_entity_state, buffer_entity_state>;
			variant_t desc{};

			VkPipelineStageFlags2 last_stage{VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT};
			VkAccessFlags2 last_access{VK_ACCESS_2_NONE};
			bool external_init_required{};

			[[nodiscard]] resource_type type() const noexcept{
				return static_cast<resource_type>(desc.index());
			}

			[[nodiscard]] entity_state() = default;

			[[nodiscard]] explicit(false) entity_state(const explicit_resource& ext){
				std::visit(overload_narrow{
					[this](const external_image& image){
						auto& s = desc.emplace<image_entity_state>();
						s.current_layout = image.expected_layout;
					},
					[](const external_buffer& buffer){

					}
				}, ext.desc);
			}

			VkImageLayout get_layout() const noexcept{
				return std::get<image_entity_state>(desc).current_layout;
			}
		};
	}

	export
	struct slot_pair{
		inout_index in{no_slot};
		inout_index out{no_slot};


		[[nodiscard]] bool is_inout() const noexcept{
			return in != no_slot && out != no_slot;
		}

		[[nodiscard]] bool has_in() const noexcept{
			return in != no_slot;
		}

		[[nodiscard]] bool has_out() const noexcept{
			return out != no_slot;
		}

		friend bool operator==(const slot_pair& lhs, const slot_pair& rhs) noexcept = default;
	};
}

template <>
struct std::hash<mo_yanxi::graphic::slot_pair>{
	static std::size_t operator()(mo_yanxi::graphic::slot_pair pair) noexcept{
		return std::hash<unsigned long long>{}(std::bit_cast<unsigned long long>(pair));
	}
};

namespace mo_yanxi::graphic{

	export
	struct resource_map_entry{
		binding_info binding{};
		slot_pair slot{};

		[[nodiscard]] bool is_inout() const noexcept{
			return slot.is_inout();
		}
	};

	export
	struct post_process_stage_inout_map{
		// private:
		std::vector<resource_map_entry> connection{};

	public:
		[[nodiscard]] post_process_stage_inout_map() = default;

		[[nodiscard]] post_process_stage_inout_map(std::initializer_list<resource_map_entry> map)
			: connection(map){
#if DEBUG_CHECK
			std::unordered_set<binding_info> bindings{};
			bindings.reserve(connection.size());
			for(const auto& pass : connection){
				if(!bindings.insert(pass.binding).second){
					assert(false);
				}
			}
#endif
		}

		std::optional<slot_pair> operator[](binding_info binding) const noexcept{
			if(auto itr = std::ranges::find(connection, binding, &resource_map_entry::binding); itr != connection.
				end()){
				return itr->slot;
			}

			return std::nullopt;
		}


		void compact_check(){
			if(connection.empty()){
				return;
			}


			auto checker = [this](auto transform){
				std::ranges::sort(connection, {}, transform);
				if(transform(connection.front()) != no_slot && transform(connection.front()) != 0) return false;

				for(auto [l, r] : connection | std::views::transform(transform) | std::views::take_while(
					    [](inout_index v){ return v != no_slot; }) | std::views::adjacent<2>){
					if(r - l > 1){
						return false;
					}
				}

				return true;
			};

			if(!checker([](const resource_map_entry& e){ return e.slot.in; })){
				throw std::invalid_argument("Invalid resource map entry");
			}

			if(!checker([](const resource_map_entry& e){ return e.slot.out; })){
				throw std::invalid_argument("Invalid resource map entry");
			}

			std::ranges::sort(connection, std::ranges::greater{}, &resource_map_entry::is_inout);

			//TODO check no inout overlap?
		}
	};

	using inout_type = resource_desc::inout_type;

	struct categorized_data_indices{
		struct inout_pair{
			unsigned in;
			unsigned out;
		};

		std::vector<inout_pair> in_out{};
		std::vector<unsigned> in{};
		std::vector<unsigned> out{};
	};

	export
	struct inout_data{
		static constexpr std::size_t sso = 4;
		using index_slot_type = std::vector<inout_index>;

		std::vector<resource_desc::ubo_desc> uniform_buffers{};

		std::vector<resource_desc::resource_requirement> data{};
		index_slot_type input_slots{};
		index_slot_type output_slots{};

	private:
		void resize_and_set_in(inout_index idx, std::size_t desc_index){
			input_slots.resize(std::max<std::size_t>(input_slots.size(), idx + 1), no_slot);
			input_slots[idx] = desc_index;
		}

		void resize_and_set_out(inout_index idx, std::size_t desc_index){
			output_slots.resize(std::max<std::size_t>(output_slots.size(), idx + 1), no_slot);
			output_slots[idx] = desc_index;
		}

		[[nodiscard]] bool has_index(inout_index idx, index_slot_type inout_data::* mptr) const noexcept{
			if((this->*mptr).size() <= idx) return false;
			return (this->*mptr)[idx] != no_slot;
		}

	public:
		template <typename T, typename S>
		auto& at_in(this S&& self, inout_index index) noexcept{
			return self.data.at(self.input_slots.at(index)).template get<T>();
		}

		template <typename T, typename S>
		auto& at_out(this S&& self, inout_index index) noexcept{
			return self.data.at(self.output_slots.at(index)).template get<T>();
		}


		template <typename S>
		auto& at_in(this S&& self, inout_index index) noexcept{
			return self.data.at(self.input_slots.at(index));
		}

		template <typename S>
		auto& at_out(this S&& self, inout_index index) noexcept{
			return self.data.at(self.output_slots.at(index));
		}

		[[nodiscard]] std::optional<resource_desc::resource_requirement> get_out(inout_index outIdx) const noexcept{
			if(outIdx >= output_slots.size()){
				return std::nullopt;
			}
			auto sidx = output_slots[outIdx];
			if(sidx >= data.size()) return std::nullopt;
			return data[sidx];
		}

		[[nodiscard]] std::optional<resource_desc::resource_requirement> get_in(inout_index inIdx) const noexcept{
			if(inIdx >= input_slots.size()){
				return std::nullopt;
			}
			auto sidx = input_slots[inIdx];
			if(sidx >= data.size()) return std::nullopt;
			return data[sidx];
		}

	private:
		auto get_valid_of(index_slot_type inout_data::* which) const noexcept {
			return (this->*which) | std::views::enumerate | std::views::filter([](auto&& t){
				auto&& [idx, v] = t;
				return v != no_slot;
			});
		}

	public:

		auto get_valid_in() const noexcept{
			return get_valid_of(&inout_data::input_slots);
		}

		auto get_valid_out() const noexcept{
			return get_valid_of(&inout_data::output_slots);
		}


		template <typename T>
			requires std::constructible_from<resource_desc::resource_requirement, T&&>
		void add(resource_desc::inout_type type, T&& val){
			auto sz = data.size();
			data.push_back(std::forward<T>(val));
			switch(type){
			case inout_type::in : input_slots.push_back(sz);
				break;
			case inout_type::out : output_slots.push_back(sz);
				break;
			case inout_type::inout :
				input_slots.push_back(sz);
				output_slots.push_back(sz);
				break;
			default : break;
			}
		}


		template <typename T>
			requires std::constructible_from<resource_desc::resource_requirement, T&&>
		void add(resource_map_entry type, T&& val){
			if(has_index(type.slot.in, &inout_data::input_slots) ||
				has_index(type.slot.out, &inout_data::output_slots)) return;

			const auto sz = data.size();
			data.push_back(std::forward<T>(val));

			if(type.slot.in != no_slot){
				resize_and_set_in(type.slot.in, sz);
			}

			if(type.slot.out != no_slot){
				resize_and_set_out(type.slot.out, sz);
			}
		}

		[[nodiscard]] categorized_data_indices get_categorized_data_indices() const{
			categorized_data_indices rst;
			for(auto&& [idx, data_idx] : output_slots | std::views::enumerate){
				if(auto itr = std::ranges::find(input_slots, data_idx); itr != input_slots.end()){
					rst.in_out.push_back({
							static_cast<unsigned>(itr - input_slots.begin()),
							static_cast<unsigned>(idx)
						});
				} else{
					rst.out.push_back(idx);
				}
			}

			for(auto&& [idx, data_idx] : input_slots | std::views::enumerate){
				if(!std::ranges::contains(rst.in_out, idx, &categorized_data_indices::inout_pair::in)){
					rst.in.push_back(idx);
				}
			}

			return rst;
		}
	};

	template <typename V, typename T>
	void resize_to_fit_and_set(V& range, std::size_t index, T&& val){
		auto max = std::max<std::size_t>(std::ranges::size(range), index);
		range.resize(max);
		range[index] = std::forward<T>(val);
	}

	export
	struct pass;

	export
	struct pass_dependency{
		pass* id;
		inout_index src_idx;
		inout_index dst_idx;
	};

	namespace post_graph{
		struct pass_identity{
			const pass* where{};
			const pass* external_target{nullptr};
			inout_index slot_in{no_slot};
			inout_index slot_out{no_slot};

			const pass* operator->() const noexcept{
				return where;
			}

			bool operator==(const pass_identity& i) const noexcept{
				if(where == i.where){
					if(where == nullptr){
						return external_target == i.external_target && slot_out == i.slot_out && slot_in == i.slot_in;
					}else{
						return true;
					}
				}

				return false;
			}

			std::string format(std::string_view endpoint_name = "Tml") const;
		};


		struct resource_lifebound{
			resource_desc::resource_requirement requirement{};


			resource_desc::ownership_type ownership{};

			// bool external_spec{false};
			std::vector<pass_identity> passed_by{};

			resource_desc::resource_entity *used_entity{nullptr};

			[[nodiscard]] explicit resource_lifebound(const resource_desc::resource_requirement& requirement)
				: requirement(requirement){
			}
			//
			// [[nodiscard]] bool done() const noexcept{
			// 	return get_head().slot_in == no_slot;
			// }

			[[nodiscard]] inout_index source_out() const noexcept{
				return get_head().slot_out;
			}

			[[nodiscard]] pass_identity get_head() const noexcept{
				return passed_by.front();
			}
			//
			// [[nodiscard]] bool from_external() const noexcept{
			// 	return passed_by.back().stage == nullptr;
			// }
		};

		struct record_pair{
			inout_index index{};
			resource_lifebound* life_bound{};
		};

		struct pass_identity_hasher{
			static std::size_t operator()(const pass_identity& idt) noexcept{
				if(idt.where){
					auto p0 = std::bit_cast<std::uintptr_t>(idt.where);
					return std::hash<std::size_t>{}(p0);
				}else{
					auto p1 = std::bit_cast<std::size_t>(idt.external_target) ^ static_cast<std::size_t>(idt.slot_out) ^ (static_cast<std::size_t>(idt.slot_in) << 31);
					return std::hash<std::size_t>{}(p1);
				}
			}
		};

		struct life_bounds{
			std::vector<resource_lifebound> bounds;

			/**
			 * @brief Merge lifebounds with the same source
			 */
			void unique(){
				auto& range = bounds;
				struct full_eq{
					static bool operator()(const pass_identity& l, const pass_identity& r) noexcept {
						return l.where == r.where && l.external_target == r.external_target && l.slot_out == r.slot_out;
					}
				};
				std::unordered_map<pass_identity, resource_lifebound*, pass_identity_hasher, full_eq> checked_outs{};
				auto itr = std::ranges::begin(range);
				auto end = std::ranges::end(range);
				while(itr != end){
					auto& indices = checked_outs[itr->get_head()];

					if(indices != nullptr){
						const auto view = itr->passed_by;
						const auto mismatch = std::ranges::mismatch(
							indices->passed_by, view);
						for(auto& pass : std::ranges::subrange{mismatch.in2, view.end()}){
							indices->passed_by.push_back(pass);
						}

						assert(!indices->requirement.get_incompatible_info(itr->requirement).has_value());
						indices->requirement.promote(itr->requirement);

						itr = range.erase(itr);
						end = std::ranges::end(range);
					}else{
						if(itr->source_out() != no_slot) indices = &*itr;
						++itr;
					}
				}
			}

		};
	}


	export
	struct post_process_graph;

	export
	struct pass_resource_reference{
		std::vector<const resource_desc::resource_entity*> inputs{};
		std::vector<const resource_desc::resource_entity*> outputs{};

		[[nodiscard]] bool has_in(inout_index index) const noexcept{
			if(index >= inputs.size()) return false;
			return inputs[index] != nullptr;
		}

		[[nodiscard]] bool has_out(inout_index index) const noexcept{
			if(index >= outputs.size()) return false;
			return outputs[index] != nullptr;
		}

		[[nodiscard]] const resource_desc::resource_entity* get_in_if(inout_index index) const noexcept{
			if(index >= inputs.size()) return nullptr;
			return inputs[index];
		}

		[[nodiscard]] const resource_desc::resource_entity* get_out_if(inout_index index) const noexcept{
			if(index >= outputs.size()) return nullptr;
			return outputs[index];
		}

		[[nodiscard]] const resource_desc::resource_entity& at_in(inout_index index) const {
			if(auto res = get_in_if(index)){
				return *res;
			}else{
				throw std::out_of_range("resource not found");
			}
		}

		[[nodiscard]] const resource_desc::resource_entity& at_out(inout_index index) const {
			if(auto res = get_out_if(index)){
				return *res;
			}else{
				throw std::out_of_range("resource not found");
			}
		}

		void set_in(inout_index idx, const resource_desc::resource_entity* res){
			inputs.resize(std::max<std::size_t>(idx + 1, inputs.size()));
			assert(inputs[idx] == nullptr);
			for (const auto & input : inputs){
				assert(input != res);
			}
			inputs[idx] = res;

		}

		void set_out(inout_index idx, const resource_desc::resource_entity* res){
			outputs.resize(std::max<std::size_t>(idx + 1, outputs.size()));
			assert(outputs[idx] == nullptr);
			outputs[idx] = res;
		}

		void set_inout(inout_index idx, const resource_desc::resource_entity* res){
			set_in(idx, res);
			set_out(idx, res);
		}
	};

	struct pass{
	protected:
		static constexpr math::u32size2 compute_group_unit_size2{16, 16};

		constexpr static math::u32size2 get_work_group_size(math::u32size2 image_size) noexcept{
			return image_size.add(compute_group_unit_size2.copy().sub(1u, 1u)).div(compute_group_unit_size2);
		}

	public:
		std::vector<pass_dependency> dependencies_resource_{};
		std::vector<pass*> dependencies_executions_{};

		[[nodiscard]] explicit pass(vk::context& ctx){};

		pass(const pass& other) = delete;
		pass(pass&& other) noexcept = delete;
		pass& operator=(const pass& other) = delete;
		pass& operator=(pass&& other) noexcept = delete;

		virtual ~pass() = default;

		virtual inout_data& sockets() noexcept = 0;

		virtual void post_init(vk::context& context, const math::u32size2 extent){

		}

		virtual void record_command(
			vk::context& context,
			const pass_resource_reference* resources,
			math::u32size2 extent,
			VkCommandBuffer buffer){

		}

		virtual void reset_resources(vk::context& context, const pass_resource_reference* resources, const math::u32size2 extent){

		}


		[[nodiscard]] const inout_data& sockets() const noexcept{
			return const_cast<pass*>(this)->sockets();
		}

		[[nodiscard]] virtual std::string_view get_name() const noexcept{
			return {};
		}

		[[nodiscard]] std::string get_identity_name() const noexcept{
			return std::format("({:#X}){}", static_cast<std::uint16_t>(std::bit_cast<std::uintptr_t>(this)), get_name());
		}


		void add_dep(pass_dependency dep){
			dependencies_resource_.push_back(dep);
		}

		void add_dep(std::initializer_list<pass_dependency> dep){
			dependencies_resource_.append_range(dep);
		}

		void add_exec_dep(pass* dep){
			dependencies_executions_.push_back(dep);
		}

		void add_exec_dep(std::initializer_list<pass*> dep){
			dependencies_executions_.append_range(dep);
		}
	};

	struct post_process_graph{
	private:
		vk::context* context_{};

		std::vector<std::unique_ptr<pass>> stages{};

		//TODO integrate there with pass?
		std::unordered_map<pass*, std::vector<resource_desc::explicit_resource_usage>> external_outputs{};
		std::unordered_map<pass*, std::vector<resource_desc::explicit_resource_usage>> external_inputs{};
		std::unordered_map<pass*, pass_resource_reference> resource_ref{};

		std::vector<resource_desc::resource_entity> shared_resources{};
		std::vector<resource_desc::resource_entity> exclusive_resources{};
		std::vector<resource_desc::resource_entity> borrowed_resources{};

		plf::hive<resource_desc::explicit_resource> explicit_resources{};

		math::u32size2 extent_{};

		vk::command_buffer main_command_buffer{};
		post_graph::life_bounds life_bounds_{};
	public:
		[[nodiscard]] post_process_graph() = default;

		[[nodiscard]] explicit post_process_graph(vk::context& context)
			: context_(&context), main_command_buffer{context_->get_compute_command_pool().obtain()}{
		}

		template <std::derived_from<pass> T, typename ...Args>
			requires (std::constructible_from<T, vk::context&, Args&& ...>)
		T& add_stage(Args&& ...args){
			auto& ref = *stages.emplace_back(std::make_unique<T>(*context_, std::forward<Args>(args) ...));
			return static_cast<T&>(ref);
		}

		void add_output(pass* from, std::initializer_list<resource_desc::explicit_resource_usage> externals){
			external_outputs[from].append_range(externals);
		}

		void add_output(pass* from, std::initializer_list<std::pair<resource_desc::explicit_resource, inout_index>> externals){
			for (const auto & [res, slot] : externals){
				auto& resref = add_explicit_resource({res});
				external_outputs[from].push_back({resref, slot});
			}
		}

		void add_input(pass* to, std::initializer_list<resource_desc::explicit_resource_usage> externals){
			external_inputs[to].append_range(externals);
		}

		void add_input(pass* from, std::initializer_list<std::pair<resource_desc::explicit_resource, inout_index>> externals){
			for (const auto & [res, slot] : externals){
				auto& resref = add_explicit_resource({res});
				external_inputs[from].push_back({resref, slot});
			}
		}

		auto& add_explicit_resource(resource_desc::explicit_resource res){
			return *explicit_resources.insert(std::move(res));
		}

		void sort(){
			if(external_outputs.empty()){
				throw std::runtime_error("No outputs found");
			}

			struct node{
				unsigned in_degree{};
			};
			std::unordered_map<pass*, node> nodes;



			for(const auto& v : stages){
				nodes.try_emplace(v.get());

				for(auto& dep : v->dependencies_resource_){
					++nodes[dep.id].in_degree;
				}

				for(auto& dep : v->dependencies_executions_){
					++nodes[dep].in_degree;
				}
			}

			std::vector<pass*> queue;
			std::vector<pass*> sorted;
			queue.reserve(stages.size() * 2);
			sorted.reserve(stages.size());

			std::size_t current_index{};

			{//Set Initial Nodes
				for (const auto & [node, indeg] : nodes){
					if(indeg.in_degree)continue;

					queue.push_back(node);
				}
			}

			if(queue.empty()){
				throw std::runtime_error("No zero dependes found");
			}

			while(current_index != queue.size()){
				auto current = queue[current_index++];
				sorted.push_back(current);

				for(const auto& dependency : current->dependencies_resource_){
					auto& deg = nodes[dependency.id].in_degree;
					--deg;
					if(deg == 0){
						queue.push_back(dependency.id);
					}
				}
				for(const auto& dependency : current->dependencies_executions_){
					auto& deg = nodes[dependency].in_degree;
					--deg;
					if(deg == 0){
						queue.push_back(dependency);
					}
				}
			}

			// 检查是否有环
			if(sorted.size() != stages.size()){
				// 存在环，返回空向量或抛出异常
				throw std::runtime_error("sorted_tasks_.size() != nodes.size()");
			}

			std::ranges::reverse(sorted);

			for (auto&& [idx, unique_ptr] : stages | std::views::enumerate){
				unique_ptr.release();
				unique_ptr.reset(sorted[idx]);
			}
		}

	public:
		void check_sockets_connection() const{
			for(const auto& stage : stages){
				auto& inout = stage->sockets();

				std::unordered_map<inout_index, resource_desc::resource_requirement*> resources{inout.input_slots.size()};
				for(const auto& [idx, data_idx] : inout.input_slots | std::views::enumerate){
					resources.try_emplace(idx, std::addressof(inout.data[data_idx]));
				}

				for(const auto& dependency : stage->dependencies_resource_){
					if(auto itr = resources.find(dependency.dst_idx); itr != resources.end()){
						auto& dep_out = dependency.id->sockets();
						if(auto src_data = dep_out.get_out(dependency.src_idx)){
							if(auto cpt = src_data->get_incompatible_info(*itr->second)){
								std::println(std::cerr, "{} {} out[{}] -> {} in[{}]",
									cpt.value(),
										dependency.id->get_identity_name(), dependency.src_idx,
										stage->get_identity_name(), dependency.dst_idx
								);
							} else{

								resources.erase(itr);
							}
						} else{
							std::println(std::cerr, "Unmatched Slot {} out[{}] -> {} in[{}]",
							             dependency.id->get_identity_name(), dependency.src_idx,
							             stage->get_identity_name(), dependency.dst_idx
							);
						}
					}
				}

				if(auto itr = external_inputs.find(stage.get()); itr != external_inputs.end()){
					for(const auto& res : itr->second){
						if(auto ritr = resources.find(res.slot); ritr != resources.end()){
							if(res.is_local()){
								resources.erase(ritr);
								continue;
							}

							if(ritr->second->type() == res.resource->type()){
								resources.erase(ritr);
							} else{
								//TODO type mismatch error
							}
						} else{
							//TODO slot mismatch error
						}
					}
				}

				for(const auto& [in_idx, desc] : resources){
					std::println(std::cerr, "Unresolved Slot {} in[{}]", stage->get_identity_name(), in_idx
					);
				}
			}
		}

	private:
		post_graph::life_bounds get_resource_lifebounds(){
			using namespace post_graph;

			life_bounds result{};

			for(const auto& [node_id, desc] : this->external_outputs){
				auto pass = node_id;
				auto& inout = pass->sockets();
				auto indices = inout.get_categorized_data_indices();

				for(const auto& entry : desc){
					auto& rst = result.bounds.emplace_back(inout.at_out(entry.slot));
					rst.ownership = entry.get_ownership();

					rst.passed_by.push_back({nullptr, node_id, entry.slot, no_slot});
					auto& next = rst.passed_by.emplace_back(pass);
					next.slot_out = entry.slot;

					if(auto itr =
						std::ranges::find(indices.in_out, entry.slot, &categorized_data_indices::inout_pair::out);
						itr != indices.in_out.end()){
						next.slot_in = itr->in;
					}
				}
			}

			{
				auto is_done = [](const resource_lifebound& b){
					return b.passed_by.back().slot_in == no_slot;
				};
				for(auto&& pass : stages | std::views::reverse){
					auto& inout = pass->sockets();
					auto indices = inout.get_categorized_data_indices();
					for(auto in : indices.in){
						auto& rst = result.bounds.emplace_back(inout.at_in(in));
						auto& cur = rst.passed_by.emplace_back(pass.get());
						cur.slot_in = in;
						rst.requirement = inout.at_in(in);
					}
				}


				for(auto&& res_bnd : result.bounds){
					if(is_done(res_bnd)) continue;

					auto cur_head = res_bnd.passed_by.back();
					while(true){
						for(const auto& dependency : cur_head->dependencies_resource_){
							if(dependency.dst_idx == cur_head.slot_in){
								auto& pass = *dependency.id;
								auto& inout = pass.sockets();
								auto indices = inout.get_categorized_data_indices();


								res_bnd.requirement.promote(inout.at_out(dependency.src_idx));
								auto& cur = res_bnd.passed_by.emplace_back(&pass);
								cur.slot_out = dependency.src_idx;

								if(auto itr =
									std::ranges::find(
										indices.in_out, dependency.src_idx, &categorized_data_indices::inout_pair::out);
										itr != indices.in_out.end()){
									cur.slot_in = itr->in;
								}

								break;
							}
						}

						if(cur_head == res_bnd.passed_by.back()){
							break;
						}
						cur_head = res_bnd.passed_by.back();
					}

					if(is_done(res_bnd)) continue;

					if(auto ext_itr = external_inputs.find(const_cast<pass*>(cur_head.where)); ext_itr != external_inputs.end()){
						for(const auto& entry : ext_itr->second){
							if(entry.slot == cur_head.slot_in){
								res_bnd.passed_by.push_back(pass_identity{nullptr, cur_head.where, no_slot, entry.slot});
								res_bnd.ownership = entry.get_ownership();

								break;
							}
						}
					}
				}
			}

			for(auto& lifebound : result.bounds){
				std::ranges::reverse(lifebound.passed_by);
			}

			result.unique();

			return result;
		}

	public:
		const resource_desc::resource_entity& out_at(const pass* p, inout_index index) const {
			return resource_ref.at(const_cast<pass*>(p)).at_out(index);
		}

		void analysis_minimal_allocation(){
			auto life_bounds = get_resource_lifebounds();

			using namespace post_graph;
			using namespace resource_desc;

			static constexpr resource_requirement_partial_greater_comp comp{};
			std::ranges::sort(life_bounds.bounds, comp, &resource_lifebound::requirement);
			life_bounds_ = life_bounds;

			struct lifebound_ref{
				resource_lifebound* bound{};
				unsigned* assigned_resource_index{nullptr};

				const resource_requirement& requirement() const noexcept{
					return bound->requirement;
				}
			};

			std::vector<unsigned> lifebound_assignments(life_bounds.bounds.size(), no_slot);
			using StagesMap =
			std::unordered_map<pass_identity, std::vector<lifebound_ref>, pass_identity_hasher>;
			StagesMap req_per_stage{};

			struct req_partial_eq{
				static bool operator()(const resource_requirement& l, const resource_requirement& r) noexcept {
					return l.is_compatibility_part_equal_to(r);
				}
			};

			struct req_partial_hash{
				static std::size_t operator()(const resource_requirement& l) noexcept {
					static constexpr std::hash<std::string_view> hasher{};
					//TODO better hash
					const std::string_view hash{reinterpret_cast<const char*>(&l), sizeof(l)};
					return hasher(hash);
				}
			};

			struct partition{
				std::vector<resource_requirement> requirements{};
				std::unordered_map<pass_identity, std::vector<std::uint8_t>, pass_identity_hasher> stage_captures{};
				std::vector<unsigned> indices{};

				unsigned prefix_sum{};

				[[nodiscard]] const resource_requirement& identity() const noexcept{
					return requirements.front();
				}

				std::size_t add(const resource_requirement& requirement){
					const auto idx = requirements.size();
					indices.push_back(idx);
					requirements.emplace_back(requirement);
					for (auto& marks : stage_captures | std::views::values){
						marks.resize(requirements.size(), 0);
					}
					return idx;
				}
				//
				// void reset_marks(const StagesMap& req_per_stage){
				// 	for (const auto & pass_identity : req_per_stage | std::views::keys){
				// 		stage_captures[pass_identity].assign(requirements.size(), 0);
				// 	}
				// }

			};

			std::vector<partition> minimal_requirements_per_partition{};

			auto get_partition_itr = [&](const resource_requirement& r){
				return std::ranges::find_if(minimal_requirements_per_partition, [&](const partition& req){
					return !req.identity().get_incompatible_info(r).has_value();
				});
			};


			{
				//cal shared requirements
				for (auto&& [idx, image] : life_bounds.bounds | std::views::enumerate){
					if(image.ownership != ownership_type::shared)continue;
					for (const auto & passed_by : image.passed_by){
						req_per_stage[passed_by].push_back({&image, &lifebound_assignments[idx]});
					}
				}

				for (auto&& [idx, image] : life_bounds.bounds | std::views::enumerate){
					if(image.ownership != ownership_type::shared)continue;
					if(auto category = get_partition_itr(image.requirement); category == minimal_requirements_per_partition.end()){
						auto& vec = minimal_requirements_per_partition.emplace_back();
						for (const auto & pass : req_per_stage | std::views::keys){
							vec.stage_captures.insert({pass, std::vector<std::uint8_t>{}});
						}

						vec.add(image.requirement);
					}
				}

				for (auto&& [bound, assignment] : std::views::zip(life_bounds.bounds, lifebound_assignments)){
					if(bound.ownership != ownership_type::shared)continue;

					if(assignment != no_slot)continue;

					auto partition_itr = get_partition_itr(bound.requirement);
					assert(partition_itr != minimal_requirements_per_partition.end());
					std::ranges::sort(partition_itr->indices, [&](unsigned l, unsigned r){
						return comp(partition_itr->requirements[l], partition_itr->requirements[r]);
					});
					for (auto& candidate : partition_itr->indices){
						if(std::ranges::any_of(bound.passed_by, [&](const pass_identity& passed_by){
							return partition_itr->stage_captures.at(passed_by)[candidate];
						})){
							continue;
						}

						if(comp(bound.requirement, partition_itr->requirements[candidate])){
							for (const auto & passed_by : bound.passed_by){
								partition_itr->stage_captures.at(passed_by)[candidate] = true;
							}
							assignment = candidate;

							goto skip;
						}
					}

					//No currently compatibled, promote the minimal found one to fit
					for (unsigned candidate : partition_itr->indices | std::views::reverse){
						if(std::ranges::any_of(bound.passed_by, [&](const pass_identity& passed_by){
							return partition_itr->stage_captures.at(passed_by)[candidate];
						})){
							continue;
						}

						for (const auto & passed_by : bound.passed_by){
							partition_itr->stage_captures.at(passed_by)[candidate] = true;
						}

						assignment = candidate;
						partition_itr->requirements[candidate].promote(bound.requirement);
						goto skip;

					}

					assignment = partition_itr->add(bound.requirement);

					skip:
					(void)0;
				}
			}

			unsigned prefix_sum{};
			{
				for (auto & pt : minimal_requirements_per_partition){
					pt.prefix_sum = prefix_sum;
					prefix_sum += pt.requirements.size();

					for (const auto& requirement : pt.requirements){
						shared_resources.push_back(resource_entity{requirement});
					}
				}
			}

			for (auto&& [bound, assignment] : std::views::zip(life_bounds.bounds, lifebound_assignments)){
				if(bound.ownership != ownership_type::shared)continue;
				assert(assignment != no_slot);

				auto& pt = *get_partition_itr(bound.requirement);
				bound.used_entity = &shared_resources[pt.prefix_sum + assignment];

				for (const auto & passed_by : bound.passed_by){
					if(!passed_by.where)continue;
					auto& ref = resource_ref[const_cast<pass*>(passed_by.where)];
					if(passed_by.slot_in != no_slot){
						ref.set_in(passed_by.slot_in, bound.used_entity);
					}

					if(passed_by.slot_out != no_slot){
						ref.set_out(passed_by.slot_out, bound.used_entity);
					}
				}
			}


			{
				//assign exclusive resources
				for (const auto & req : life_bounds.bounds){
					if(req.ownership != ownership_type::exclusive)continue;
					exclusive_resources.push_back(resource_entity{req.requirement});
				}

				for (const auto & [idx, req] : life_bounds.bounds
					| std::views::filter([](const resource_lifebound& b){return b.ownership == ownership_type::exclusive;})
					| std::views::enumerate){

					req.used_entity = &exclusive_resources[idx];

					for (const auto & passed_by : req.passed_by){
						if(!passed_by.where)continue;
						auto& ref = resource_ref[const_cast<pass*>(passed_by.where)];

						if(passed_by.slot_in != no_slot){
							ref.set_in(passed_by.slot_in, req.used_entity);
						}

						if(passed_by.slot_out != no_slot){
							ref.set_out(passed_by.slot_out, req.used_entity);
						}
					}
				}
			}

			{

				std::unordered_map<const explicit_resource*, resource_entity*> group;
				std::unordered_map<pass_identity, resource_entity* *, pass_identity_hasher> pass_to_group;

				for (const auto & [pass, res] : external_inputs){
					for (const auto & re : res){
						if(!re.resource->is_external_spec())continue;

						pass_to_group[{nullptr, pass, no_slot, re.slot}] = &(group[re.resource.get()] = nullptr);
					}
				}

				auto initial = pass_to_group;

				// std::vector<>pass_identity
				std::size_t count{};

				//assign borrowed resources
				for (const auto & req : life_bounds.bounds){
					if(req.ownership != ownership_type::external)continue;
					++count;
				}

				borrowed_resources.reserve(count);

				for (const auto & [idx, req] : life_bounds.bounds
					| std::views::filter([](const resource_lifebound& b){return b.ownership == ownership_type::external;})
					| std::views::enumerate){
					if(const auto ptr = pass_to_group.at(req.get_head()); *ptr != nullptr){
						req.used_entity = *ptr;
					}else{
						*ptr = req.used_entity = &borrowed_resources.emplace_back(resource_entity{req.requirement});
					}

					for (const auto & passed_by : req.passed_by){
						if(!passed_by.where)continue;
						auto& ref = resource_ref[const_cast<pass*>(passed_by.where)];

						if(passed_by.slot_in != no_slot){
							ref.set_in(passed_by.slot_in, req.used_entity);
						}

						if(passed_by.slot_out != no_slot){
							ref.set_out(passed_by.slot_out, req.used_entity);
						}
					}
				}
			}
		}

		void print_resource_reference() const {
			std::println();
			std::println("Resource Requirements: {}", shared_resources.size());

			std::map<const resource_desc::resource_entity*, std::vector<post_graph::pass_identity>> inouts{};
			for (const auto & [pass, ref] : resource_ref){
				for (const auto & [idx, res] : ref.inputs | std::views::enumerate){
					if(res){
						inouts[res].push_back(post_graph::pass_identity{pass, nullptr, static_cast<inout_index>(idx)});
					}
				}
				for (const auto & [idx, res] : ref.outputs | std::views::enumerate){
					if(res){
						auto& vec = inouts[res];
						if(auto itr = std::ranges::find(vec, pass, &post_graph::pass_identity::where); itr != vec.end()){
							itr->slot_out = idx;
						}else{
							vec.push_back(post_graph::pass_identity{pass, nullptr, no_slot, static_cast<inout_index>(idx)});
						}

					}
				}
			}

			for (const auto & [res, user] : inouts){
				std::println("Res[{:0>2}]<{}>: ", res - shared_resources.data(), res->overall_req.type_name());
				std::println("{:n:}", user | std::views::transform([](const post_graph::pass_identity& value){
					return value.format("None");
				}));
				std::println();
			}
		}


		void post_init(){
			for (const auto & stage : stages){
				stage->post_init(*context_, extent_);
			}
		}

		void update_external_resources() noexcept{
			static constexpr auto get_resource = [](std::vector<resource_desc::resource_entity>& range, const resource_desc::resource_entity* ptr) -> resource_desc::resource_entity* {
				static constexpr auto lt = std::ranges::less{};
				static constexpr auto gteq = std::ranges::greater_equal{};
				if( gteq(ptr, range.data()) && lt(ptr, range.data() + range.size())){
					return range.data() + (ptr - range.data());
				}else{
					return nullptr;
				}
			};

			static constexpr auto update = [](const resource_desc::explicit_resource& external_resource, resource_desc::resource_entity& entity){
				using namespace resource_desc;
				std::visit(overload_def_noop{
					std::in_place_type<void>,
					[](const external_buffer& ext, buffer_entity& ent){
						ent.buffer = ext.handle;
					}, [](const external_image& ext, image_entity& ent){
						ent.image = ext.handle;
					}
				}, external_resource.desc, entity.resource);
			};

			for (auto& [pass, ref] : resource_ref){
				for (const auto & [slot, entity] : ref.inputs | std::views::enumerate){
					if(entity == nullptr)continue;
					const auto res = get_resource(borrowed_resources, entity);
					if(!res)continue;

					const auto itr = external_inputs.find(pass);
					if(itr == external_inputs.end())continue;

					for (const auto & external : itr->second){
						if(external.slot == slot){
							update(*external.resource, *res);
							break;
						}
					}
				}

				for (const auto & [slot, entity] : ref.outputs | std::views::enumerate){
					if(entity == nullptr)continue;
					const auto res = get_resource(borrowed_resources, entity);
					if(!res)continue;

					const auto itr = external_outputs.find(pass);
					if(itr == external_outputs.end())continue;

					for (const auto & external : itr->second){
						if(external.slot == slot){
							update(*external.resource, *res);
							break;
						}
					}
				}

			}
		}

		void resize(math::u32size2 size){
			if(extent_ != size){
				extent_ = size;
				allocate();
			}
		}

		void resize(VkExtent2D size){
			resize(std::bit_cast<math::u32size2>(size));
		}
		void resize(){
			resize(context_->get_extent());
		}

		void allocate(){
			for (auto & local_resource : shared_resources){
				local_resource.allocate_image(*context_, std::bit_cast<VkExtent2D>(extent_));
			}
			for (auto & local_resource : exclusive_resources){
				local_resource.allocate_image(*context_, std::bit_cast<VkExtent2D>(extent_));
			}

			for (const auto & stage : stages){
				const pass_resource_reference* ref{};
				if(auto itr = resource_ref.find(stage.get()); itr != resource_ref.end()){
					ref = &itr->second;
				}

				stage->reset_resources(*context_, ref, extent_);
			}

			// for (auto && [pass, spec] : external_outputs){
			// 	for (auto & external_resource : spec){
			// 		if(external_resource.get_ownership() == resource_desc::ownership_type::external)continue;
			// 		auto& ref = resource_ref.at(pass);
			// 		auto out = ref.outputs[external_resource.slot];
			// 		external_resource.resource->set_from_entity(*out);
			// 	}
			// }
		}

		void create_command(){
			using namespace resource_desc;
			std::unordered_map<const resource_entity*, entity_state> res_states{};

			// auto try_get_layout = [&](const resource_entity* entity){
			// 	if(const auto itr = res_states.find(entity); itr != res_states.end()){
			// 		return itr->second.get_layout();
			// 	}
			// 	return VK_IMAGE_LAYOUT_UNDEFINED;
			// };

			vk::scoped_recorder scoped_recorder{main_command_buffer, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT};

			vk::cmd::dependency_gen dependency_gen{};

			for (const auto & stage : stages){
				auto& inout = stage->sockets();
				auto& ref = resource_ref.at(stage.get());

				for (const auto& [in_idx, data_idx] : inout.get_valid_in()){
					auto rentity = ref.inputs[in_idx];
					auto& cur_req = inout.data[data_idx];

					assert(rentity != nullptr);

					std::visit(overload_narrow{
						[&](const image_requirement& r, const image_entity& entity){
							if(auto to_this_itr = external_inputs.find(stage.get()); to_this_itr != external_inputs.end()){
								for (const auto & res : to_this_itr->second){
									if(res.slot == in_idx && !res.is_local()){
										auto [itr, suc] = res_states.try_emplace(ref.inputs[res.slot], entity_state{*res.resource});
										if(!suc)continue;
										auto& state = itr->second;
										state.external_init_required = true;

										if(auto layout = res.resource->get_layout(); layout != VK_IMAGE_LAYOUT_UNDEFINED){
											state.last_stage = deduce_stage(layout);
											state.last_access = deduce_external_image_access(state.last_stage);
										}

									}
								}
							}

							const auto& req = cur_req.get<image_requirement>();
							VkImageLayout old_layout{};
							VkPipelineStageFlags2 old_stage{VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT};
							VkAccessFlags2 old_access{VK_ACCESS_2_NONE};
							if(const auto itr = res_states.find(rentity); itr != res_states.end()){
								old_layout = itr->second.get_layout();
								old_stage = itr->second.last_stage;
								old_access = itr->second.last_access;
							}

							const auto new_layout = req.get_expected_layout();
							const auto next_stage = deduce_stage(new_layout);
							const auto next_access = req.get_image_access(next_stage);
							const auto aspect = r.get_aspect();

							dependency_gen.push(
								entity.image.image,
								old_stage,
								old_access,
								next_stage,
								next_access,
								old_layout,
								new_layout,
								{
									.aspectMask = aspect,
									.baseMipLevel = 0,
									.levelCount = value_or(r.mip_levels, 1u),
									.baseArrayLayer = 0,
									.layerCount = 1
								}
							);

							auto& state = res_states[rentity];
							std::get<image_entity_state>(state.desc).current_layout = new_layout;
							state.last_stage = next_stage;
							state.last_access = next_access;
							state.external_init_required = false;

						},
						[&](const buffer_requirement& r, const buffer_entity& entity){

						}
					}, rentity->overall_req.desc, rentity->resource);
				}

				//modify output image layout, as output initialization
				for (const auto& [out_idx, data_idx] : inout.get_valid_out()){
					auto rentity = ref.outputs[out_idx];
					auto& cur_req = inout.data[data_idx];

					assert(rentity != nullptr);

					std::visit(overload_narrow{
						[&](const image_requirement& r, const image_entity& entity){

							const auto& req = cur_req.get<image_requirement>();
							VkImageLayout old_layout{};
							VkPipelineStageFlags2 old_stage{VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT};
							VkAccessFlags2 old_access{VK_ACCESS_2_NONE};
							if(const auto itr = res_states.find(rentity); itr != res_states.end()){
								old_layout = itr->second.get_layout();
								old_stage = itr->second.last_stage;
								old_access = itr->second.last_access;
							}

							const auto new_layout = req.get_expected_layout();
							const auto next_stage = deduce_stage(new_layout);
							const auto next_access = req.get_image_access(next_stage);
							const auto aspect = r.get_aspect();

							dependency_gen.push(
								entity.image.image,
								old_stage,
								old_access,
								next_stage,
								next_access,
								old_layout,
								new_layout,
								{
									.aspectMask = aspect,
									.baseMipLevel = 0,
									.levelCount = value_or(r.mip_levels, 1u),
									.baseArrayLayer = 0,
									.layerCount = 1
								}
							);

							std::get<image_entity_state>(res_states[rentity].desc).current_layout = new_layout;
						},
						[&](const buffer_requirement& r, const buffer_entity& entity){

						}
					}, rentity->overall_req.desc, rentity->resource);
				}
				if(!dependency_gen.empty())dependency_gen.apply(scoped_recorder);

				stage->record_command(*context_, &ref, extent_, scoped_recorder);

				//restore output final format, transition if any should be done within the pass
				for (const auto& [out_idx, data_idx] : inout.get_valid_in()){
					auto rentity = ref.inputs[out_idx];
					auto& cur_req = inout.data[data_idx];

					assert(rentity != nullptr);

					std::visit(overload_narrow{
						[&](const image_requirement& r, const image_entity& entity){
							auto layout = cur_req.get<image_requirement>().get_expected_layout_on_output();
							std::get<image_entity_state>(res_states[rentity].desc).current_layout = layout;
						},
						[&](const buffer_requirement& r, const buffer_entity& entity){

						}
					}, rentity->overall_req.desc, rentity->resource);
				}

				for (const auto& [out_idx, data_idx] : inout.get_valid_out()){
					auto rentity = ref.outputs[out_idx];
					auto& cur_req = inout.data[data_idx];

					assert(rentity != nullptr);

					std::visit(overload_narrow{
						[&](const image_requirement& r, const image_entity& entity){
							auto layout = cur_req.get<image_requirement>().get_expected_layout_on_output();
							std::get<image_entity_state>(res_states[rentity].desc).current_layout = layout;
						},
						[&](const buffer_requirement& r, const buffer_entity& entity){

						}
					}, rentity->overall_req.desc, rentity->resource);

					if(auto to_this_itr = external_outputs.find(stage.get()); to_this_itr != external_outputs.end()){
						for (const auto & res : to_this_itr->second){
							if(res.resource->get_layout() == VK_IMAGE_LAYOUT_UNDEFINED || !res.resource->is_external_spec())continue;
							if(res.slot != out_idx)continue;

							std::visit(
								overload_narrow{
									[&](const image_requirement& r, const image_entity& entity){
										VkImageLayout old_layout{};
										VkPipelineStageFlags2 old_stage{VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT};
										VkAccessFlags2 old_access{VK_ACCESS_2_NONE};
										if(const auto itr = res_states.find(rentity); itr != res_states.end()){
											old_layout = itr->second.get_layout();
											old_stage = itr->second.last_stage;
											old_access = itr->second.last_access;
										}

										const auto new_layout = res.resource->get_layout();
										const auto next_stage = deduce_stage(new_layout);

										const auto& req = cur_req.get<image_requirement>();
										const auto aspect = r.get_aspect();

										dependency_gen.push(
											entity.image.image,
											old_stage,
											old_access,
											next_stage,
											deduce_external_image_access(next_stage),
											old_layout,
											new_layout,
											{
												.aspectMask = aspect,
												.baseMipLevel = 0,
												.levelCount = value_or(r.mip_levels, 1u),
												.baseArrayLayer = 0,
												.layerCount = 1
											}
										);
									},
									[&](const buffer_requirement& r, const buffer_entity& entity){
									}
								}, rentity->overall_req.desc, rentity->resource);
						}
					}
				}
			}

			if(!dependency_gen.empty())dependency_gen.apply(scoped_recorder);
		}

		VkCommandBuffer get_main_command_buffer() const noexcept{
			return main_command_buffer;
		}

		// pass& operator[](post_process_id id) noexcept{
		// 	return *stages.at(id);
		// }
		//
		// const pass& operator[](post_process_id id) const noexcept{
		// 	return *stages.at(id);
		// }
	};
}

module : private;

namespace  mo_yanxi::graphic::post_graph{
	std::string pass_identity::format(std::string_view endpoint_name) const{
		static constexpr auto fmt_slot = [](std::string_view epn, inout_index idx) -> std::string {
			return idx == no_slot ? std::string(epn) : std::format("{}", idx);
		};

		return std::format("[{}] {} [{}]", fmt_slot(endpoint_name, slot_in), std::string(where ? where->get_identity_name() : "endpoint"), fmt_slot(endpoint_name, slot_out));
	}
}
