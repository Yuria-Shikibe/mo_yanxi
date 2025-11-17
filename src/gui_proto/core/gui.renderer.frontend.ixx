module;

#include <cassert>
#include <vulkan/vulkan.h>

export module mo_yanxi.gui.renderer.frontend;

import mo_yanxi.math.rect_ortho;
import mo_yanxi.math.vector2;
import mo_yanxi.math.matrix3;

export import mo_yanxi.graphic.draw.instruction.general;

import mo_yanxi.gui.alloc;
//TODO move this to other namespace
import mo_yanxi.vk.util.uniform;

import mo_yanxi.meta_programming;

import std;

namespace mo_yanxi::gui{

struct scissor{
	math::vec2 src{};
	math::vec2 dst{};

	//TODO margin is never used
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

export
struct layer_viewport{
	struct transform_pair{
		math::mat3 current;
		math::mat3 accumul;
	};

	math::frect viewport{};
	std::vector<scissor_raw> scissors{};

	math::mat3 transform_to_root_screen{};
	std::vector<transform_pair> element_local_transform{};

	[[nodiscard]] layer_viewport() = default;

	[[nodiscard]] layer_viewport(
		const math::frect& viewport,
		const scissor_raw& scissors,
		std::nullptr_t allocator_, //TODO
		const math::mat3& transform_to_root_screen)
		:
		viewport(viewport),
		scissors({scissors}/*, alloc*/),
		transform_to_root_screen(transform_to_root_screen),
		element_local_transform({{math::mat3_idt, math::mat3_idt}}/*, alloc*/){
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

struct alignas(16) ubo_screen_info{
	vk::padded_mat3 screen_to_uniform;
};

struct alignas(16) ubo_layer_info{
	vk::padded_mat3 element_to_screen;
	scissor scissor;

	std::uint32_t cap[3];
};

export
struct alignas(16) blit_config{
	math::rect_ortho_trivial<unsigned> blit_region;
};

export
enum struct draw_mode : std::uint32_t{
	def,
	msdf,

	COUNT_or_fallback,
};

export
struct alignas(16) draw_mode_param{
	draw_mode mode_;
	std::uint32_t _cap[3];
};


export
using gui_reserved_user_data_tuple = std::tuple<ubo_screen_info, ubo_layer_info, blit_config, draw_mode_param>;

export
using gui_reserved_user_data_tuple_uniform_buffer_used = std::tuple<ubo_screen_info, ubo_layer_info>;

template <typename T, typename TupV>
consteval std::size_t offset_of() noexcept{
	union S{
		T t{};
		std::byte b[sizeof(T)];

		constexpr S(){}

		constexpr ~S(){
			t.~T();
		}
	};

	S s{};

	const void* p = std::addressof(std::get<tuple_index_v<TupV, T>>(s.t));

	for (std::ptrdiff_t i = 0; i < sizeof(T); ++i){
		if(s.b + i == p)return i;
	}

	return static_cast<std::size_t>(std::bit_cast<std::uintptr_t>(nullptr));
}

export
template <typename T>
constexpr inline std::size_t data_offset_of = sizeof(gui_reserved_user_data_tuple) - sizeof(T) - gui::offset_of<gui_reserved_user_data_tuple, T>();


template <typename T>
constexpr inline graphic::draw::instruction::user_data_indices reserved_data_index_of{tuple_index_v<T, gui_reserved_user_data_tuple>, 0};

export
struct renderer_frontend{
private:
	graphic::draw::instruction::batch_backend_interface batch_backend_interface_{};
	math::frect region_{};

	//screen space to uniform space viewport
	mr::vector<layer_viewport> viewports{};
	math::mat3 uniform_proj{};

public:
	[[nodiscard]] renderer_frontend() = default;

	[[nodiscard]] explicit renderer_frontend(
		const graphic::draw::instruction::batch_backend_interface& batch_backend_interface)
	: batch_backend_interface_(batch_backend_interface){
	}

	[[nodiscard]] math::frect get_region() const noexcept{
		return region_;
	}

	auto& top_viewport(this auto& self) noexcept{
		assert(!self.viewports.empty());
		return self.viewports.back();
	}

	template <typename Instr, typename ...Args>
	void push(const Instr& instr, const Args& ...args) const {
		using namespace graphic::draw;
		if constexpr (instruction::known_instruction<Instr>){
			auto* next = batch_backend_interface_.acquire(instruction::get_instr_size<Instr, Args...>(args...));
			instruction::place_instruction_at(next, instr, args...);

		}else{
			static_assert(sizeof...(Args) == 0, "User Data with args is prohibited currently");

			if constexpr (reserved_data_index_of<Instr>.local_index == std::tuple_size_v<gui_reserved_user_data_tuple>){
				//TODO support user defined indices
				static_assert(false, "Unknown builtin type");
			}else{
				auto idcs = reserved_data_index_of<Instr>;
				auto* next = batch_backend_interface_.acquire(instruction::get_instr_size<Instr>());
				instruction::place_ubo_update_at(next, instr, reserved_data_index_of<Instr>);
			}
		}
	}

	void resize(math::frect region){
		if(region_ == region)return;
		region_ = region;
		init_projection();
	}

	void init_projection(){
		viewports.clear();
		viewports.push_back(layer_viewport{region_, {{region_}}, nullptr, math::mat3_idt});
		uniform_proj = math::mat3{}.set_orthogonal(region_.get_src(), region_.extent());

		push(ubo_screen_info{uniform_proj});
		notify_viewport_changed();
	}

	void flush() const{
		batch_backend_interface_.flush();
	}

	void consume() const{
		batch_backend_interface_.consume_all();
	}

	void notify_viewport_changed() const {
		const auto& vp = top_viewport();
		push(ubo_layer_info{vp.get_element_to_root_screen(), vp.top_scissor()});
	}

	void push_viewport(const math::frect viewport, scissor_raw scissor_raw){
		assert(!viewports.empty());

		const auto& last = viewports.back();

		const auto trs = math::mat3{}.set_rect_transform(viewport.src, viewport.extent(), last.viewport.src, last.viewport.extent());
		const auto scissor_intersection = scissor_raw.intersection_with(last.top_scissor()).intersection_with({viewport});

		viewports.push_back(layer_viewport{viewport, {scissor_raw}, nullptr, last.get_element_to_root_screen() * trs});
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

};

template <typename D>
struct guard_base{
private:
	friend D;
	renderer_frontend* renderer_;

public:
	[[nodiscard]] explicit guard_base(renderer_frontend& renderer)
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
	[[nodiscard]] viewport_guard(renderer_frontend& renderer, const math::frect viewport, const scissor_raw& scissor_raw) :
	guard_base(renderer){
		renderer.push_viewport(viewport, scissor_raw);
		renderer.notify_viewport_changed();
	}

	[[nodiscard]] viewport_guard(renderer_frontend& renderer, const math::frect viewport) :
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
	[[nodiscard]] scissor_guard(renderer_frontend& renderer, const scissor_raw& scissor_raw) :
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
	[[nodiscard]] transform_guard(renderer_frontend& renderer, const math::mat3& transform) :
	guard_base(renderer){
		renderer.top_viewport().push_local_transform(transform);
		renderer.notify_viewport_changed();
	}

};
export
struct mode_guard : guard_base<mode_guard>{
private:
	friend guard_base;

	void pop() const{
		renderer_->push(draw_mode_param{draw_mode::COUNT_or_fallback});
	}

public:
	[[nodiscard]] mode_guard(renderer_frontend& renderer, const draw_mode_param& param) :
	guard_base(renderer){
		renderer.push(param);
	}

};

}
