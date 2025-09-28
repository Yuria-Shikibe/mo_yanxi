module;

#include <vulkan/vulkan.h>

export module mo_yanxi.game.graphic.render_graph_spec;

export import mo_yanxi.graphic.render_graph.bloom;
export import mo_yanxi.graphic.render_graph.fill;
export import mo_yanxi.graphic.render_graph.ssao;

import mo_yanxi.vk.context;
import mo_yanxi.assets.graphic;
import std;

//TODO to clean up
import mo_yanxi.graphic.post_processor.oit_blender;

namespace mo_yanxi::game::fx{
	using namespace graphic::render_graph;
	using namespace graphic::render_graph::resource_desc;

	export
	struct post_process_graph{
		graphic::render_graph::post_process_graph graph;

		explicit_resource& ui_base{graph.add_explicit_resource(explicit_resource{external_image{}})};
		explicit_resource& ui_light{graph.add_explicit_resource(explicit_resource{external_image{}})};

		explicit_resource& world_base{graph.add_explicit_resource(explicit_resource{external_image{}})};
		explicit_resource& world_light{graph.add_explicit_resource(explicit_resource{external_image{}})};

		explicit_resource& world_depth{graph.add_explicit_resource(explicit_resource{external_image{}})};
		explicit_resource& world_draw_base{graph.add_explicit_resource(explicit_resource{external_image{}})};
		explicit_resource& world_draw_light{graph.add_explicit_resource(explicit_resource{external_image{}})};

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


		std::move_only_function<void(post_process_graph&)> updater;

	public:

		[[nodiscard]] post_process_graph(vk::context& context) : graph(context){
			final_merge.meta.sockets().at_out(0).get<image_requirement>().format = VK_FORMAT_R8G8B8A8_UNORM;
			final_merge.meta.sockets().at_out(0).get<image_requirement>().usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

			{
				const post_process_meta bloom_meta = []{
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
				}();


				auto ui_bloom = graph.add_stage<bloom_pass>(bloom_meta);
				ui_bloom.meta.set_sampler_at_binding(0, assets::graphic::samplers::blit_sampler);
				ui_bloom.meta.set_sampler_at_binding(1, assets::graphic::samplers::blit_sampler);
				ui_bloom.pass.add_input({{ui_light, 0}});

				auto world_merge = graph.add_stage<post_process_stage>(post_process_meta{
						assets::graphic::shaders::comp::world_merge, {
							{{0}, no_slot, 0},
							{{1}, 0, no_slot},
							{{2}, 1, no_slot},
							{{3}, 2, no_slot},
							{{4}, 3, no_slot},
						}
					});

				auto world_bloom = graph.add_stage<bloom_pass>(bloom_meta);
				world_bloom.meta.set_sampler_at_binding(0, assets::graphic::samplers::blit_sampler);
				world_bloom.meta.set_sampler_at_binding(1, assets::graphic::samplers::blit_sampler);

				auto ssao = graph.add_stage<ssao_pass>(post_process_meta{
						assets::graphic::shaders::comp::ssao, {
							{{0}, 0, no_slot},
							{{1}, no_slot, 0}
						}
					});
				ssao.meta.set_sampler_at_binding(0, assets::graphic::samplers::blit_sampler);
				ssao.meta.sockets().at_in<image_requirement>(0).aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;


				{
					auto ui_clear = graph.add_stage<image_clear>(2);

					ui_clear.pass.add_exec_dep(&final_merge.pass);
					ui_clear.pass.add_inout({
						                {ui_base, 0},
						                {ui_light, 1},
					                });
				}

				final_merge.pass.add_input({
					{ui_base, 1},
					{ui_light, 2},
				});

				final_merge.pass.add_output({
					{0, false},
				});

				ssao.pass.add_input({{world_depth, 0}});

				auto oit = graph.add_stage<post_process_stage>(post_process_meta{
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
					explicit_resource_usage{world_depth, 2},
					explicit_resource_usage{world_draw_base, 3},
					explicit_resource_usage{world_draw_light, 4},
					explicit_resource_usage{world_base, 5},
					explicit_resource_usage{world_light, 6},
				});
				world_bloom.pass.add_dep({&oit.pass, 1});

				world_merge.pass.add_dep({
						{&oit.pass, 0, 0},
						{&oit.pass, 1, 1},
						{&ssao.pass, 0, 3},
						{&world_bloom.pass, 0, 2}
					});


				auto anti_alias = graph.add_stage<post_process_stage>(post_process_meta{assets::graphic::shaders::comp::anti_aliasing, {
					{{1}, 0, no_slot},
					{{2}, no_slot, 0},
				}});
				anti_alias.meta.set_sampler_at_binding(1, assets::graphic::samplers::blit_sampler);

				anti_alias.pass.add_dep({
					.id = &world_merge.pass
				});

				final_merge.pass.add_dep({
						pass_dependency{
							.id = ui_bloom.id(),
							.dst_idx = 3
						},
						{.id = anti_alias.id(),}
						// {.id = &world_merge.pass,}
					});

				{
					auto world_clear = graph.add_stage<image_clear>(2);

					world_clear.pass.add_exec_dep(&world_merge.pass);
					world_clear.pass.add_inout({
						                {world_base, 0},
						                {world_light, 1},
					                });


					auto world_depth_clear =
						graph.add_stage<depth_stencil_image_clear>(std::initializer_list<depth_stencil_image_clear::clear_info>{{{1}}});

					world_depth_clear.pass.add_exec_dep(&ssao.pass);
					world_depth_clear.pass.add_exec_dep(&oit.pass);
					world_depth_clear.pass.add_inout({{world_depth, 0}});

				}


				{
					auto world_clear = graph.add_stage<image_clear>(2);

					world_clear.pass.add_exec_dep(&oit.pass);
					world_clear.pass.add_inout({
						                explicit_resource_usage{world_draw_base, 0},
						                explicit_resource_usage{world_draw_light, 1},
					                });
				}

				{

					VkClearColorValue color{.uint32 = {~0u}};
					auto oit_clear = graph.add_stage<image_clear>(
						std::initializer_list<image_clear::clear_info>{{color}});

					oit_clear.pass.add_exec_dep(&oit.pass);
					oit_clear.pass.add_inout({{world_draw_oit_head, 0}});


					auto off = std::bit_cast<std::uint32_t>(&graphic::oit_statistic::size);
					auto oit_buf_clear = graph.add_stage<buffer_fill>(std::initializer_list<buffer_fill::clear_info>{
							{0, std::bit_cast<std::uint32_t>(&graphic::oit_statistic::size), sizeof(graphic::oit_statistic) - off},
						});

					oit_buf_clear.pass.add_exec_dep(&oit.pass);
					oit_buf_clear.pass.add_inout({
						explicit_resource_usage{world_draw_oit_buffer_stat, 0},
					});
				}
			}

			graph.auto_create_buffer();
			graph.sort();
			graph.check_sockets_connection();
			graph.analysis_minimal_allocation();
			graph.post_init();
		}

		vk::image_handle get_output_image() const {
			return final_merge.pass.at_out(0).as_image().image;
		}

		void set_updator(std::invocable<post_process_graph&> auto&& fn){
			updater = std::forward<decltype(fn)>(fn);
		}

		void update_resources(){
			updater(*this);
			graph.update_external_resources();
			graph.resize();
			graph.create_command();
		}

		auto get_main_command_buffer() const{
			return graph.get_main_command_buffer();
		}
	};
}
