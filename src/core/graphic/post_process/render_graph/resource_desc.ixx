module;

#include <vulkan/vulkan.h>

#include "ext/enum_operator_gen.hpp"
#include "gch/small_vector.hpp"

export module mo_yanxi.graphic.render_graph.resource_desc;

import mo_yanxi.vk.image_derives;
import mo_yanxi.vk.resources;
import mo_yanxi.vk.vma;

import mo_yanxi.graphic.shader_reflect;
import mo_yanxi.meta_programming;
import mo_yanxi.basic_util;
import mo_yanxi.referenced_ptr;

import std;
#define THOROUGH
#define TRANSIENT

namespace mo_yanxi::graphic::render_graph{
	export
	constexpr VkPipelineStageFlags2 deduce_stage(VkImageLayout layout) noexcept {
		switch(layout){
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : return VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL : [[fallthrough]];
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : return VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : return VK_PIPELINE_STAGE_2_TRANSFER_BIT;

		default : return VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		}
	}

	export
	constexpr VkAccessFlags2 deduce_external_image_access(VkPipelineStageFlags2 stage) noexcept {
	 	//TODO use bit_or to merge, instead of individual test?
	 	switch(stage){
	 		case VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT : return VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	 		case VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT : return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	 		default: return VK_ACCESS_2_NONE;
	 	}
	}


	export using inout_index = unsigned;
	export constexpr inline inout_index no_slot = std::numeric_limits<inout_index>::max();

	namespace resource_desc{
		export
		enum struct resource_type : unsigned{
			image, buffer, unknown
		};

		export
		[[nodiscard]] constexpr std::string_view type_to_name(resource_type type) noexcept{
			switch(type){
			case resource_type::image: return "image";
			case resource_type::buffer: return "buffer";
			default: return "unknown";
			}
		}



		export
		enum struct decoration{
			unknown,
			// sampled = 1,
			read = 1 << 1,
			write = 1 << 2,
		};

		BITMASK_OPS(export, decoration);

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
		constexpr T value_or(T val, T alternative) noexcept{
			if(val == no_req_spec){
				return alternative;
			}
			return val;
		}

		export
		template <typename T>
		constexpr T value_or(T val, T alternative, const T& nullopt_value) noexcept{
			if(val == nullopt_value){
				return alternative;
			}
			return val;
		}

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
			THOROUGH TRANSIENT decoration decr{};

			THOROUGH std::uint32_t sample_count{};

			THOROUGH VkFormat format{VK_FORMAT_UNDEFINED};

			THOROUGH VkImageUsageFlags usage{/*No Spec*/};

			THOROUGH VkImageAspectFlags aspect_flags{/*No Spec*/};

			/**
			 * @brief explicitly specfied expected initial layout of the image
			 */
			TRANSIENT VkImageLayout override_layout{VK_IMAGE_LAYOUT_UNDEFINED};

			/**
			 * @brief explicitly specfied expected final layout of the image
			 */
			TRANSIENT VkImageLayout override_output_layout{VK_IMAGE_LAYOUT_UNDEFINED};

			THOROUGH unsigned dim{no_req_spec};
			THOROUGH unsigned mip_levels{no_req_spec};
			THOROUGH unsigned scaled_times{no_req_spec};

			[[nodiscard]] VkImageUsageFlags get_required_usage() const noexcept{
				VkImageUsageFlags rst{};
				if(sample_count){
					rst |= VK_IMAGE_USAGE_SAMPLED_BIT;
				} else{
					rst |= VK_IMAGE_USAGE_STORAGE_BIT;
				}

				return rst;
			}

			[[nodiscard]] VkImageAspectFlags get_aspect() const noexcept{
				return aspect_flags ? aspect_flags : VK_IMAGE_ASPECT_COLOR_BIT;
			}

			void promote(const image_requirement& other) noexcept{
				dim = std::max(dim, other.dim);

				sample_count = std::max(sample_count, other.sample_count);
				decr |= other.decr;

				mip_levels = get_optional_max(mip_levels, other.mip_levels, no_req_spec);
				if(format == VK_FORMAT_UNDEFINED)format = other.format;
				if(scaled_times == no_req_spec)scaled_times = other.scaled_times;
				assert(format == other.format || other.format == VK_FORMAT_UNDEFINED);
				assert(scaled_times == other.scaled_times || other.scaled_times == no_req_spec);

				usage |= other.usage;
				aspect_flags |= other.aspect_flags;
			}

			[[nodiscard]] bool is_sampled_image() const noexcept{
				return sample_count > 0 ;
			}

			[[nodiscard]] VkImageLayout get_expected_layout() const noexcept{
				if(override_layout)return override_layout;
				return is_sampled_image()
						   ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
						   : VK_IMAGE_LAYOUT_GENERAL;
			}

			[[nodiscard]] VkImageLayout get_expected_layout_on_output() const noexcept{
				if(override_output_layout)return override_output_layout;
				return get_expected_layout();
			}


			[[nodiscard]] VkAccessFlags2 get_image_access(VkPipelineStageFlags2 pipelineStageFlags2) const noexcept{
				switch(pipelineStageFlags2){
				case VK_PIPELINE_STAGE_2_TRANSFER_BIT : switch(decr){
					case decoration::read : return VK_ACCESS_2_TRANSFER_READ_BIT;
					case decoration::write : return VK_ACCESS_2_TRANSFER_WRITE_BIT;
					case decoration::read | decoration::write : return VK_ACCESS_2_TRANSFER_READ_BIT |
							VK_ACCESS_2_TRANSFER_WRITE_BIT;
					default : return VK_ACCESS_2_NONE;
					}


				default :
					if(is_sampled_image())return VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
					switch(decr){
					case decoration::read : return VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
					case decoration::write : return VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
					case decoration::read | decoration::write : return VK_ACCESS_2_SHADER_STORAGE_READ_BIT |
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
					dim <=> other_in_same_partition.dim
				);
			}
		};

		export
		struct buffer_requirement{
			THOROUGH TRANSIENT VkDeviceSize size;
			TRANSIENT VkAccessFlagBits2 access{VK_ACCESS_2_NONE};

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
						if(l.format != r.format && (l.format != VK_FORMAT_UNDEFINED && r.format != VK_FORMAT_UNDEFINED)){
							return "Format Incompatible";
						}

						return std::nullopt;
					},
					[](const buffer_requirement& l, const buffer_requirement& r) -> ret {
						return std::nullopt;
					}
				}, desc, req.desc);
			}


			[[nodiscard]] bool must_compatible_with(const resource_requirement& req) const noexcept{
				if(!is_type_valid_equal_to(req)){
					return false;
				}

				return std::visit<bool>(overload_def_noop{
					std::in_place_type<bool>,
					[](const image_requirement& l, const image_requirement& r){
						if(l.aspect_flags != r.aspect_flags)return false;
						if(l.scaled_times != r.scaled_times)return false;
						if(l.format != r.format && (l.format != VK_FORMAT_UNDEFINED && r.format != VK_FORMAT_UNDEFINED))return false;

						return true;
					},
					[](const buffer_requirement& l, const buffer_requirement& r) -> bool {
						//TODO
						throw std::invalid_argument("not implemented");
						return false;
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
		};

		export
		struct image_entity{
			//TODO switch to image with no wrap
			vk::storage_image entity{};
			vk::image_handle image{};

			[[nodiscard]] bool is_local() const noexcept{
				return static_cast<bool>(entity);
			}

			void allocate(vk::allocator& allocator, VkExtent2D extent2D, const image_requirement& requirement){
				auto div_times = value_or<std::uint32_t>(requirement.scaled_times, 0);
				extent2D.width >>= div_times;
				extent2D.height >>= div_times;

				entity = vk::storage_image{
					allocator,
					extent2D,
					vk::get_recommended_mip_level(extent2D, value_or<std::uint32_t>(requirement.mip_levels, 1u)),
					value_or(requirement.format,  VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_UNDEFINED),
					requirement.usage
				};

				image = entity;
			}

		};

		export
		struct buffer_entity{
			vk::storage_buffer entity{};
			vk::buffer_borrow buffer{};

			void allocate(vk::allocator& allocator, VkDeviceSize size){
				entity = vk::storage_buffer{
						allocator, size,
						VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
						VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
						VK_BUFFER_USAGE_TRANSFER_DST_BIT
					};

				buffer = entity.borrow();
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

			void allocate_buffer(vk::allocator& allocator, VkDeviceSize size){
				if(const auto res = std::get_if<buffer_entity>(&resource)){
					res->allocate(allocator, size);
				}
			}

			void allocate_image(vk::allocator& allocator, VkExtent2D extent){
				if(const auto res = std::get_if<image_entity>(&resource)){
					res->allocate(allocator, extent, overall_req.get<image_requirement>());
				}
			}

			[[nodiscard]] resource_type type() const noexcept{
				return overall_req.type();
			}

			[[nodiscard]] image_entity& as_image() noexcept{
				return std::get<image_entity>(resource);
			}

			[[nodiscard]] const image_entity& as_image() const noexcept{
				return std::get<image_entity>(resource);
			}

			[[nodiscard]] buffer_entity& as_buffer() noexcept{
				return std::get<buffer_entity>(resource);
			}

			[[nodiscard]] const buffer_entity& as_buffer() const noexcept{
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
			VkPipelineStageFlags2    src_stage{VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT};
			VkAccessFlags2           src_access{VK_ACCESS_2_NONE};
			VkPipelineStageFlags2    dst_stage{VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT};
			VkAccessFlags2           dst_access{VK_ACCESS_2_NONE};
		};

		export
		struct explicit_resource{
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

			[[nodiscard]] resource_type type() const noexcept{
				return resource_type{static_cast<std::underlying_type_t<resource_type>>(desc.index())};
			}

			[[nodiscard]] VkImageLayout get_layout() const noexcept{
				if(auto img = std::get_if<external_image>(&desc)){
					return img->expected_layout;
				}
				return VK_IMAGE_LAYOUT_UNDEFINED;
			}


			[[nodiscard]] external_image& as_image() noexcept{
				return std::get<external_image>(desc);
			}

			[[nodiscard]] const external_image& as_image() const noexcept{
				return std::get<external_image>(desc);
			}

			[[nodiscard]] external_buffer& as_buffer() noexcept{
				return std::get<external_buffer>(desc);
			}

			[[nodiscard]] const external_buffer& as_buffer() const noexcept{
				return std::get<external_buffer>(desc);
			}
		};

		export
		struct explicit_resource_usage{
			explicit_resource* resource{};
			inout_index slot{no_slot};
			bool shared{};

			[[nodiscard]] explicit_resource_usage() = default;

			//Construct for borrow external resource
			[[nodiscard]] explicit_resource_usage(explicit_resource& resource, inout_index slot)
				: resource(&resource),
				  slot(slot){
			}

			//Construct for internal explicit allocation
			[[nodiscard]] explicit_resource_usage(inout_index slot, bool shared)
				: slot(slot), shared(shared){
			}

			[[nodiscard]] ownership_type get_ownership() const noexcept{
				if(!is_local()){
					return ownership_type::external;
				}else{
					return shared ? ownership_type::shared : ownership_type::exclusive;
				}
			}

			[[nodiscard]] bool is_local() const noexcept{
				return resource == nullptr;
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

			[[nodiscard]] entity_state() = default;

			[[nodiscard]] explicit(false) entity_state(const explicit_resource& ext){
				last_stage = ext.dependency.src_stage;
				last_access = ext.dependency.src_access;

				std::visit(overload_narrow{
					[&, this](const external_image& image){
						auto& s = desc.emplace<image_entity_state>();
						s.current_layout = image.expected_layout;


						if(auto layout = ext.get_layout(); layout != VK_IMAGE_LAYOUT_UNDEFINED){
							if(last_stage == VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT)last_stage = deduce_stage(layout);
							if(last_access == VK_ACCESS_2_NONE)last_access = deduce_external_image_access(last_stage);
						}
					},
					[](const external_buffer& buffer){

					}
				}, ext.desc);
			}

			[[nodiscard]] resource_type type() const noexcept{
				return static_cast<resource_type>(desc.index());
			}

			[[nodiscard]] VkImageLayout get_layout() const noexcept{
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

		friend constexpr bool operator==(const slot_pair& lhs, const slot_pair& rhs) noexcept = default;
	};


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
	private:
		gch::small_vector<resource_map_entry> connection_{};

	public:
		[[nodiscard]] post_process_stage_inout_map() = default;

		[[nodiscard]] post_process_stage_inout_map(std::initializer_list<resource_map_entry> map)
			: connection_(map){
#if DEBUG_CHECK
			std::unordered_set<binding_info> bindings{};
			bindings.reserve(connection_.size());
			for(const auto& pass : connection_){
				if(!bindings.insert(pass.binding).second){
					assert(false);
				}
			}
#endif
		}

		std::optional<slot_pair> operator[](binding_info binding) const noexcept{
			if(auto itr = std::ranges::find(connection_, binding, &resource_map_entry::binding); itr != connection_.
				end()){
				return itr->slot;
				}

			return std::nullopt;
		}


		void compact_check(){
			if(connection_.empty()){
				return;
			}


			auto checker = [this](auto transform){
				std::ranges::sort(connection_, {}, transform);
				if(transform(connection_.front()) != no_slot && transform(connection_.front()) != 0) return false;

				for(auto [l, r] : connection_ | std::views::transform(transform) | std::views::take_while(
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

			std::ranges::sort(connection_, std::ranges::greater{}, &resource_map_entry::is_inout);

			//TODO check no inout overlap?
		}

		[[nodiscard]] std::span<const resource_map_entry> connection() const noexcept{
			return {connection_.data(), connection_.size()};
		}
	};

	//TODO make the access control more strict
	export
	struct pass_inout_connection{
		static constexpr std::size_t sso = 4;
		using data_index = unsigned;
		using slot_to_data_index = gch::small_vector<data_index, sso>;

		gch::small_vector<resource_desc::resource_requirement, sso> data{};
		slot_to_data_index input_slots{};
		slot_to_data_index output_slots{};

	private:
		void resize_and_set_in(inout_index idx, std::size_t desc_index){
			input_slots.resize(std::max<std::size_t>(input_slots.size(), idx + 1), no_slot);
			input_slots[idx] = desc_index;
		}

		void resize_and_set_out(inout_index idx, std::size_t desc_index){
			output_slots.resize(std::max<std::size_t>(output_slots.size(), idx + 1), no_slot);
			output_slots[idx] = desc_index;
		}

		[[nodiscard]] bool has_index(inout_index idx, slot_to_data_index pass_inout_connection::* mptr) const noexcept{
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
		auto get_valid_of(slot_to_data_index pass_inout_connection::* which) const noexcept {
			return (this->*which) | std::views::enumerate | std::views::filter([](auto&& t){
				auto&& [idx, v] = t;
				return v != no_slot;
			});
		}

	public:

		auto get_valid_in() const noexcept{
			return get_valid_of(&pass_inout_connection::input_slots);
		}

		auto get_valid_out() const noexcept{
			return get_valid_of(&pass_inout_connection::output_slots);
		}


		template <typename T>
			requires std::constructible_from<resource_desc::resource_requirement, T&&>
		void add(const bool in, const bool out, T&& val){
			const auto sz = data.size();
			data.push_back(std::forward<T>(val));
			if(in){
				input_slots.push_back(sz);
			}
			if(out){
				output_slots.push_back(sz);
			}
		}


		template <typename T>
			requires std::constructible_from<resource_desc::resource_requirement, T&&>
		void add(resource_map_entry type, T&& val){
			if(has_index(type.slot.in, &pass_inout_connection::input_slots) || has_index(type.slot.out, &pass_inout_connection::output_slots)){
				return;
			}

			const auto sz = data.size();
			data.push_back(std::forward<T>(val));

			if(type.slot.in != no_slot){
				resize_and_set_in(type.slot.in, sz);
			}

			if(type.slot.out != no_slot){
				resize_and_set_out(type.slot.out, sz);
			}
		}

		[[nodiscard]] gch::small_vector<slot_pair> get_inout_indices() const{
			gch::small_vector<slot_pair> rst{};

			for(auto&& [idx, data_idx] : output_slots | std::views::enumerate){
				if(auto itr = std::ranges::find(input_slots, data_idx); itr != input_slots.end()){
					rst.push_back({
							static_cast<unsigned>(itr - input_slots.begin()),
							static_cast<unsigned>(idx)
						});
				}
			}

			return rst;
		}
	};
}


template <>
struct std::hash<mo_yanxi::graphic::render_graph::slot_pair>{
	static std::size_t operator()(mo_yanxi::graphic::render_graph::slot_pair pair) noexcept{
		return std::hash<unsigned long long>{}(std::bit_cast<unsigned long long>(pair));
	}
};
