module test;

import mo_yanxi.gui.infrastructure;
import mo_yanxi.gui.infrastructure.group;
import mo_yanxi.gui.global;

import mo_yanxi.react_flow;

import mo_yanxi.gui.elem.manual_table;
import mo_yanxi.gui.elem.sequence;
import mo_yanxi.gui.elem.scroll_pane;
import mo_yanxi.gui.elem.collapser;
import mo_yanxi.gui.elem.table;
import mo_yanxi.gui.elem.grid;
import mo_yanxi.gui.elem.menu;
import mo_yanxi.gui.elem.label;

void test::build_main_ui(gui::scene& scene, gui::loose_group& root, mo_yanxi::react_flow::node& console_input){
	auto e = scene.create<gui::scaling_stack>();
	e->set_fill_parent({true, true});
	auto& mroot = static_cast<gui::scaling_stack&>(root.insert(0, std::move(e)));

	auto& layoutnode = scene.request_independent_react_node<mo_yanxi::gui::label_layout_node>();
	layoutnode.connect_predecessor(console_input);

	auto hdl = mroot.create_back([&](mo_yanxi::gui::async_label& label){
		label.set_dependency(layoutnode);
	});

	hdl.cell().region_scale = {.0f, .0f, .4f, 1.f};
	hdl.cell().align = gui::align::pos::bottom_left;
	/*
	{
		auto hdl = mroot.emplace_back<mo_yanxi::gui::scroll_pane>(gui::layout::layout_policy::vert_major);
		hdl.cell().region_scale = {.0f, .0f, .4f, 1.f};
		hdl.cell().align = gui::align::pos::bottom_left;

		hdl->create([](mo_yanxi::gui::sequence& sequence){
			sequence.set_style();
			sequence.set_expand_policy(gui::layout::expand_policy::prefer);
			sequence.template_cell.set_external();
			sequence.template_cell.set_pad({6.f});

			for(int i = 0; i < 14; ++i){
				sequence.create_back([&](::mo_yanxi::gui::collapser& c){
					c.set_update_opacity_during_expand(true);
					c.set_expand_cond(gui::collapser_expand_cond::inbound);

					c.emplace_head<gui::elem>();
					c.set_head_size(50);
					c.create_content([](gui::table& e){
						e.set_tooltip_state({
							.layout_info = gui::tooltip::align_meta{
								.follow = gui::tooltip::anchor_type::owner,
								.attach_point_spawner = align::pos::top_left,
								.attach_point_tooltip = align::pos::top_right,
							},
						}, [](gui::table& tooltip){
							using namespace gui;
							struct dialog_creator : elem{
								[[nodiscard]] dialog_creator(gui::scene& scene, elem* parent)
								: elem(scene, parent){
									interactivity = interactivity_flag::enabled;
								}

								gui::events::op_afterwards on_click(const gui::events::click event, std::span<elem* const> aboves) override{
									if(event.key.on_release()){
										get_scene().create_overlay({
												.extent = {
													{layout::size_category::passive, .4f},
													{layout::size_category::scaling, 1.f}
												},
												.align = align::pos::center,
											}, [](table& e){
												e.end_line().emplace_back<elem>();
												e.end_line().emplace_back<elem>();
												e.end_line().emplace_back<elem>();
												e.end_line().emplace_back<elem>();
												e.end_line().emplace_back<elem>();
											});
									}
									return gui::events::op_afterwards::intercepted;
								}
							};
							tooltip.emplace_back<dialog_creator>().cell().set_size({160, 60});
						});
						e.set_entire_align(align::pos::top_left);
						for(int k = 0; k < 5; ++k){
							e.emplace_back<gui::elem>().cell().set_size({250, 60});
						}
						// e.interactivity = gui::interactivity_flag::enabled;
					});

					c.set_head_body_transpose(i & 1);

				});
			}
		});
	}
	*/

	{
		auto hdl = mroot.create_back([](mo_yanxi::gui::menu& menu){
			// menu.set_layout_policy(gui::layout::layout_policy::vert_major);
			menu.set_head_size(90);
			menu.get_head_template_cell().set_size(240).set_pad({4, 4});

			for(int i = 0; i < 4; ++i){
				auto hdl = menu.create_back([](mo_yanxi::gui::elem& e){

					e.interactivity = gui::interactivity_flag::enabled;
				}, [&](mo_yanxi::gui::sequence& e){
					e.set_has_smooth_pos_animation(true);
					e.set_expand_policy(gui::layout::expand_policy::passive);
					e.template_cell.set_pad({4, 4});
					for(int j = 0; j < i + 1; ++j){
						e.emplace_back<gui::elem>();
					}
				});
			}
		});
		hdl->set_expand_policy(gui::layout::expand_policy::passive);
		hdl.cell().region_scale = {.0f, .0f, .6f, 1.f};
		hdl.cell().align = gui::align::pos::bottom_right;
		//
		// hdl->create([](gui::table& table){
		// 	table.set_expand_policy(gui::layout::expand_policy::prefer);
		// 	table.set_entire_align(align::pos::center);
		// 	for(int i = 0; i < 4; ++i){
		// 		table.emplace_back<gui::elem>().cell().set_size({60, 120});
		// 		table.emplace_back<gui::elem>();
		// 		table.end_line();
		// 	}
		// 	table.emplace_back<gui::elem>().cell().set_height(40).set_width_passive(.85f).saturate = true;
		// 	//.align = align::pos::center;
		// 	// table.emplace_back<gui::elem>();
		// 	table.end_line();
		//
		// 	for(int i = 0; i < 4; ++i){
		// 		table.emplace_back<gui::elem>().cell().set_size({120, 120});
		// 		table.emplace_back<gui::elem>();
		// 		table.end_line();
		// 	}
		// });
	}
	/*{
		auto hdl = mroot.emplace_back<mo_yanxi::gui::scroll_pane>();
		hdl.cell().region_scale = {.0f, .0f, .4f, 1.f};
		hdl.cell().align = gui::align::pos::bottom_right;

		hdl->create([](gui::table& table){
			table.set_expand_policy(gui::layout::expand_policy::prefer);
			table.set_entire_align(align::pos::center);
			for(int i = 0; i < 4; ++i){
				table.emplace_back<gui::elem>().cell().set_size({60, 120});
				table.emplace_back<gui::elem>();
				table.end_line();
			}
			table.emplace_back<gui::elem>().cell().set_height(40).set_width_passive(.85f).saturate = true;
			//.align = align::pos::center;
			// table.emplace_back<gui::elem>();
			table.end_line();

			for(int i = 0; i < 4; ++i){
				table.emplace_back<gui::elem>().cell().set_size({120, 120});
				table.emplace_back<gui::elem>();
				table.end_line();
			}
		});

	}*/
	/*{
		auto hdl = mroot.emplace_back<mo_yanxi::gui::scroll_pane>();
		hdl.cell().region_scale = {.0f, .0f, .6f, 1.f};
		hdl.cell().align = gui::align::pos::bottom_right;
		hdl->set_layout_policy(gui::layout::layout_policy::vert_major);



		hdl->create([](mo_yanxi::gui::grid& table){
			table.set_expand_policy(gui::layout::expand_policy::prefer);
			table.emplace_back<gui::elem>().cell().extent = {
					{.type = gui::grid_extent_type::src_extent, .desc = {0, 2},},
					{.type = gui::grid_extent_type::src_extent, .desc = {0, 1},},
				};
			table.emplace_back<gui::elem>().cell().extent = {
					{.type = gui::grid_extent_type::src_extent, .desc = {1, 2},},
					{.type = gui::grid_extent_type::src_extent, .desc = {1, 1},},
				};
			table.emplace_back<gui::elem>().cell().extent = {
					{.type = gui::grid_extent_type::src_extent, .desc = {2, 2},},
					{.type = gui::grid_extent_type::src_extent, .desc = {2, 1},},
				};
			table.emplace_back<gui::elem>().cell().extent = {
					{.type = gui::grid_extent_type::src_extent, .desc = {0, 4},},
					{.type = gui::grid_extent_type::src_extent, .desc = {2, 2},},
				};
			table.emplace_back<gui::elem>().cell().extent = {
					{.type = gui::grid_extent_type::margin, .desc = {1, 1},},
					{.type = gui::grid_extent_type::src_extent, .desc = {5, 1},},
				};
			table.emplace_back<gui::elem>().cell().extent = {
					{.type = gui::grid_extent_type::margin, .desc = {4, 1},},
					{.type = gui::grid_extent_type::src_extent, .desc = {7, 1},},
				};
			table.emplace_back<gui::elem>().cell().extent = {
					{.type = gui::grid_extent_type::src_extent, .desc = {0, 5},},
					{.type = gui::grid_extent_type::margin, .desc = {0, 0},},
				};
		}, math::vector2<mo_yanxi::gui::grid_dim_spec>{
			mo_yanxi::gui::grid_uniformed_mastering{6, 300.f, {4, 4}},
			mo_yanxi::gui::grid_uniformed_passive{8, {4, 4}}
		});

	}*/


}

