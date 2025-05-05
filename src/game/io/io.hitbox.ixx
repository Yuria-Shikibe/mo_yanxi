//
// Created by Matrix on 2025/5/3.
//

export module mo_yanxi.game.io:hitbox;

export import mo_yanxi.game.ecs.component.hitbox;

import std;

namespace mo_yanxi::game::io{
	bool write_hitbox_meta(std::ostream& stream, const hitbox_meta& meta);
	bool load_hitbox_meta(std::istream& stream, hitbox_meta& meta);
}
