module mo_yanxi.game.ecs.component.command;

import mo_yanxi.game.ecs.system.motion_system;
import mo_yanxi.utility;


//TODO make it return {} by default?
std::optional<mo_yanxi::math::trans2> mo_yanxi::game::ecs::reference_target::get_target_trans() const noexcept{
	using ret = std::optional<math::trans2>;
	return std::visit(overload{
		[](std::monostate){return ret{};},
		[](math::trans2 t){return ret{t};},
		[](const entity_ref& ref){
			if(ref.is_expired())return ret{};
			if(auto t = ref->try_get<mech_motion>()){
				return ret{t->trans};
			}
			return ret{};
		}
	}, target);
}

std::optional<mo_yanxi::math::trans2> mo_yanxi::game::ecs::route::get_next_local() const noexcept{
	using ret = std::optional<math::trans2>;
	return std::visit(overload{
		[](std::monostate){return ret{};},
		[](const path_seq& path){return ret{path.next()};},
		// [](const orbit& obt){
		// 	return path.,next();
		// },
		[](const formation& f){return ret{f.trans};}
	}, route_);
}

mo_yanxi::game::march_state mo_yanxi::game::ecs::route::update(math::trans2 current_local_trs, const arth_type dst_margin, const arth_type ang_margin) noexcept{
	return std::visit(overload_def_noop{
		std::in_place_type<march_state>,
		[](std::monostate){
			return march_state::no_dest;
		},
		[&](path_seq& path){
			return path.check_arrive_and_update(current_local_trs, dst_margin, ang_margin);
		},
		[&](const formation& f){
			return current_local_trs.within(f.trans, dst_margin, ang_margin) ? march_state::reached : march_state::underway;
		}
	}, route_);
}

mo_yanxi::game::march_state mo_yanxi::game::ecs::move_command::update(math::trans2 current, arth_type dst_margin,
                                                                           arth_type ang_margin) noexcept{
	const auto target_abs = reference.get_target_trans().value_or({});
	const auto current_local = target_abs.apply_inv_to(current);
	auto rst = route.update(current_local, dst_margin, ang_margin);
	if(rst == march_state::finished){
		route = std::monostate{};
	}
	return rst;
}

std::optional<mo_yanxi::math::trans2> mo_yanxi::game::ecs::move_command::get_next(math::trans2 current, arth_type dst_margin) const noexcept{
	const auto target_abs = reference.get_target_trans();
	if(auto p = route.get_if<path_seq>(); p && dst_margin > std::numeric_limits<arth_type>::epsilon()){
		auto curIdx = p->current_index();
		if(curIdx == p->size() || p->size() < 2){

		}else{
			auto next = p->next();
			auto next_next = (*p)[curIdx];
			return math::lerp(next_next, next.value(), math::min<arth_type>(current.vec.dst(next->vec) / dst_margin, 1));
		}
	}

	auto next = route.get_next_local();
	return next.transform([&, this](math::trans2 trs){
		trs = target_abs.value_or({}).apply_inv_to(trs);

		if(const auto t = override_face_target.get_target_trans()){
			trs.rot = current.vec.angle_to_rad(t->vec);
		}
		return trs;
	});
}
