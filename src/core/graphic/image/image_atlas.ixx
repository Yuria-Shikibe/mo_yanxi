module;

#include <vulkan/vulkan.h>
#include <cassert>
#include <vk_mem_alloc.h>

export module mo_yanxi.graphic.image_manage;

export import mo_yanxi.graphic.image_region;
export import mo_yanxi.graphic.color;
export import mo_yanxi.graphic.bitmap;

import mo_yanxi.referenced_ptr;
import mo_yanxi.meta_programming;
import mo_yanxi.handle_wrapper;
import mo_yanxi.allocator_2D;

import mo_yanxi.vk;
import mo_yanxi.vk.util;
import mo_yanxi.vk.cmd;

import mo_yanxi.heterogeneous;
import mo_yanxi.condition_variable_single;

import mo_yanxi.graphic.msdf;
import mo_yanxi.io.image;

import std;

namespace mo_yanxi::graphic{
	constexpr math::usize2 DefaultTexturePageSize = math::vectors::constant2<std::uint32_t>::base_vec2 * (1 << 12);

	struct bad_image_allocation : std::exception{

	};

	export struct sub_page;
	export struct image_page;

	using region_type = combined_image_region<size_awared_uv<uniformed_rect_uv>>;

	export struct allocated_image_region :
		region_type,
		referenced_object{
	protected:
		math::urect region{};
		exclusive_handle_member<sub_page*> page{};

	public:
		[[nodiscard]] constexpr allocated_image_region() = default;

		[[nodiscard]] constexpr allocated_image_region(
			sub_page& page,
			VkImageView imageView,
			const math::usize2 image_size,
			const math::urect region
			)
			: combined_image_region{.view = imageView}, region(region), page{&page}{

			uv.fetch_into(image_size, region);
		}

		~allocated_image_region();

		allocated_image_region(const allocated_image_region& other) = delete;
		allocated_image_region(allocated_image_region&& other) noexcept = default;
		allocated_image_region& operator=(const allocated_image_region& other) = delete;
		allocated_image_region& operator=(allocated_image_region&& other) noexcept = default;

		explicit operator bool() const noexcept{
			return view != nullptr;// && get_ref_count() > 0;
		}

		[[nodiscard]] math::urect get_region() const noexcept{
			return region;
		}

		allocated_image_region& rotate(int times = 1) noexcept{
			uv.rotate(times);
			return *this;
		}

		[[nodiscard]] sub_page* get_page() const noexcept{
			return page;
		}

		using referenced_object::droppable;
		using referenced_object::ref_decr;
		using referenced_object::ref_incr;
	};

	export struct borrowed_image_region : referenced_ptr<allocated_image_region, no_deletion_on_ref_count_to_zero>{
		[[nodiscard]] constexpr explicit(false) borrowed_image_region(allocated_image_region* object)
			: referenced_ptr(object){
		}

		[[nodiscard]] constexpr explicit(false) borrowed_image_region(allocated_image_region& object)
			: referenced_ptr(std::addressof(object)){
		}

		[[nodiscard]] constexpr borrowed_image_region() = default;

		constexpr operator combined_image_region<uniformed_rect_uv>() const{
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

	struct bad_image_asset : public std::exception{
		[[nodiscard]] bad_image_asset() = default;
	};

	struct bitmap_represent{
		using byte_span = std::span<const std::byte>;

		std::variant<byte_span, bitmap> present{};

		[[nodiscard]] constexpr bitmap_represent() = default;

		template <typename T>
			requires std::constructible_from<decltype(present), T>
		[[nodiscard]] constexpr explicit(false) bitmap_represent(T&& present)
			: present(std::forward<T>(present)){
		}

		[[nodiscard]] constexpr byte_span get_bytes() const noexcept{
			return std::visit([] <typename T> (const T& data){
				if constexpr (std::same_as<T, bitmap>){
					const std::span<const bitmap::value_type> s = data.to_span();
					return byte_span{reinterpret_cast<const std::byte*>(s.data()), s.size_bytes()};
				}else{
					return data;
				}
			}, present);
		}
	};

	export
	struct bitmap_path_load{
		std::string path{};

		[[nodiscard]] math::usize2 get_extent() const{
			const auto ext = io::image::read_image_extent(path.c_str());
			if(!ext){
				throw bad_image_asset{};
			}
			return std::bit_cast<math::usize2>(ext.value());
		}

		bitmap_represent operator()(unsigned w, unsigned h, unsigned level) const{
			return bitmap(path);
		}
	};

	export
	struct bitmap_load{
		bitmap bitmap{};

		[[nodiscard]] math::usize2 get_extent() const{
			return bitmap.extent();
		}

		bitmap_represent operator()(unsigned w, unsigned h, unsigned level) const{
			if(!bitmap){
				return bitmap_represent{};
			}
			return bitmap.to_byte_span();
		}
	};

	export
	struct sdf_load{
		std::variant<
			msdf::msdf_generator,
			msdf::msdf_glyph_generator_crop
		> generator{};

		math::usize2 extent{};
		std::uint32_t prov_levels{3};

		[[nodiscard]] math::usize2 get_extent() const noexcept{
			return extent;
		}

		bitmap_represent operator()(unsigned w, unsigned h, unsigned level) const {
			return std::visit<bitmap>([&]<vk::texture_source_prov T>(const T& prov){
				return prov.operator()(w, h, level);
			}, generator);
		}
	};

	export
	struct image_load_description{
		std::variant<bitmap_path_load, sdf_load, bitmap_load> raw{};

		[[nodiscard]] math::usize2 get_extent() const{
			return std::visit([](const auto& v){
				return v.get_extent();
			}, raw);
		}

		[[nodiscard]] std::uint32_t get_prov_levels() const noexcept{
			return std::visit([]<typename T>(const T& t) -> std::uint32_t {
				if constexpr (std::same_as<T, sdf_load>){
					return t.prov_levels;
				}else{
					return 1;
				}
			}, raw);
		}

		[[nodiscard]] bitmap_represent get(unsigned w, unsigned h, unsigned level) const{
			return std::visit<bitmap_represent>([&]<typename T>(const T& gen){
				return gen(w, h, level);
			}, raw);
		}
	};

	struct allocated_image_load_description{
		vk::image_handle texture{};
		std::uint32_t mip_level{};
		std::uint32_t layer_index{};

		image_load_description desc{};
		math::urect region{};

	};

	struct async_image_loader{
	private:
		vk::context* context_{};
		vk::fence fence_{};

		vk::command_pool command_pool_{};

		vk::command_buffer running_command_buffer_{};
		vk::allocator async_allocator_{};
		vk::buffer using_buffer_{};

		std::multimap<VkDeviceSize, vk::buffer> stagings{};
		std::mutex queue_mutex{};
		condition_variable_single queue_cond{};
		std::vector<allocated_image_load_description> queue{};

		std::atomic_flag loading{};

		std::jthread working_thread{};

		static void work_func(std::stop_token stop_token, async_image_loader& self){
			std::vector<allocated_image_load_description> dumped_queue{};

			while(!stop_token.stop_requested()){
				{
					std::unique_lock lock(self.queue_mutex);

					self.queue_cond.wait(lock, [&]{
						return !self.queue.empty() || stop_token.stop_requested();
					});

					dumped_queue = std::exchange(self.queue, {});
				}

				self.loading.test_and_set(std::memory_order::relaxed);
				for (auto& desc : dumped_queue){
					if(stop_token.stop_requested())break;
					self.load(std::move(desc));
				}
				self.loading.clear(std::memory_order::release);
				self.loading.notify_all();
			}

			self.fence_.wait();
		}

		void load(allocated_image_load_description&& desc);

	public:
		[[nodiscard]] async_image_loader() = default;

		[[nodiscard]] explicit async_image_loader(vk::context& context)
			:
		context_(&context),
		fence_(context.get_device(), true),
		command_pool_(context.get_device(), context.graphic_family(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT),
		async_allocator_(context.create_allocator(VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT)),
		working_thread([](std::stop_token stop_token, async_image_loader& self){
			work_func(std::move(stop_token), self);
		}, std::ref(*this)){
		}

		void push(allocated_image_load_description&& desc){
			{
				std::lock_guard lock(queue_mutex);
				queue.push_back(std::move(desc));
			}
			queue_cond.notify_one();
		}

		[[nodiscard]] vk::context& context() const noexcept{
			assert(context_);
			return *context_;
		}

		void wait() const noexcept{
			if(working_thread.joinable()){
				loading.wait(true, std::memory_order::acquire);
			}
		}

		[[nodiscard]] bool is_loading() const noexcept{
			return loading.test(std::memory_order::acquire);
		}

		~async_image_loader(){
			working_thread.request_stop();
			queue_cond.notify_one();
			wait();
			fence_.wait();
		}
	};

	struct page_acquire_result{
		allocated_image_region region;
		vk::texture* handle;
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

		//TODO merge similar code
		//TODO dont use bitcast of rects here?

		std::optional<page_acquire_result> acquire(
			const math::usize2 extent,
			const std::uint32_t margin
		){
			if(const auto pos = allocate(extent.copy().add(margin))){
				const math::urect rst{tags::unchecked, tags::from_extent, pos.value(), extent};

				return page_acquire_result{
						allocated_image_region{
							*this, texture.get_image_view(),
							std::bit_cast<math::usize2>(texture.get_image().get_extent2()), rst
						},
						&texture
					};
			}

			return std::nullopt;
		}

		std::optional<allocated_image_region> push(
			VkCommandBuffer commandBuffer,
			VkBuffer buffer,
			const math::usize2 extent,
			const std::uint32_t margin
		){
			if(const auto pos = allocate(extent.copy().add(margin))){
				const math::urect rst{tags::from_extent, pos.value(), extent};

				texture.write(commandBuffer,
					{vk::texture_buffer_write{
						buffer,
						{std::bit_cast<VkOffset2D>(pos->as<std::int32_t>()), std::bit_cast<VkExtent2D>(extent)}
					}});

				return allocated_image_region{
					*this, texture.get_image_view(),
					std::bit_cast<math::usize2>(texture.get_image().get_extent2()), rst};
			}


			return std::nullopt;
		}
		std::optional<std::pair<allocated_image_region, vk::buffer>> push(
			VkCommandBuffer commandBuffer,
			vk::texture_source_prov auto bitmaps_prov,
			const math::usize2 extent,
			const std::uint32_t max_gen_depth,
			const std::uint32_t margin
		){
			if(const auto pos = allocate(extent.copy().add(margin))){
				const math::urect rst{tags::from_extent, pos.value(), extent};

				VkRect2D rect{std::bit_cast<VkOffset2D>(pos->as<std::int32_t>()), std::bit_cast<VkExtent2D>(extent)};
				vk::buffer buf = texture.write(commandBuffer, rect, std::ref(bitmaps_prov), max_gen_depth);

				return std::optional<std::pair<allocated_image_region, vk::buffer>>{
						std::in_place,
						allocated_image_region{
							*this, texture.get_image_view(),
							std::bit_cast<math::usize2>(texture.get_image().get_extent2()), rst
						},
						std::move(buf)
					};
			}

			return std::nullopt;
		}
	};

	struct image_page{
		static constexpr std::string_view name_main = "main";
		static constexpr std::string_view name_ui = "ui";
		static constexpr std::string_view name_font = "font";
		static constexpr std::string_view name_temp = "temp";

	private:
		async_image_loader* loader{};
		math::usize2 page_size{};
		std::deque<sub_page> subpages{};
		color clear_color{};
		std::uint32_t margin{};

		string_hash_map<allocated_image_region> named_image_regions{};
		std::unordered_set<allocated_image_region*> protected_regions{};

	public:
		[[nodiscard]] image_page() = default;

		[[nodiscard]] image_page(
			async_image_loader& loader,
			const math::usize2 page_size = DefaultTexturePageSize,
			const color& clear_color = {},
			const std::uint32_t margin = 4)
			: loader(&loader),
			  page_size(page_size),
			  clear_color(clear_color),
			  margin(margin){
		}

	private:
		auto& add_page(VkCommandBuffer command_buffer){
			//TODO dynamic page size to handle large image?
			auto& subpage = subpages.emplace_back(loader->context(), VkExtent2D{page_size.x, page_size.y});

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

		allocated_image_region async_load(image_load_description&& desc, page_acquire_result&& result) const{
			loader->push({
				.texture = *result.handle,
				.mip_level = result.handle->get_mip_level(),
				.layer_index = 0,
				.desc = std::move(desc),
				.region = result.region.get_region()
			});

			return std::move(result.region);
		}

	public:
		allocated_image_region async_allocate(image_load_description&& desc){
			auto extent = desc.get_extent();

			for(auto& subpass : subpages){
				if(auto rst = subpass.acquire(extent, margin)){
					return async_load(std::move(desc), std::move(rst.value()));
				}
			}

			clear_unused();

			for(auto& subpass : subpages){
				if(auto rst = subpass.acquire(extent, margin)){
					return async_load(std::move(desc), std::move(rst.value()));
				}
			}

			auto& newSubpage = add_page(loader->context().get_transient_graphic_command_buffer());
			auto rst = newSubpage.acquire(extent, margin);

			if(!rst){
				throw bad_image_allocation{};
			}

			return async_load(std::move(desc), std::move(rst.value()));
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

			clear_unused();

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

		[[nodiscard]] allocated_image_region allocate(
			vk::texture_source_prov auto bitmaps_prov,
			math::usize2 extent,
			const std::uint32_t max_gen_depth
		){
			auto cmd_buf = loader->context().get_transient_graphic_command_buffer();
			for(auto& subpass : subpages){
				if(auto rst = subpass.push(cmd_buf, std::ref(bitmaps_prov), extent, max_gen_depth, margin)){
					cmd_buf = {};
					return std::move(rst->first);
				}
			}

			auto& newSubpage = add_page(cmd_buf);
			auto rst = newSubpage.push(cmd_buf, std::ref(bitmaps_prov), extent, max_gen_depth, margin);
			cmd_buf = {};

			if(!rst){
				throw std::invalid_argument("Invalid region size");
			}

			return std::move(rst->first);
		}

		allocated_image_region allocate(const bitmap& bitmap){
			auto buffer = vk::templates::create_staging_buffer(loader->context().get_allocator(), bitmap.size_bytes());
			(void)vk::buffer_mapper{buffer}.load_range(bitmap.to_span());
			return allocate(loader->context().get_transient_graphic_command_buffer(), buffer, bitmap.extent());
		}

		std::pair<allocated_image_region&, bool> register_named_region(
			std::string&& name,
			const bitmap& region){
			if(auto itr = named_image_regions.find(name); itr != named_image_regions.end()){
				return {itr->second, false};
			}
			auto [itr, rst] = named_image_regions.try_emplace(std::move(name), allocate(region));
			return {itr->second, rst};
		}

		std::pair<allocated_image_region&, bool> register_named_region(
			const std::string_view name,
			const bitmap& region){
			if(auto itr = named_image_regions.find(name); itr != named_image_regions.end()){
				return {itr->second, false};
			}
			auto [itr, rst] = named_image_regions.try_emplace(name, allocate(region));
			return {itr->second, rst};
		}

		std::pair<allocated_image_region&, bool> register_named_region(
			std::string&& name,
			image_load_description&& desc){
			if(auto itr = named_image_regions.find(name); itr != named_image_regions.end()){
				return {itr->second, false};
			}
			auto [itr, rst] = named_image_regions.try_emplace(std::move(name), async_allocate(std::move(desc)));
			return {itr->second, rst};
		}

		std::pair<allocated_image_region&, bool> register_named_region(
			const std::string_view name,
			image_load_description&& desc){
			if(auto itr = named_image_regions.find(name); itr != named_image_regions.end()){
				return {itr->second, false};
			}
			auto [itr, rst] = named_image_regions.try_emplace(name, async_allocate(std::move(desc)));
			return {itr->second, rst};
		}

		template <typename Str, typename T>
			requires (std::constructible_from<image_load_description, T> && std::constructible_from<std::string_view, Str>)
		std::pair<allocated_image_region&, bool> register_named_region(
			Str&& name,
			T&& desc,
			const bool is_protected = false){

			std::string_view sv{name};
			if(auto itr = named_image_regions.find(sv); itr != named_image_regions.end()){
				return {itr->second, false};
			}
			auto [itr, rst] = named_image_regions.try_emplace(std::move(name), this->async_allocate(image_load_description{std::forward<T>(desc)}));
			if(is_protected){
				mark_protected(sv);
			}
			return {itr->second, rst};
		}

		std::pair<allocated_image_region&, bool> register_named_region(
			bitmap_path_load&& desc,
			const bool is_protected = false){
			return register_named_region(std::string_view{desc.path}, std::move(desc), is_protected);
		}

		std::pair<allocated_image_region&, bool> register_named_region(
			std::string&& name,
			vk::texture_source_prov auto bitmaps_prov,
			math::usize2 extent,
			const std::uint32_t max_gen_depth = 2
		){
			if(auto itr = named_image_regions.find(name); itr != named_image_regions.end()){
				return {itr->second, false};
			}
			auto [itr, rst] = named_image_regions.try_emplace(std::move(name), this->allocate(std::move(bitmaps_prov), extent, max_gen_depth));
			return {itr->second, rst};
		}

		std::pair<allocated_image_region&, bool> register_named_region(
			const std::string_view name,
			vk::texture_source_prov auto bitmaps_prov,
			math::usize2 extent,
			const std::uint32_t max_gen_depth = 3

		){
			if(auto itr = named_image_regions.find(name); itr != named_image_regions.end()){
				return {itr->second, false};
			}
			auto [itr, rst] = named_image_regions.try_emplace(name, this->allocate(std::move(bitmaps_prov), extent, max_gen_depth));
			return {itr->second, rst};
		}

		auto register_named_region(const char* name, const bitmap& region){
			return register_named_region(std::string_view{name}, region);
		}

		bool mark_protected(const std::string_view localName) noexcept{
			if(const auto page = named_image_regions.try_find(localName)){
				if(protected_regions.insert(page).second){
					//not inserted
					page->ref_incr();
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
					page->ref_decr();
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

		template <typename T>
		[[nodiscard]] auto* find(this T& self, const std::string_view localName) noexcept{
			return self.named_image_regions.try_find(localName);
		}

		template <typename T>
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
		std::unique_ptr<async_image_loader> async_image_loader_{};

	public:
		[[nodiscard]] image_atlas() = default;

		[[nodiscard]] explicit image_atlas(vk::context& context)
			: context_(&context), async_image_loader_{std::make_unique<async_image_loader>(context)}{
		}

		image_page& create_image_page(
			const std::string_view name,
			const color clearColor = {},
			const math::usize2 size = DefaultTexturePageSize,
			const std::uint32_t margin = 4){
			return pages.try_emplace(name, image_page{*async_image_loader_, size, clearColor, margin}).first->second;
		}

		image_page& operator[](
			const std::string_view name){
			return pages.try_emplace(name, image_page{*async_image_loader_, DefaultTexturePageSize, {}, 4}).first->second;
		}

		void wait_load() const noexcept{
			async_image_loader_->wait();
		}

		bool is_loading() const noexcept{
			return async_image_loader_->is_loading();
		}

		void clean_unused() noexcept{
			for(auto& page : pages | std::views::values){
				page.clear_unused();
			}
		}

		template <typename T>
		[[nodiscard]] image_page* find_page(this T& self, const std::string_view name) noexcept{
			return self.pages.try_find(name);
		}

		template <typename T>
		allocated_image_region* find(this T& self, const std::string_view name_category_local) noexcept{
			const auto [category, localName] = splitKey(name_category_local);
			if(const auto page = self.find_page(category)){
				return page->find(localName);
			}

			return nullptr;
		}

		template <typename T>
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

	public:
		image_atlas(const image_atlas& other) = delete;
		image_atlas(image_atlas&& other) noexcept = default;
		image_atlas& operator=(const image_atlas& other) = delete;
		image_atlas& operator=(image_atlas&& other) noexcept = default;

		~image_atlas() = default;
	};

	
	/*
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
		 #1#
		[[nodiscard]] allocated_image_region* find(const std::string_view name) const noexcept{
			return page_->find(name);
		}

		/**
		 * @param name in format of "<category>.<name>"
		 #1#
		[[nodiscard]] allocated_image_region& at(const std::string_view name) const{
			return page_->at(name);
		}
	};
	*/


	allocated_image_region::~allocated_image_region(){
		if(page)page->deallocate(region.src);
	}

}
