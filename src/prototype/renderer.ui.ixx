//
// Created by Matrix on 2025/10/26.
//

export module prototype.renderer.ui;

import mo_yanxi.math.rect_ortho;
import mo_yanxi.math.vector2;
import mo_yanxi.math.matrix3;

import mo_yanxi.vk.util.uniform;

import std;

namespace mo_yanxi::ui{


struct ui_vertex_uniform{
	vk::padded_mat3 view;

	constexpr friend bool operator==(const ui_vertex_uniform& lhs, const ui_vertex_uniform& rhs) noexcept = default;
};


struct scissor{
	math::vec2 src{};
	math::vec2 dst{};
	float distance{};

	constexpr friend bool operator==(const scissor& lhs, const scissor& rhs) noexcept = default;
};

struct scissor_raw{
	math::frect rect{};
	float distance{};

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
		return scissor{rect.get_src(), rect.get_end(), distance};
	}
};

struct renderer{

private:
	std::unique_ptr<std::pmr::unsynchronized_pool_resource> pool_resource{std::make_unique<std::pmr::unsynchronized_pool_resource>()};

	std::pmr::vector<math::mat3> projections{pool_resource.get()};
	std::pmr::vector<math::frect> viewports{pool_resource.get()};
	std::pmr::vector<scissor_raw> scissors{pool_resource.get()};

	std::pmr::vector<math::mat3> merged_view_proj{pool_resource.get()};
public:

		// void push_scissor(scissor_raw scissor){
		// 	auto back = scissors.back();
		// 	if(back != scissor){
		// 		batch.consume_all();
		// 	}
		//
		// 	auto proj = get_cur_full_proj();
		// 	scissor.uniform(proj);
		// 	scissor.rect = scissor.rect.intersection_with(back.rect);
		//
		// 	scissors.push_back(scissor);
		// }
		//
		// void pop_scissor(){
		// 	if(scissors.size() >= 2 && scissors.back() == scissors.crbegin()[1]){
		//
		// 	}else{
		// 		batch.consume_all();
		// 	}
		//
		// 	scissors.pop_back();
		// 	assert(scissors.size() > 0);
		// }
		//
		// void push_projection(const math::frect viewport){
		// 	batch.consume_all();
		//
		// 	projections.push_back(math::mat3{}.set_orthogonal(viewport.src, viewport.extent()));
		// }
		//
		// [[nodiscard]] math::mat3 get_cur_proj() const{
		// 	assert(!projections.empty());
		// 	return current_viewport_transform * projections.back();
		// }
		//
		// [[nodiscard]] const scissor_raw& get_cur_scissor() const{
		// 	assert(!scissors.empty());
		// 	return scissors.back();
		// }
		//
		// void push_projection(const math::mat3& proj){
		// 	batch.consume_all();
		//
		// 	projections.push_back(proj);
		// }
		//
		// void pop_projection() noexcept{
		// 	batch.consume_all();
		//
		// 	assert(!projections.empty());
		// 	projections.pop_back();
		// }
		//
		// void push_viewport(const math::frect viewport){
		// 	batch.consume_all();
		// 	math::frect last_viewport = get_last_viewport();
		//
		// 	math::mat3 transform;
		// 	auto scale = viewport.extent() / last_viewport.extent();
		// 	transform.from_scaling(scale);
		// 	transform.set_translation(scale - math::vec2{1, 1} + viewport.src / last_viewport.extent() * 2);
		//
		// 	if(viewports.empty()){
		// 		current_viewport_transform = transform;
		// 	}else{
		// 		current_viewport_transform *= transform;
		// 	}
		//
		// 	viewports.push_back(viewport);
		// }
		//
		// void pop_viewport(){
		// 	batch.consume_all();
		// 	assert(!viewports.empty());
		//
		// 	auto viewport = viewports.back();
		// 	viewports.pop_back();
		//
		// 	math::frect last_viewport = get_last_viewport();
		//
		// 	math::mat3 transform;
		// 	const auto scale = viewport.extent() / last_viewport.extent();
		// 	transform.from_scaling(scale);
		// 	transform.set_translation(scale - math::vec2{1, 1} + viewport.src / last_viewport.extent() * 2);
		//
		// 	if(viewports.empty()){
		// 		current_viewport_transform = transform;
		// 	}else{
		// 		current_viewport_transform *= transform.inv();
		// 	}
		//
		// }
};
}