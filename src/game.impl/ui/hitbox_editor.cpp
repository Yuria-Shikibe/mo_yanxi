module;

#include "../src/srl/srl.game.hpp"

module mo_yanxi.game.ui.hitbox_editor;

import mo_yanxi.core.global.assets;

void mo_yanxi::game::ui::hit_box_editor::write_to(const std::filesystem::path& path){
	auto metas = viewport->channel_hitbox.export_to_meta();
	std::ofstream stream(path, std::ios::out | std::ios::binary);
	io::loader<meta::hitbox_transed>::serialize_to(stream, metas);
}

void mo_yanxi::game::ui::hit_box_editor::load_from(const std::filesystem::path& path){
	meta::hitbox_transed meta{};
	std::ifstream stream(path, std::ios::in | std::ios::binary);
	io::loader<meta::hitbox_transed>::parse_from(stream, meta);
	viewport->channel_hitbox.clear();
	viewport->channel_hitbox.add_comp(meta.components | std::views::transform([](const meta::hitbox::comp& a){
		return box_wrapper{{a}};
	}), false);
	viewport->channel_hitbox.origin_trans = meta.trans;
}


void mo_yanxi::game::ui::hit_box_editor::set_image_ref(const std::filesystem::path& path){
	auto region = core::global::assets::atlas[graphic::image_page::name_temp].async_allocate(
		{graphic::path_load{path.string()}}
	);

	viewport->channel_reference_image.set_region(std::move(region));
}