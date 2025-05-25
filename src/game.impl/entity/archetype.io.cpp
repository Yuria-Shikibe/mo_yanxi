module mo_yanxi.game.ecs.component.io;

namespace mo_yanxi::game::ecs{

	chunk_serialize_handle archetype_serialize_info<decl::chamber_entity_desc>::write(std::ostream& stream, const dump_chunk& chunk) {
		co_yield 0;
	}

}
