module;

#include "../src/srl/srl.game.hpp"

module mo_yanxi.game.io;


bool mo_yanxi::game::io::write_hitbox_meta(std::ostream& stream, const hitbox_meta& meta){
	const auto msg = mo_yanxi::io::loader<hitbox_meta>::pack(meta);
	return msg.SerializeToOstream(&stream);
}

bool mo_yanxi::game::io::load_hitbox_meta(std::istream& stream, hitbox_meta& meta){
	mo_yanxi::io::loader<hitbox_meta>::buffer_type buffer;
	return buffer.ParseFromIstream(&stream);
}
