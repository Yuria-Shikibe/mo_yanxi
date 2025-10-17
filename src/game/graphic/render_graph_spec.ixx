module;

#include <vulkan/vulkan.h>

export module mo_yanxi.game.graphic.render_graph_spec;

export import mo_yanxi.graphic.render_graph.bloom;
export import mo_yanxi.graphic.render_graph.fill;
export import mo_yanxi.graphic.render_graph.ssao;

import mo_yanxi.vk.context;
import mo_yanxi.vk.image_derives;
import mo_yanxi.assets.graphic;
import std;

//TODO to clean up
import mo_yanxi.graphic.post_processor.oit_blender;
import mo_yanxi.graphic.render_graph.smaa_asserts;

namespace mo_yanxi::game::fx{
	using namespace graphic::render_graph;
	using namespace graphic::render_graph::resource_desc;

	export
	post_process_meta get_bloom_default_meta(){
		post_process_meta meta{
			assets::graphic::shaders::comp::bloom,
			{
										{{0}, {0, no_slot}}, //Input
										{{1}, {1, no_slot}},
										{{2}, {no_slot, 0}}, //Output
										{{3}, {no_slot, 0}},
									}
		};

		{
			auto& req = meta.sockets.at_out<image_requirement>(0);

			req.override_layout = req.override_output_layout = VK_IMAGE_LAYOUT_GENERAL;
			req.mip_levels = 6;
			req.scaled_times = 0;
			req.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		}

		{
			auto& req = meta.sockets.at_in<image_requirement>(1);

			req.override_layout = req.override_output_layout = VK_IMAGE_LAYOUT_GENERAL;
			req.mip_levels = 6;
			req.scaled_times = 1;
			req.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		}

		return meta;
	}

	export
	struct post_process_graph{
		render_graph_manager graph{};

		explicit_resource& ui_base{graph.add_explicit_resource(explicit_resource{external_image{}})};
		explicit_resource& ui_light{graph.add_explicit_resource(explicit_resource{external_image{}})};

		explicit_resource& world_blit_base{graph.add_explicit_resource(explicit_resource{external_image{}})};
		explicit_resource& world_blit_light{graph.add_explicit_resource(explicit_resource{external_image{}})};

		explicit_resource& world_draw_depth{graph.add_explicit_resource(explicit_resource{external_image{}})};
		explicit_resource& world_draw_base{graph.add_explicit_resource(explicit_resource{external_image{}})};
		explicit_resource& world_draw_light{graph.add_explicit_resource(explicit_resource{external_image{}})};

		explicit_resource& world_draw_resolved_depth{graph.add_explicit_resource(explicit_resource{external_image{}})};
		explicit_resource& world_draw_resolved_base{graph.add_explicit_resource(explicit_resource{external_image{}})};
		explicit_resource& world_draw_resolved_light{graph.add_explicit_resource(explicit_resource{external_image{}})};

		explicit_resource& world_draw_oit_head{graph.add_explicit_resource(explicit_resource{external_image{}})};
		explicit_resource& world_draw_oit_buffer{graph.add_explicit_resource(explicit_resource{external_buffer{}})};
		explicit_resource& world_draw_oit_buffer_stat{graph.add_explicit_resource(explicit_resource{external_buffer{}})};

	private:
		add_result<post_process_stage> final_merge{graph.add_stage<post_process_stage>(post_process_meta{
			assets::graphic::shaders::comp::result_merge, {
				{{0}, no_slot, 0},
				{{1}, 0, no_slot},
				{{2}, 1, no_slot},
				{{3}, 2, no_slot},
				{{4}, 3, no_slot},
			}
		})};

		bloom_pass* world_bloom_{};
		bloom_pass* ui_bloom_{};
		ssao_pass* ssao_pass_{};


		std::move_only_function<void(post_process_graph&)> updater{};

	public:

		[[nodiscard]] explicit post_process_graph(vk::context& context) : graph(context){
			final_merge.meta.sockets().at_out(0).get<image_requirement>().format = VK_FORMAT_R8G8B8A8_UNORM;
			final_merge.meta.sockets().at_out(0).get<image_requirement>().usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;


			const auto oit = graph.add_stage<post_process_stage>(post_process_meta{
					assets::graphic::shaders::comp::oit_blend, post_process_stage_inout_map{
						{{0}, 0, no_slot},
						{{1}, 1, no_slot},
						{{2}, 2, no_slot},
						{{3}, 3, no_slot},
						{{4}, 4, no_slot},
						{{5}, 5, 0},
						{{6}, 6, 1},
					}
				});
			oit.meta.set_sampler_at_binding(2, assets::graphic::samplers::blit_sampler);
			oit.pass.add_input({
				explicit_resource_usage{world_draw_oit_buffer, 0},
				explicit_resource_usage{world_draw_oit_head, 1},
				explicit_resource_usage{world_draw_resolved_depth, 2},
				explicit_resource_usage{world_draw_resolved_base, 3},
				explicit_resource_usage{world_draw_resolved_light, 4},
				explicit_resource_usage{world_blit_base, 5},
				explicit_resource_usage{world_blit_light, 6},
			});


			const auto bloom_meta = get_bloom_default_meta();

			const auto world_bloom = graph.add_stage<bloom_pass>(bloom_meta);
			world_bloom.meta.set_sampler_at_binding(0, assets::graphic::samplers::blit_sampler);
			world_bloom.pass.add_dep({&oit.pass, 1});

			const auto ssao = graph.add_stage<graphic::render_graph::ssao_pass>(post_process_meta{
					assets::graphic::shaders::comp::ssao, {
						{{0}, no_slot, 0},
						{{1}, 0, no_slot},
						{{2}, 1, no_slot},
					}
				});
			ssao.meta.set_sampler_at_binding(1, assets::graphic::samplers::blit_sampler);
			ssao.meta.set_sampler_at_binding(2, assets::graphic::samplers::blit_sampler);
			ssao.meta.sockets().at_in<image_requirement>(0).aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;
			ssao.pass.add_input({{world_draw_resolved_depth, 0}});
			ssao.pass.add_input({{world_draw_light, 1}});


			auto world_merge = graph.add_stage<post_process_stage>(post_process_meta{
					assets::graphic::shaders::comp::world_merge, {
						{{0}, no_slot, 0},
						{{1}, 0, no_slot},
						{{2}, 1, no_slot},
						{{3}, 2, no_slot},
						{{4}, 3, no_slot},
					}
				});
			world_merge.pass.add_dep({
					{oit.id(), 0, 0},
					{oit.id(), 1, 1},
					{ssao.id(), 0, 3},
					{world_bloom.id(), 0, 2}
				});

				auto smaa_edge_pass = graph.add_stage<post_process_stage>(post_process_meta{assets::graphic::shaders::comp::smaa::edge_detection, {
					   {{0}, no_slot, 0},
					   {{1}, 0, no_slot},
				   }});

				smaa_edge_pass.meta.set_sampler_at_binding(1, assets::graphic::samplers::blit_sampler);
				smaa_edge_pass.pass.add_dep({.id = world_merge.id()});

				auto smaa_weight_pass = graph.add_stage<post_process_stage>(post_process_meta{assets::graphic::shaders::comp::smaa::weight_calculate, {
					   {{0}, no_slot, 0},
					   {{1}, 0, no_slot},
					   {{2}, 1, no_slot},
					   {{3}, 2, no_slot},
				   }});

				auto& areaTex = graph.add_explicit_resource(explicit_resource{external_image{}});
				auto& searchTex = graph.add_explicit_resource(explicit_resource{external_image{}});

				smaa_weight_pass.meta.set_sampler_at_binding(1, assets::graphic::samplers::blit_sampler);
				smaa_weight_pass.meta.set_sampler_at_binding(2, assets::graphic::samplers::texture_sampler);
				smaa_weight_pass.meta.set_sampler_at_binding(3, assets::graphic::samplers::texture_sampler);
				smaa_weight_pass.pass.add_dep({.id = smaa_edge_pass.id()});
				smaa_weight_pass.pass.add_input({{areaTex, 1}});
				smaa_weight_pass.pass.add_input({{searchTex, 2}});

				const auto smaaTex = assets::graphic::get_smaa_tex();
				areaTex.as_image().handle = smaaTex.area_tex;
				areaTex.as_image().expected_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				searchTex.as_image().handle = smaaTex.search_tex;
				searchTex.as_image().expected_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				auto smaa_final_pass = graph.add_stage<post_process_stage>(post_process_meta{assets::graphic::shaders::comp::smaa::neighborhood_blending, {
					   {{0}, no_slot, 0},
					   {{1}, 0, no_slot},
					   {{2}, 1, no_slot},
				}});
				smaa_final_pass.meta.set_sampler_at_binding(1, assets::graphic::samplers::blit_sampler);
				smaa_final_pass.meta.set_sampler_at_binding(2, assets::graphic::samplers::blit_sampler);
				smaa_final_pass.pass.add_dep({world_merge.id(), 0, 0});
				smaa_final_pass.pass.add_dep({smaa_weight_pass.id(), 0, 1});
				// smaa_final_pass.pass.add_output({{0, true}});


			const auto ui_bloom = graph.add_stage<bloom_pass>(bloom_meta);
			ui_bloom.meta.set_sampler_at_binding(0, assets::graphic::samplers::blit_sampler);
			ui_bloom.meta.set_sampler_at_binding(1, assets::graphic::samplers::blit_sampler);
			ui_bloom.pass.add_input({{ui_light, 0}});

			final_merge.pass.add_input({
					{ui_base, 1},
					{ui_light, 2},
				});

			final_merge.pass.add_output({
					{0, false},
				});


			final_merge.pass.add_dep({
					pass_dependency{
						.id = ui_bloom.id(),
						.dst_idx = 3
					},
					{.id = smaa_final_pass.id(),}
					// {.id = world_merge.id(),}
				});


			{
				auto ui_clear = graph.add_stage<image_clear>(2);

				ui_clear.pass.add_exec_dep(&final_merge.pass);
				ui_clear.pass.add_in_out({
						{ui_base, 0},
						{ui_light, 1},
					});
			}

			{
				auto world_clear = graph.add_stage<image_clear>(2);

				world_clear.pass.add_exec_dep(&world_merge.pass);
				world_clear.pass.add_in_out({
						{world_blit_base, 0},
						{world_blit_light, 1},
					});
			}

			{
				auto world_clear = graph.add_stage<image_clear>(2);

				world_clear.pass.add_exec_dep(&oit.pass);
				world_clear.pass.add_in_out({
						explicit_resource_usage{world_draw_resolved_base, 0},
						explicit_resource_usage{world_draw_resolved_light, 1},
					});


				auto world_depth_clear =
					graph.add_stage<depth_stencil_image_clear>(std::initializer_list<depth_stencil_image_clear::clear_info>{{{1}}});

				world_depth_clear.pass.add_exec_dep(&ssao.pass);
				world_depth_clear.pass.add_exec_dep(&oit.pass);
				world_depth_clear.pass.add_in_out({{world_draw_resolved_depth, 0}});
			}

			{
				auto world_clear = graph.add_stage<image_clear>(2);

				world_clear.pass.add_in_out({
						explicit_resource_usage{world_draw_base, 0},
						explicit_resource_usage{world_draw_light, 1},
					});


				auto world_depth_clear =
					graph.add_stage<depth_stencil_image_clear>(std::initializer_list<depth_stencil_image_clear::clear_info>{{{1}}});

				world_depth_clear.pass.add_in_out({{world_draw_depth, 0}});
			}

			{

				VkClearColorValue color{.uint32 = {~0u}};
				auto oit_clear = graph.add_stage<image_clear>(
					std::initializer_list<image_clear::clear_info>{{color}});

				oit_clear.pass.add_exec_dep(&oit.pass);
				oit_clear.pass.add_in_out({{world_draw_oit_head, 0}});


				auto off = std::bit_cast<std::uint32_t>(&graphic::oit_statistic::size);
				auto oit_buf_clear = graph.add_stage<buffer_fill>(std::initializer_list<buffer_fill::clear_info>{
						{0, std::bit_cast<std::uint32_t>(&graphic::oit_statistic::size), sizeof(graphic::oit_statistic) - off},
					});

				oit_buf_clear.pass.add_exec_dep(&oit.pass);
				oit_buf_clear.pass.add_in_out({
						explicit_resource_usage{world_draw_oit_buffer_stat, 0},
					});
			}

			graph.init_after_pass_initialized();
			std::println("{}", graph.get_exec_seq() | std::views::transform([](const pass_data& pass){
				return pass.get_identity_name();
			}));

			world_bloom_ = &world_bloom.meta;
			ui_bloom_ = &ui_bloom.meta;
			ssao_pass_ = &ssao.meta;

		}

		[[nodiscard]] vk::image_handle get_output_image() const {
			return final_merge.pass.at_out(0).as_image().image;
		}

		void set_updator(std::invocable<post_process_graph&> auto&& fn){
			updater = std::forward<decltype(fn)>(fn);
		}

		void update_resources(){
			updater(*this);
			graph.update_external_resources();
			graph.resize();
			graph.reset_resources();
			graph.create_command();
		}

		auto get_main_command_buffer() const{
			return graph.get_main_command_buffer();
		}

		[[nodiscard]] bloom_pass& world_bloom() const noexcept{
			return *world_bloom_;
		}

		[[nodiscard]] bloom_pass& ui_bloom() const noexcept{
			return *ui_bloom_;
		}

		[[nodiscard]] ssao_pass& ssao_pass() const noexcept{
			return *ssao_pass_;
		}
	};
}
