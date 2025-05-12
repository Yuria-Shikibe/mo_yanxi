#pragma once


#include "srl.hpp"
#include <srl/hitbox.pb.h>

import std;
import mo_yanxi.game.ecs.component.hitbox;

namespace mo_yanxi::io{
	template <>
	struct loader_impl<game::meta::hitbox::comp> : loader_base<pb::game::hitbox_comp, game::meta::hitbox::comp>{
		static void store(buffer_type& buf, const value_type& data){
			io::store(buf.mutable_box(), data.box);
			io::store(buf.mutable_trans(), data.trans);
		}

		static void load(const buffer_type& buf, value_type& data){
			io::load(buf.box(), data.box);
			io::load(buf.trans(), data.trans);
		}
	};

	template <>
	struct loader_impl<game::meta::hitbox> : loader_base<pb::game::hitbox_meta, game::meta::hitbox>{
		static void store(buffer_type& buf, const value_type& data){
			auto transformer = data | std::views::transform([](const value_type::comp& meta){
				return io::pack(meta);
			});

			buf.mutable_comps()->Assign(transformer.begin(), transformer.end());
		}

		static void load(const buffer_type& buf, value_type& data){
			data.components.clear();
			data.components.reserve(buf.comps_size());
			for (const auto & comp : buf.comps()){
				data.components.push_back(io::extract<value_type::comp>(comp));
			}
		}
	};
}