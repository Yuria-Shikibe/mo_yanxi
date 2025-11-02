module test;

import mo_yanxi.gui.infrastructure;
import mo_yanxi.gui.infrastructure.group;
import mo_yanxi.gui.global;

import mo_yanxi.gui.elem.manual_table;
import mo_yanxi.gui.elem.sequence;
import mo_yanxi.gui.elem.scroll_pane;

void test::build_main_ui(gui::scene& scene, gui::loose_group& root){
	auto e = scene.create<gui::manual_table>();
	e->set_fill_parent({true, true});
	auto& mroot = static_cast<gui::manual_table&>(root.insert(0, std::move(e)));

	{
		auto hdl = mroot.emplace_back<mo_yanxi::gui::scroll_pane>();
		hdl.cell().region_scale = {.0f, .0f, .6f, 1.f};
		hdl.cell().align = gui::align::pos::bottom_left;

		hdl->create([](mo_yanxi::gui::seq_list& sequence){
			sequence.set_scaling({1.f, .5f});
			sequence.set_expand_policy(gui::layout::expand_policy::prefer);
			sequence.template_cell.set_size(120);
			sequence.template_cell.set_pad({6.f});

			for(int i = 0; i < 14; ++i){
				sequence.emplace_back<gui::elem>()->interactivity = gui::interactivity_flag::enabled;
			}
		});

	}
}
