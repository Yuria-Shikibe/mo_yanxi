module mo_yanxi.core.global.ui;


import mo_yanxi.ui.pre_decl;
import mo_yanxi.ui.root;

void mo_yanxi::core::global::ui::init(){
	root = new mo_yanxi::ui::root{};
}

void mo_yanxi::core::global::ui::dispose(){
	delete root;
}
