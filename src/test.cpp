module test;

import mo_yanxi.gui.infrastructure;
import mo_yanxi.gui.infrastructure.group;
import mo_yanxi.gui.global;

import mo_yanxi.gui.elem.manual_table;
import mo_yanxi.gui.elem.sequence;
import mo_yanxi.gui.elem.scroll_pane;
import mo_yanxi.gui.elem.collapser;
import mo_yanxi.gui.elem.table;

void test::build_main_ui(gui::scene& scene, gui::loose_group& root){
	auto e = scene.create<gui::manual_table>();
	e->set_fill_parent({true, true});
	auto& mroot = static_cast<gui::manual_table&>(root.insert(0, std::move(e)));

	{
		auto hdl = mroot.emplace_back<mo_yanxi::gui::scroll_pane>(gui::layout::layout_policy::vert_major);
		hdl.cell().region_scale = {.0f, .0f, .6f, 1.f};
		hdl.cell().align = gui::align::pos::bottom_left;

		hdl->create([](mo_yanxi::gui::sequence& sequence){
			sequence.set_style();
			sequence.set_expand_policy(gui::layout::expand_policy::prefer);
			sequence.template_cell.set_external();
			sequence.template_cell.set_pad({6.f});

			for(int i = 0; i < 14; ++i){
				sequence.create_back([](::mo_yanxi::gui::collapser& c){
					c.set_update_opacity_during_expand(true);
					c.set_expand_cond(gui::collapser_expand_cond::inbound);

					c.emplace_head<gui::elem>();
					c.set_head_size(50);
					c.create_content([](gui::table& e){
						e.set_entire_align(align::pos::top_left);
						for(int k = 0; k < 5; ++k){
							e.emplace_back<gui::elem>().cell().set_size({250, 60});
						}
						// e.interactivity = gui::interactivity_flag::enabled;
					});
				});
			}
		});
	}

	{
		auto hdl = mroot.emplace_back<mo_yanxi::gui::scroll_pane>();
		hdl.cell().region_scale = {.0f, .0f, .4f, 1.f};
		hdl.cell().align = gui::align::pos::bottom_right;

		hdl->create([](mo_yanxi::gui::table& table){
			table.set_expand_policy(gui::layout::expand_policy::prefer);
			table.set_entire_align(align::pos::center);
			for(int i = 0; i < 4; ++i){
				table.emplace_back<gui::elem>().cell().set_size({60, 120});
				table.emplace_back<gui::elem>();
				table.end_line();
			}
			table.emplace_back<gui::elem>().cell().set_height(40).set_width_passive(.85f).saturate = true;//.align = align::pos::center;
			// table.emplace_back<gui::elem>();
			table.end_line();

			for(int i = 0; i < 4; ++i){
				table.emplace_back<gui::elem>().cell().set_size({120, 120});
				table.emplace_back<gui::elem>();
				table.end_line();
			}
		});

	}


}
