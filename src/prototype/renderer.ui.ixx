module;

#include <cassert>
#include <vulkan/vulkan.h>

export module prototype.renderer.ui;

import mo_yanxi.math.rect_ortho;
import mo_yanxi.math.vector2;
import mo_yanxi.math.matrix3;

import mo_yanxi.graphic.draw.instruction.batch;
import mo_yanxi.vk.util.uniform;
import mo_yanxi.vk;
import mo_yanxi.vk.cmd;


import std;

namespace mo_yanxi::gui{

struct scissor{
	math::vec2 src{};
	math::vec2 dst{};
	float margin{};


	constexpr friend bool operator==(const scissor& lhs, const scissor& rhs) noexcept = default;
};

struct scissor_raw{
	math::frect rect{};
	float margin{};

	[[nodiscard]] scissor_raw intersection_with(const scissor_raw& other) const noexcept{
		return {rect.intersection_with(other.rect), margin};
	}

	void uniform(const math::mat3& mat) noexcept{
		if(rect.area() < 0.05f){
			rect = {};
			return;
		}

		auto src = rect.get_src();
		auto dst = rect.get_end();
		src *= mat;
		dst *= mat;

		rect = {src, dst};
	}

	constexpr friend bool operator==(const scissor_raw& lhs, const scissor_raw& rhs) noexcept = default;

	constexpr explicit(false) operator scissor() const noexcept{
		return scissor{rect.get_src(), rect.get_end(), margin};
	}
};

struct layer_viewport{
	struct transform_pair{
		math::mat3 current;
		math::mat3 accumul;
	};

	math::frect viewport{};
	std::pmr::vector<scissor_raw> scissors{};

	math::mat3 transform_to_root_screen{};
	std::pmr::vector<transform_pair> element_local_transform{};

	[[nodiscard]] layer_viewport() = default;

	[[nodiscard]] layer_viewport(
		const math::frect& viewport,
		const scissor_raw& scissors,
		std::pmr::memory_resource* alloc,
		const math::mat3& transform_to_root_screen)
		:
		viewport(viewport),
		scissors({scissors}, alloc),
		transform_to_root_screen(transform_to_root_screen),
		element_local_transform({{math::mat3_idt, math::mat3_idt}}, alloc){
	}

	[[nodiscard]] scissor_raw top_scissor() const noexcept{
		assert(!scissors.empty());
		return scissors.back();
	}

	void pop_scissor() noexcept{
		assert(scissors.size() > 1);
		scissors.pop_back();
	}

	void push_scissor(const scissor_raw& scissor){
		scissors.push_back(scissor.intersection_with(top_scissor()));
	}

	[[nodiscard]] math::mat3 get_element_to_root_screen() const noexcept{
		assert(!element_local_transform.empty());
		return transform_to_root_screen * element_local_transform.back().accumul;
	}

	void push_local_transform(const math::mat3& mat){
		assert(!element_local_transform.empty());
		auto& last = element_local_transform.back();
		element_local_transform.push_back({mat, last.accumul * mat});
	}

	void pop_local_transform() noexcept{
		assert(element_local_transform.size() > 1);
		element_local_transform.pop_back();
	}

	void set_local_transform(const math::mat3& mat) noexcept{
		assert(!element_local_transform.empty() && "cannot set empty transform");
		auto& last = element_local_transform.back();
		last.current = mat;
		if(element_local_transform.size() > 1){
			last.accumul = element_local_transform[element_local_transform.size() - 2].accumul * mat;
		}else{
			last.accumul = mat;
		}

	}
};

struct ubo_screen_info{
	vk::padded_mat3 screen_to_uniform;
};

struct alignas(16) ubo_layer_info{
	vk::padded_mat3 element_to_screen;
	scissor scissor;

	std::uint32_t cap[3];
};


export
struct renderer{
private:
	graphic::draw::instruction::ubo_index_table ubo_table_{graphic::draw::instruction::make_ubo_table<
		std::tuple<ubo_screen_info, ubo_layer_info>
		>()};

public:
	graphic::draw::instruction::batch batch{};
private:
	graphic::draw::instruction::batch_descriptor_slots general_descriptors_{};
	vk::uniform_buffer uniform_buffer_{};

	graphic::draw::instruction::batch_command_slots batch_commands_{};
	vk::pipeline_layout pipeline_layout_{};
	vk::pipeline pipeline_{};

	// vk::descriptor_layout descriptor_layout_{};
	// vk::descriptor_buffer descriptor_buffer_{};

	std::unique_ptr<std::pmr::unsynchronized_pool_resource> pool_resource{
			std::make_unique<std::pmr::unsynchronized_pool_resource>()
		};

	//screen space to uniform space viewport
	std::pmr::vector<layer_viewport> viewports{pool_resource.get()};
	math::mat3 uniform_proj{};

	vk::color_attachment attachment_base{};
	vk::color_attachment attachment_background{};

	math::urect region_{};

public:
	[[nodiscard]] renderer() = default;

	[[nodiscard]] explicit renderer(
		vk::context& ctx,
		VkSampler spr,
		const vk::shader_module& ui_main_shader
		)
		: batch(ctx, spr, ubo_table_)
	, general_descriptors_(ctx, [](vk::descriptor_layout_builder& b){
		b.push_seq(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT);
		b.push_seq(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT);
	})
	, uniform_buffer_(ctx.get_allocator(), ubo_table_.max_size() * batch.work_group_count(), VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
	, batch_commands_(ctx.get_graphic_command_pool())
	, pipeline_layout_(
		ctx.get_device(), 0,
{batch.get_batch_descriptor_layout(), general_descriptors_.descriptor_set_layout()})

	{
		general_descriptors_.bind([&](const std::uint32_t idx, const vk::descriptor_mapper& mapper){
			using T = graphic::draw::instruction::ubo_index_table;
			for(auto&& [group, rng] : ubo_table_.get_entries() | std::views::chunk_by([](const T::identity_entry& l, const T::identity_entry& r){
				return l.entry.group_index == r.entry.group_index;
			}) | std::views::enumerate){
				for (const auto & [binding, ientry] : rng | std::views::enumerate){
					auto& entry = ientry.entry;
					(void)mapper.set_uniform_buffer(
						binding,
						uniform_buffer_.get_address() + idx * ubo_table_.max_size() + entry.local_offset, entry.size,
						idx);
				}
			}

		});


		batch.set_submit_callback([&](const std::uint32_t idx, graphic::draw::instruction::batch::ubo_data_entries spn) -> VkCommandBuffer {
			if(spn){
				//TODO support multiple ubo
				vk::buffer_mapper mapper{uniform_buffer_};
				for (const auto & entry : spn.entries){
					if(!entry)continue;
					(void)mapper.load_range(entry.to_range(spn.base_address), entry.local_offset + idx * ubo_table_.max_size());
				}
			}

			return batch_commands_[idx];
		});

		init_pipeline(ui_main_shader);

	}

	void resize(math::urect region){
		if(region == this->region_)return;
		region_ = region;
		VkExtent2D extent{region.width(), region.height()};
		attachment_base =
			vk::color_attachment{
				context().get_allocator(), extent,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
				VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_FORMAT_R16G16B16A16_SFLOAT
			};
		attachment_background =
			vk::color_attachment{
				context().get_allocator(), extent,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
				VK_IMAGE_USAGE_SAMPLED_BIT
			};

		{
			auto transient = context().get_transient_graphic_command_buffer();
			attachment_base.init_layout(transient);
			attachment_background.init_layout(transient);
		}
		init_projection();
		record_command();
	}

	[[nodiscard]] vk::context& context() const noexcept{
		return batch.context();
	}

private:
	[[nodiscard]] VkRect2D get_screen_area() const noexcept{
		return {static_cast<std::int32_t>(region_.src.x), static_cast<std::int32_t>(region_.src.y), region_.width(), region_.height()};
	}

	void record_command(){
		vk::dynamic_rendering dynamic_rendering{
			{attachment_base.get_image_view()},
			nullptr};

		const graphic::draw::instruction::batch_descriptor_buffer_binding_info dbo_info{
			general_descriptors_.dbo(),
			general_descriptors_.dbo().get_chunk_size()
		};

		batch.record_command(pipeline_layout_, {&dbo_info, 1}, [&] -> std::generator<VkCommandBuffer&&> {
				for (const auto & [idx, buf] : batch_commands_ | std::views::enumerate){
					vk::scoped_recorder recorder{buf, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT};

					dynamic_rendering.begin_rendering(recorder, context().get_screen_area());
					pipeline_.bind(recorder, VK_PIPELINE_BIND_POINT_GRAPHICS);

					vk::cmd::set_viewport(recorder, get_screen_area());
					vk::cmd::set_scissor(recorder, get_screen_area());

					co_yield buf.get();

					vkCmdEndRendering(recorder);
				}
			}());
	}

	void init_pipeline(const vk::shader_module& shader_module){
		vk::graphic_pipeline_template gtp{};
		gtp.set_shaders({
			shader_module.get_create_info(VK_SHADER_STAGE_MESH_BIT_EXT, nullptr, "main_mesh"),
			shader_module.get_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, nullptr, "main_frag")
		});
		gtp.push_color_attachment_format(VK_FORMAT_R16G16B16A16_SFLOAT, vk::blending::alpha_blend);

		pipeline_ = vk::pipeline{context().get_device(), pipeline_layout_, VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT, gtp};
	}

	template <typename T>
	void update_ubo(const T& payload){
		batch.update_ubo(payload);
	}

	void init_projection(){
		math::frect initial_viewport = region_.as<float>();
		viewports.clear();
		viewports.push_back(layer_viewport{initial_viewport, {{initial_viewport}}, pool_resource.get(), math::mat3_idt});
		uniform_proj = math::mat3{}.set_orthogonal(initial_viewport.get_src(), initial_viewport.extent());

		update_ubo(ubo_screen_info{uniform_proj});
		notify_viewport_changed();
	}
public:
	void wait_idle(){
		batch.consume_all();
		batch.wait_all();
		assert(batch.is_all_done());
	}

	template <graphic::draw::instruction::known_instruction Instr, typename ...Args>
	void draw(const Instr& instr, const Args& ...args){
		batch.push_instruction(instr, args...);
	}

	void notify_viewport_changed(){
		auto& vp = top_viewport();
		update_ubo(ubo_layer_info{vp.get_element_to_root_screen(), vp.top_scissor()});
	}

	void push_viewport(const math::frect viewport, scissor_raw scissor_raw){
		assert(!viewports.empty());

		const auto& last = viewports.back();

		const auto trs = math::mat3{}.set_rect_transform(viewport.src, viewport.extent(), last.viewport.src, last.viewport.extent());
		const auto scissor_intersection = scissor_raw.intersection_with(last.top_scissor()).intersection_with({viewport});

		viewports.push_back(layer_viewport{viewport, {scissor_raw}, pool_resource.get(), last.get_element_to_root_screen() * trs});
	}

	void push_viewport(const math::frect viewport){
		push_viewport(viewport, {viewport});
	}

	void pop_viewport() noexcept{
		assert(viewports.size() > 1);
		viewports.pop_back();
	}

	void push_scissor(const scissor_raw& scissor_in_screen_space){
		top_viewport().push_scissor(scissor_in_screen_space);
	}

	void pop_scissor() noexcept {
		top_viewport().pop_scissor();
	}

	auto& top_viewport() noexcept{
		assert(!viewports.empty());
		return viewports.back();
	}


#pragma region Getter
	[[nodiscard]] vk::image_handle get_base() const noexcept{
		return attachment_base;
	}

	[[nodiscard]] const math::mat3& get_screen_uniform_proj() const noexcept{
		return uniform_proj;
	}
#pragma endregion
};

template <typename D>
struct guard_base{
private:
	friend D;
	renderer* renderer_;

public:
	[[nodiscard]] explicit guard_base(renderer& renderer)
		: renderer_(std::addressof(renderer)){
	}

	guard_base(const guard_base& other) = delete;

	guard_base(guard_base&& other) noexcept
		: renderer_{std::exchange(other.renderer_, {})}{
	}

	guard_base& operator=(const guard_base& other) = delete;

	guard_base& operator=(guard_base&& other) noexcept{
		if(this == &other) return *this;
		if(renderer_){
			static_cast<D*>(this)->pop();
		}
		renderer_ = std::exchange(other.renderer_, {});
		return *this;
	}

	~guard_base(){
		if(renderer_){
			static_cast<D*>(this)->pop();
		}
	}
};

export
struct viewport_guard : guard_base<viewport_guard>{
private:
	friend guard_base;

	void pop() const{
		renderer_->pop_viewport();
		renderer_->notify_viewport_changed();
	}

public:
	[[nodiscard]] viewport_guard(renderer& renderer, const math::frect viewport, const scissor_raw& scissor_raw) :
	guard_base(renderer){
		renderer.push_viewport(viewport, scissor_raw);
		renderer.notify_viewport_changed();
	}

	[[nodiscard]] viewport_guard(renderer& renderer, const math::frect viewport) :
	guard_base(renderer){
		renderer.push_viewport(viewport);
		renderer.notify_viewport_changed();
	}
};

export
struct scissor_guard : guard_base<scissor_guard>{
private:
	friend guard_base;

	void pop() const{
		renderer_->pop_scissor();
		renderer_->notify_viewport_changed();
	}

public:
	[[nodiscard]] scissor_guard(renderer& renderer, const scissor_raw& scissor_raw) :
	guard_base(renderer){
		renderer.push_scissor(scissor_raw);
		renderer.notify_viewport_changed();
	}

};

export
struct transform_guard : guard_base<transform_guard>{
private:
	friend guard_base;

	void pop() const{
		renderer_->top_viewport().pop_local_transform();
		renderer_->notify_viewport_changed();
	}

public:
	[[nodiscard]] transform_guard(renderer& renderer, const math::mat3& transform) :
	guard_base(renderer){
		renderer.top_viewport().push_local_transform(transform);
		renderer.notify_viewport_changed();
	}

};

}
