module;

#include <vulkan/vulkan.h>
#include <cassert>
#include <vk_mem_alloc.h>
#include "plf_hive.h"

export module mo_yanxi.graphic.image_atlas;

export import mo_yanxi.graphic.image_atlas.util;
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
import mo_yanxi.vk.universal_handle;

import mo_yanxi.heterogeneous;
import mo_yanxi.condition_variable_single;

import mo_yanxi.graphic.msdf;
import mo_yanxi.io.image;

import std;

namespace mo_yanxi::graphic{
constexpr math::usize2 DefaultTexturePageSize = math::vectors::constant2<std::uint32_t>::base_vec2 * (4096);

struct image_page_base;

using region_type = combined_image_region<size_awared_uv<uniformed_rect_uv>>;


export
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
		return std::visit([] <typename T>(const T& data){
			if constexpr(std::same_as<T, bitmap>){
				const std::span<const bitmap::value_type> s = data.to_span();
				return byte_span{reinterpret_cast<const std::byte*>(s.data()), s.size_bytes()};
			} else{
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

	bitmap_represent operator()(unsigned w, unsigned h, unsigned level) const{
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
		return std::visit([]<typename T>(const T& t) -> std::uint32_t{
			if constexpr(std::same_as<T, sdf_load>){
				return t.prov_levels;
			} else{
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

export
struct allocated_image_load_description{
	vk::image_handle texture{};
	std::uint32_t mip_level{};
	std::uint32_t layer_index{};

	image_load_description desc{};
	math::urect region{};
};

struct async_image_loader{
private:
	VkQueue working_queue_{};
	vk::allocator async_allocator_{};
	vk::command_pool command_pool_{};
	vk::command_buffer running_command_buffer_{};
	vk::fence fence_{};

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
			for(auto& desc : dumped_queue){
				if(stop_token.stop_requested()) break;
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

	[[nodiscard]] explicit async_image_loader(
		const vk::context_info& context,
		std::uint32_t graphic_family_index,
		VkQueue working_queue
		)
	:
	working_queue_{working_queue},
	async_allocator_(vk::allocator(context, VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT)),
	command_pool_(context.device, graphic_family_index, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT),
	fence_(context.device, true),
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

	void wait() const noexcept{
		if(working_thread.joinable()){
			loading.wait(true, std::memory_order::acquire);
		}
	}

	[[nodiscard]] bool is_loading() const noexcept{
		return loading.test(std::memory_order::relaxed);
	}

	~async_image_loader(){
		working_thread.request_stop();
		queue_cond.notify_one();
		wait();
		fence_.wait();
	}
};

void prepare_command(VkCommandBuffer commandBuffer){
	static constexpr VkCommandBufferBeginInfo BeginInfo{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		nullptr,
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	vkBeginCommandBuffer(commandBuffer, &BeginInfo);

}

void submit_command(VkCommandBuffer commandBuffer, VkQueue queue){
	vkEndCommandBuffer(commandBuffer);

	const VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer,
	};

	vkQueueSubmit(queue, 1, &submitInfo, nullptr);
}

struct image_register_result{
	allocated_image_region& region;
	bool inserted;
};

struct image_page_base{
private:
	async_image_loader* loader_{};
	vk::allocator* allocator_{};
	vk::command_buffer temp_commandbuf_{};
	VkQueue queue_{};

	math::usize2 page_size_{};
	plf::hive<sub_page> subpages_{};
	VkClearColorValue clear_color_{};
	std::uint32_t margin{};

	string_hash_map<allocated_image_region> named_image_regions{};
	std::unordered_set<allocated_image_region*> protected_regions{};

public:
	[[nodiscard]] image_page_base() = default;

	[[nodiscard]] explicit image_page_base(
		async_image_loader& loader,
		vk::allocator& allocator,
		vk::command_buffer&& command_buffer,
		VkQueue queue,
		const math::usize2 page_size = DefaultTexturePageSize,
		const VkClearColorValue& clear_color = {},
		const std::uint32_t margin = 4)
	:
	loader_(&loader),
	allocator_(&allocator),
	temp_commandbuf_(std::move(command_buffer)),
	queue_(queue),
	page_size_(page_size),
	clear_color_(clear_color),
	margin(margin){
	}

private:
	sub_page& add_page(vk::allocator& allocator, VkCommandBuffer command_buffer){
		//TODO dynamic page size to handle large image?
		auto& subpage = *subpages_.emplace(allocator, VkExtent2D{page_size_.x, page_size_.y});

		vk::cmd::clear_color(
			command_buffer,
			subpage.texture.get_image(),
			clear_color_,
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
		loader_->push({
				.texture = *result.handle,
				.mip_level = result.handle->get_mip_level(),
				.layer_index = 0,
				.desc = std::move(desc),
				.region = result.region.get_region()
			});

		return std::move(result.region);
	}

public:
	allocated_image_region async_allocate(
		image_load_description&& desc
	){
		const auto extent = desc.get_extent();

		for(auto& subpass : subpages_){
			if(auto rst = subpass.acquire(extent, margin)){
				return async_load(std::move(desc), std::move(rst.value()));
			}
		}

		clear_unused();

		for(auto& subpass : subpages_){
			if(auto rst = subpass.acquire(extent, margin)){
				return async_load(std::move(desc), std::move(rst.value()));
			}
		}

		prepare_command(temp_commandbuf_);
		auto& newSubpage = add_page(*allocator_, temp_commandbuf_);
		submit_command(temp_commandbuf_, queue_);
		auto rst = newSubpage.acquire(extent, margin);

		if(!rst){
			throw bad_image_allocation{};
		}

		return async_load(std::move(desc), std::move(rst.value()));
	}

	//TODO support offset
	[[nodiscard]] allocated_image_region allocate(
		VkBuffer buffer,
		math::usize2 extent
	){
		prepare_command(temp_commandbuf_);

		for(auto& subpass : subpages_){
			if(auto rst = subpass.push(temp_commandbuf_, buffer, extent, margin)){
				submit_command(temp_commandbuf_, queue_);
				return std::move(rst.value());
			}
		}

		clear_unused();

		for(auto& subpass : subpages_){
			if(auto rst = subpass.push(temp_commandbuf_, buffer, extent, margin)){
				submit_command(temp_commandbuf_, queue_);
				return std::move(rst.value());
			}
		}

		auto& newSubpage = add_page(*allocator_, temp_commandbuf_);
		auto rst = newSubpage.push(temp_commandbuf_, buffer, extent, margin);

		if(!rst){
			vkEndCommandBuffer(temp_commandbuf_);
			vkResetCommandBuffer(temp_commandbuf_, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
			throw std::invalid_argument("Invalid region size");
		}

		submit_command(temp_commandbuf_, queue_);
		return std::move(rst.value());
	}

	template <typename Str, typename T>
		requires (std::constructible_from<image_load_description, T> && std::constructible_from<std::string_view, const Str&>)
	image_register_result register_named_region(
		Str&& name,
		T&& desc,
		const bool mark_as_protected = false){
		std::string_view sv{std::as_const(name)};

		if(const auto itr = named_image_regions.find(sv); itr != named_image_regions.end()){
			return {itr->second, false};
		}

		auto [itr, inserted] = named_image_regions.emplace(
			std::forward<Str>(name),
			this->async_allocate(image_load_description{std::forward<T>(desc)})
		);

		if(mark_as_protected && inserted){
			mark_protected(sv);
		}

		return {itr->second, inserted};
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
	[[nodiscard]] auto& at(this T& self, const std::string_view localName){
		return self.named_image_regions.at(localName);
	}

	template <typename T>
	[[nodiscard]] auto& operator[](this T& self, const std::string_view localName){
		return self.named_image_regions.at(localName);
	}

protected:
	void drop(){
		for (auto& protected_region : protected_regions){
			protected_region->ref_decr();
		}
		if(!subpages_.empty())clear_unused();
	}
};

export
struct image_page : image_page_base{
	using image_page_base::image_page_base;

	~image_page(){
		drop();
	}

	image_page(const image_page& other) = delete;

	image_page(image_page&& other) noexcept = default;

	image_page& operator=(const image_page& other) = delete;

	image_page& operator=(image_page&& other) noexcept{
		if(this == &other) return *this;
		drop();
		image_page_base::operator =(std::move(other));
		return *this;
	}
};

export
struct image_atlas{
private:
	vk::allocator* allocator_{};
	vk::command_pool command_pool_{};
	VkQueue default_allocation_queue_{};
	string_hash_map<image_page> pages{};
	std::unique_ptr<async_image_loader> async_image_loader_{};

public:
	[[nodiscard]] image_atlas() = default;

	[[nodiscard]] image_atlas(
	vk::allocator& allocator,
	std::uint32_t graphic_family_index,
	VkQueue loader_working_queue,
	VkQueue default_allocation_queue
	) :
	allocator_(std::addressof(allocator)),
	command_pool_{allocator.get_device(), graphic_family_index, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT},
	default_allocation_queue_{default_allocation_queue},
	async_image_loader_{std::make_unique<async_image_loader>(allocator.get_context_info(), graphic_family_index, loader_working_queue)}{
	}

	image_page& create_image_page(
		const std::string_view name,
		const VkClearColorValue& clearColor = {},
		const math::usize2 size = DefaultTexturePageSize,
		const std::uint32_t margin = 4){
		return pages.try_emplace(name, image_page{*async_image_loader_, *allocator_,  command_pool_.obtain(), default_allocation_queue_, size, clearColor, margin}).first->second;
	}

	image_page& operator[](const std::string_view name){
		if(auto itr = pages.find(name); itr != pages.end()){
			return itr->second;
		}else{
			return pages.try_emplace(name, image_page{*async_image_loader_, *allocator_,  command_pool_.obtain(), default_allocation_queue_, DefaultTexturePageSize, {}, 4}).first->second;
		}

	}

	void wait_load() const noexcept{
		async_image_loader_->wait();
	}

	[[nodiscard]] bool is_loading() const noexcept{
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

		//TODO no print here?
		std::println(std::cerr, "TextureRegion Not Found: {}", name_category_local);
		throw std::out_of_range("Undefined Region Name");
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

}
