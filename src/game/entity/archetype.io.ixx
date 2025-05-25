module;

#include <google/protobuf/message_lite.h>

export module mo_yanxi.game.ecs.component.io;

export import mo_yanxi.game.ecs.component.manage;

import mo_yanxi.game.ecs.entitiy_decleration;
import std;

#define case_of_type(type) case mo_yanxi::game::ecs::archetype_serialize_info<type>::identity.index : \
	return std::make_unique<mo_yanxi::game::ecs::archetype<type>::serializer>()


namespace mo_yanxi::game::ecs{


	component_chunk_offset write_message(std::ostream& stream, const google::protobuf::MessageLite& message){
		const std::uint64_t sz = message.ByteSizeLong();
		stream.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
		message.SerializeToOstream(&stream);
		return static_cast<component_chunk_offset>(sz);
	}

	void read_message(std::istream& stream, component_chunk_offset size, google::protobuf::MessageLite& message){
		std::vector<char> buf(size);
		stream.read(buf.data(), static_cast<std::streamsize>(size));
		std::ispanstream buffer_stream{buf};

		message.ParseFromIstream(&buffer_stream);
	}

	export
	template <>
	struct archetype_serialize_info<decl::chamber_entity_desc> : archetype_serialize_info_base<decl::chamber_entity_desc, archetype_serialize_identity{
		1
	}>{
		static chunk_serialize_handle write(std::ostream& stream, const dump_chunk& chunk);

		static srl_state read(std::istream& stream, component_chunk_offset off, dump_chunk& chunk) {
			return srl_state::failed;
		}
	};

	//
	// chunk_serialize_handle archetype_serialize_info<decl::chamber_entity_desc>::write(std::ostream& stream, const dump_chunk& chunk) {
	//
	// }

	export
	std::unique_ptr<archetype_serializer> get_archetype_serializer(const archetype_serialize_identity& identity){
		switch(identity.index){
			// case 0: return nullptr;
			case_of_type(decl::chamber_entity_desc);
			default: throw ecs::bad_archetype_serialize{std::format("unknown archetype serialization index: {}", identity.index)};
		}
	}

}
