module;

#include "../src/srl/srl.game.hpp"

module mo_yanxi.game.io;


bool mo_yanxi::game::io::write_hitbox_meta(std::ostream& stream, const meta::hitbox& meta){
	const auto msg = mo_yanxi::io::loader<meta::hitbox>::pack(meta);
	return msg.SerializeToOstream(&stream);
}

bool mo_yanxi::game::io::load_hitbox_meta(std::istream& stream, meta::hitbox& meta){
	mo_yanxi::io::loader<meta::hitbox>::buffer_type buffer;
	return buffer.ParseFromIstream(&stream);
}
