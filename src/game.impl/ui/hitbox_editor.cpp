module;



extern "C++"{
#include "../src/srl/srl.game.hpp"
#include <gch/small_vector.hpp>
}

module mo_yanxi.game.ui.hitbox_editor;

void mo_yanxi::game::ui::hit_box_editor::write_to(const std::filesystem::path& path) const{
	auto metas = viewport->channel_hitbox.export_to_meta();
	std::ofstream stream(path, std::ios::out | std::ios::binary);
	io::loader<hitbox_meta>::serialize_to(stream, metas);
}

void mo_yanxi::game::ui::hit_box_editor::load_from(const std::filesystem::path& path) const{
	hitbox_meta meta{};
	std::ifstream stream(path, std::ios::in | std::ios::binary);
	io::loader<hitbox_meta>::parse_from(stream, meta);
	viewport->channel_hitbox.clear();
	viewport->channel_hitbox.add_comp(meta.components | std::views::transform([](const hitbox_meta::meta& a){
		return box_wrapper{{a}};
	}), false);
}
