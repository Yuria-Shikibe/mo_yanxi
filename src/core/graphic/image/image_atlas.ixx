module;

#include <vulkan/vulkan.h>
#include <cassert>
//

export module mo_yanxi.graphic.image_atlas;

export import mo_yanxi.referenced_ptr;
export import mo_yanxi.graphic.image_region;
export import mo_yanxi.meta_programming;
export import mo_yanxi.handle_wrapper;
export import mo_yanxi.allocator_2D;
export import mo_yanxi.vk.image_derives;
export import mo_yanxi.vk.resources;
export import mo_yanxi.vk.context;
export import mo_yanxi.graphic.color;
export import mo_yanxi.graphic.bitmap;
export import mo_yanxi.heterogeneous;
import std;

namespace mo_yanxi::graphic{
	constexpr math::usize2 DefaultTexturePageSize = math::vectors::constant2<std::uint32_t>::base_vec2 * (1 << 13);


	export struct sub_page;
	export struct image_page;

	using region_type = combined_image_region<size_awared_uv<uniformed_rect_uv>>;

	export struct allocated_image_region :
		region_type,
		referenced_object<false>{
	protected:
		math::usize2 src{};
		exclusive_handle_member<sub_page*> page{};

	public:
		[[nodiscard]] constexpr allocated_image_region() = default;

		[[nodiscard]] constexpr allocated_image_region(
			sub_page& page,
			VkImageView imageView,
			const math::usize2 image_size,
			const math::urect region
			)
			: combined_image_region{.view = imageView}, page{&page}, src(region.get_src()){

			uv.fetch_into(image_size, region);
		}

		~allocated_image_region();

		allocated_image_region(const allocated_image_region& other) = delete;
		allocated_image_region(allocated_image_region&& other) noexcept = default;
		allocated_image_region& operator=(const allocated_image_region& other) = delete;
		allocated_image_region& operator=(allocated_image_region&& other) noexcept = default;

		using referenced_object::droppable;
		using referenced_object::decr_ref;
		using referenced_object::incr_ref;
	};

	export struct borrowed_image_region : referenced_ptr<allocated_image_region, false>{
		[[nodiscard]] constexpr explicit(false) borrowed_image_region(allocated_image_region* object)
			: referenced_ptr(object){
		}

		[[nodiscard]] constexpr explicit(false) borrowed_image_region(allocated_image_region& object)
			: referenced_ptr(object){
		}

		[[nodiscard]] constexpr borrowed_image_region() = default;

		constexpr operator combined_image_region<uniformed_rect_uv>(){
			if(*this){
				return *get();
			}else{
				return {};
			}
		}
	};

	export struct cached_image_region : borrowed_image_region{
	private:
		combined_image_region<uniformed_rect_uv> cache_{};

	public:
		[[nodiscard]] constexpr cached_image_region() = default;

		[[nodiscard]] constexpr explicit(false) cached_image_region(allocated_image_region* image_region)
			: borrowed_image_region(image_region){
			if(image_region){
				cache_ = *image_region;
			}
		}

		[[nodiscard]] constexpr explicit(false) cached_image_region(allocated_image_region& image_region)
			: borrowed_image_region(image_region){
			cache_ = image_region;
		}

		operator const combined_image_region<uniformed_rect_uv>& () const{
			return cache_;
		}

		[[nodiscard]] const combined_image_region<uniformed_rect_uv>& get_cache() const{
			return cache_;
		}
	};


	struct sub_page{
		friend allocated_image_region;
		friend image_page;
	private:
		vk::texture texture{};
		allocator2d allocator{};

		auto allocate(const math::usize2 size){
			return allocator.allocate(size);
		}

		auto deallocate(const math::usize2 point) noexcept {
			return allocator.deallocate(point);
		}


	public:
		[[nodiscard]] sub_page() = default;

		[[nodiscard]] explicit sub_page(vk::context& context, const VkExtent2D extent_2d)
			: texture(context.get_allocator(), extent_2d), allocator({extent_2d.width, extent_2d.height}){
		}

		std::optional<allocated_image_region> push(
			VkCommandBuffer commandBuffer,
			VkBuffer buffer,
			const math::usize2 region,
			const std::uint32_t margin
		){
			if(const auto pos = allocate(region.copy().add(margin))){
				const math::urect rst{tags::from_extent, pos.value(), region};

				texture.write(commandBuffer,
					{vk::texture_buffer_write{
						buffer,
						{std::bit_cast<VkOffset2D>(pos->as<std::int32_t>()), std::bit_cast<VkExtent2D>(region)}
					}});

				return allocated_image_region{
					*this, texture.get_image_view(),
					std::bit_cast<math::usize2>(texture.get_image().get_extent2()), rst};
			}

			return std::nullopt;
		}
	};


	struct image_page{
	private:
		vk::context* context{};
		math::usize2 page_size{};
		std::deque<sub_page> subpages{};
		color clear_color{};
		std::uint32_t margin{};

		string_hash_map<allocated_image_region> named_image_regions{};
		std::unordered_set<allocated_image_region*> protected_regions{};

	public:
		[[nodiscard]] image_page() = default;

		[[nodiscard]] image_page(
			vk::context& context,
			const math::usize2 page_size = DefaultTexturePageSize,
			const color& clear_color = {},
			const std::uint32_t margin = 4)
			: context(&context),
			  page_size(page_size),
			  clear_color(clear_color),
			  margin(margin){
		}

	private:
		auto& add_page(VkCommandBuffer command_buffer){
			auto& subpage = subpages.emplace_back(*context, VkExtent2D{page_size.x, page_size.y});

			vk::cmd::clear_color(
				command_buffer,
				subpage.texture.get_image(),
				{clear_color.r, clear_color.g, clear_color.b, clear_color.a},
				{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = subpage.texture.get_mip_level(),
					.baseArrayLayer = 0,
					.layerCount = subpage.texture.get_layers()
				},
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_ACCESS_NONE,
				VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);

			return subpage;
		}


		[[nodiscard]] allocated_image_region allocate(
			VkCommandBuffer commandBuffer,
			VkBuffer buffer,
			math::usize2 extent
		){
			for(auto& subpass : subpages){
				if(auto rst = subpass.push(commandBuffer, buffer, extent, margin)){
					return std::move(rst.value());
				}
			}

			auto& newSubpage = add_page(commandBuffer);
			auto rst = newSubpage.push(commandBuffer, buffer, extent, margin);

			if(!rst){
				throw std::invalid_argument("Invalid region size");
			}

			return std::move(rst.value());
		}

		allocated_image_region allocate(const bitmap& bitmap){
			auto buffer = vk::templates::create_staging_buffer(context->get_allocator(), bitmap.size_bytes());
			(void)vk::buffer_mapper{buffer}.load_range(bitmap.to_span());
			return allocate(context->get_transient_graphic_command_buffer(), buffer, bitmap.extent());
		}

	public:
		std::pair<allocated_image_region&, bool> register_named_region(std::string&& name, const bitmap& region){
			if(auto itr = named_image_regions.find(name); itr != named_image_regions.end()){
				return {itr->second, false};
			}
			auto [itr, rst] = named_image_regions.try_emplace(std::move(name), allocate(region));
			return {itr->second, rst};
		}

		std::pair<allocated_image_region&, bool> register_named_region(const std::string_view name, const bitmap& region){
			if(auto itr = named_image_regions.find(name); itr != named_image_regions.end()){
				return {itr->second, false};
			}
			auto [itr, rst] = named_image_regions.try_emplace(name, allocate(region));
			return {itr->second, rst};
		}

		auto register_named_region(const char* name, const bitmap& region){
			return register_named_region(std::string_view{name}, region);
		}

		bool mark_protected(const std::string_view localName) noexcept{
			if(const auto page = named_image_regions.try_find(localName)){
				if(protected_regions.insert(page).second){
					//not inserted
					page->incr_ref();
					return true;
				}

				//already marked
				return true;
			}

			return false;
		}

		bool mark_unprotected(const std::string_view localName) noexcept{
			if(const auto page = named_image_regions.try_find(localName)){
				if(protected_regions.erase(page)){
					page->decr_ref();
					return true;
				}
			}

			return false;
		}

		void clear_unused() noexcept{
			std::erase_if(named_image_regions, [](decltype(named_image_regions)::const_reference region){
				return region.second.droppable();
			});
		}

		template <std::derived_from<image_page> T>
		[[nodiscard]] auto* find(this T& self, const std::string_view localName) noexcept{
			return self.named_image_regions.try_find(localName);
		}

		template <std::derived_from<image_page> T>
		[[nodiscard]] auto& at(this T& self, const std::string_view localName) noexcept{
			return self.named_image_regions.at(localName);
		}

		~image_page() = default;

		image_page(const image_page& other) = default;
		image_page(image_page&& other) noexcept = default;
		image_page& operator=(const image_page& other) = default;
		image_page& operator=(image_page&& other) noexcept = default;
	};

	export
	struct image_atlas{
	private:
		vk::context* context_{};
		string_hash_map<image_page> pages{};

	public:
		[[nodiscard]] image_atlas() = default;

		[[nodiscard]] explicit image_atlas(vk::context& context)
			: context_(&context){
		}

		image_page& create_image_page(
			const std::string_view name,
			const color clearColor = {},
			const math::usize2 size = DefaultTexturePageSize,
			const std::uint32_t margin = 4){
			return pages.insert_or_assign(name, image_page{*context_, size, clearColor, margin}).first->second;
		}

		void clean_unused() noexcept{
			for(auto& page : pages | std::views::values){
				page.clear_unused();
			}
		}

		template <std::derived_from<image_atlas> T>
		[[nodiscard]] image_page* find_page(this T& self, const std::string_view name) noexcept{
			return self.pages.try_find(name);
		}

		template <std::derived_from<image_atlas> T>
		allocated_image_region* find(this T& self, const std::string_view name_category_local) noexcept{
			const auto [category, localName] = splitKey(name_category_local);
			if(const auto page = self.find_page(category)){
				return page->find(localName);
			}

			return nullptr;
		}

		template <std::derived_from<image_atlas> T>
		allocated_image_region& at(this T& self, const std::string_view name_category_local){
			const auto [category, localName] = splitKey(name_category_local);
			if(const auto page = self.find_page(category)){
				return *page->find(localName);
			}

			std::println(std::cerr, "TextureRegion Not Found: {}", name_category_local);
			throw std::out_of_range("Undefined Region Name");
		}

		[[nodiscard]] vk::context& context() const noexcept{
			assert(context_);
			return *context_;
		}

	private:
		static std::pair<std::string_view, std::string_view> splitKey(const std::string_view name){
			const auto pos = name.find('.');
			const std::string_view category = name.substr(0, pos);
			const std::string_view localName = name.substr(pos + 1);

			return {category, localName};
		}

		
	};

	
	export struct borrowed_image_page{
	private:
		image_atlas* atlas_{};
		image_page* page_{};

	public:
		[[nodiscard]] image_atlas& atlas() const noexcept{
			assert(atlas_);
			return *atlas_;
		}

		[[nodiscard]] image_page& page() const noexcept{
			assert(page_);
			return *page_;
		}

		[[nodiscard]] borrowed_image_page() = default;

		[[nodiscard]] borrowed_image_page(image_atlas& atlas, const std::string_view pageName)
			: atlas_(&atlas){
			page_ = atlas.find_page(pageName);
			if(page_ == nullptr){
				page_ = &atlas.create_image_page(pageName);
			}
		}

		[[nodiscard]] auto create(const std::string_view regionName, const bitmap& bitmap) const{
			return page().register_named_region(regionName, bitmap);
		}

		[[nodiscard]] auto create(std::string&& regionName, const bitmap& bitmap) const{
			return page().register_named_region(std::move(regionName), bitmap);
		}

		/**
		 * @param name in format of "<category>.<name>"
		 * @return Nullable
		 */
		[[nodiscard]] allocated_image_region* find(const std::string_view name) const noexcept{
			return page_->find(name);
		}

		/**
		 * @param name in format of "<category>.<name>"
		 */
		[[nodiscard]] allocated_image_region& at(const std::string_view name) const{
			return page_->at(name);
		}
	};


	allocated_image_region::~allocated_image_region(){
		if(page)page->deallocate(src);
	}

}
