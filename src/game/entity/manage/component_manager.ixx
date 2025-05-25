module;

#include <plf_hive.h>
#include <gch/small_vector.hpp>
#include <cassert>
#include "../src/ext/adapted_attributes.hpp"

export module mo_yanxi.game.ecs.component.manage;

export import :entity;
export import :serializer;
export import mo_yanxi.heterogeneous.open_addr_hash;
export import mo_yanxi.strided_span;

import mo_yanxi.concepts;
import mo_yanxi.meta_programming;
import mo_yanxi.basic_util;

import std;



namespace mo_yanxi::game::ecs{
	export
	template <typename T>
	struct readonly_decay : std::type_identity<std::add_const_t<std::decay_t<T>>>{};

	export
	template <typename T>
	struct readonly_decay<const T&> : readonly_decay<T>{};

	export
	template <typename T>
	struct readonly_decay<const volatile T&> : readonly_decay<T>{};

	template <typename T>
	struct readonly_decay<T&> : std::type_identity<std::decay_t<T>>{};

	template <typename T>
	struct readonly_decay<volatile T&> : std::type_identity<std::decay_t<T>>{};

	export
	template <typename T>
	using readonly_const_decay_t = readonly_decay<T>::type;
}

