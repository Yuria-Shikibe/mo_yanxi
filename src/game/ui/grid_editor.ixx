module;

#include <cassert>

export module mo_yanxi.game.ui.grid_editor;


import mo_yanxi.ui.basic;
import mo_yanxi.ui.table;
import mo_yanxi.ui.elem.button;
import mo_yanxi.ui.manual_table;
import mo_yanxi.ui.elem.text_elem;
import mo_yanxi.ui.creation.file_selector;


import std;

namespace mo_yanxi::game{
	namespace ui{
		export using namespace mo_yanxi::ui;

		struct grid_editor : table{
			[[nodiscard]] grid_editor(scene* scene, group* group)
				: table(scene, group){
			}
		};
	}
}