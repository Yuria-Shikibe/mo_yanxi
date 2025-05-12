module;

#include <gch/small_vector.hpp>

export module mo_yanxi.game.ecs.component.chamber;

export import :tile;
export import :grid;


namespace mo_yanxi::game::ecs{
	using namespace chamber;

	export
	template <>
	struct component_custom_behavior<chamber::building_data> : component_custom_behavior_base<chamber::building_data>{
		static void on_init(const chunk_meta& meta, value_type& comp){
			comp.tile_states.resize(comp.region().area());
			comp.grid()->local_grid.insert(meta.id());
		}

		static void on_terminate(const chunk_meta& meta, const value_type& comp){
			comp.grid()->local_grid.erase(meta.id());
		}
	};


	namespace chamber{
		math::trans2 building_data::get_trans() const noexcept{
			auto trs = grid_->get_transform();
			return {region().get_src().scl(tile_size_integral).as<float>() | grid_->get_transform(), trs.rot};
		}
	}
}
