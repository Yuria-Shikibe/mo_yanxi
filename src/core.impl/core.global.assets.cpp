module mo_yanxi.core.global.assets;

import mo_yanxi.graphic.draw;
import mo_yanxi.font.typesetting;
import mo_yanxi.vk.context;

void mo_yanxi::core::global::assets::init(void* vk_context_ptr){
	auto& context = *static_cast<vk::context*>(vk_context_ptr);

	atlas = graphic::image_atlas{context};
	font_manager.set_page(atlas.create_image_page("font"));


	{ //TODO using relative path
		// auto p = font_manager.page().register_named_region(
		// 	std::string_view{"white"}, graphic::path_load{
		// 		R"(D:\projects\mo_yanxi\prop\assets\texture\white.png)"
		// 	});
		// p.first.uv.shrink(64);
		// font_manager.page().mark_protected("white");

		// graphic::draw::white_region = p.first;

		auto& face_tele = font_manager.register_face("tele", R"(D:\projects\mo_yanxi\prop\assets\fonts\telegrama.otf)");
		auto& face_srch = font_manager.register_face("srch", R"(D:\projects\mo_yanxi\prop\assets\fonts\SourceHanSerifSC-SemiBold.otf)");
		auto& face_timesi = font_manager.register_face("timesi", R"(D:\projects\mo_yanxi\prop\assets\fonts\timesi.ttf)");
		auto& face_ui = font_manager.register_face("segui", R"(D:\projects\mo_yanxi\prop\assets\fonts\seguisym.ttf)");

		face_tele.fallback = &face_srch;
		face_srch.fallback = &face_ui;
		font::typesetting::default_font_manager = &font_manager;
		font::typesetting::default_font = &face_tele;
	}
}

void mo_yanxi::core::global::assets::dispose(){
	atlas = {};
	font_manager = {};
}
