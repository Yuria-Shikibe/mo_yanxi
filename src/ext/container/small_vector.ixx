module;

export module mo_yanxi.small_vector;


#include <gch/small_vector.hpp>
export namespace mo_yanxi{
	using namespace gch::concepts;
	using gch::small_vector;
	using gch::small_vector_iterator;
	using gch::default_buffer_size;
	using gch::default_buffer_size_v;
	using gch::erase;
	using gch::erase_if;
	using gch::begin;
	using gch::end;
	using gch::cbegin;
	using gch::cend;
	using gch::rbegin;
	using gch::rend;
	using gch::crbegin;
	using gch::crend;
}