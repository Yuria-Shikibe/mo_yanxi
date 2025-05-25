//
// Created by Matrix on 2025/5/21.
//

export module mo_yanxi.game.ui.chamber_ui_elem;

export import mo_yanxi.game.ecs.component.chamber;
export import mo_yanxi.ui.table;
export import mo_yanxi.game.ui.bars;

import std;

namespace mo_yanxi::game::ui{
	export using namespace mo_yanxi::ui;

	struct build_tile_status_elem : ui::elem{
		static constexpr float chunk_max_draw_size = 64;
		ecs::chamber::building_ref building_data{};

	private:
		template <typename T = int>
		static float get_unit_size(math::vec2 self_extent, math::vector2<T> extent) noexcept{
			self_extent /= extent.template as<float>();
			auto min = math::min(self_extent.x, self_extent.y);
			return std::min(chunk_max_draw_size, min);
		}
	public:
		bool is_transposed() const noexcept{
			return building_data.data().region().width() < building_data.data().region().height();
		}

		[[nodiscard]] build_tile_status_elem(scene* scene, group* group)
			: elem(scene, group, "tile_status"){
		}

		std::optional<math::vec2> pre_acquire_size_impl(stated_extent extent) override{
			auto size = clip_boarder_from(extent);

			auto region = building_data.data().region().size();
			if(region.x < region.y){
				region.swap_xy();
			}

			if(size.fully_dependent()){
				return region.scl(chunk_max_draw_size).as<float>();
			}else{
				math::vec2 sz;
				if(size.width.mastering()){
					sz = {size.width, size.width * region.as<float>().slope()};
				}else{
					sz = {size.height * region.as<float>().slope_inv(), size.height};
				}

				auto usz = region.as<float>() * get_unit_size(sz, region);

				if(size.width.mastering()){
					return math::vec2{size.width, usz.y} + prop().boarder.get_size();
				}else{
					return math::vec2{usz.x, size.height} + prop().boarder.get_size();
				}
			}
		}

		void draw_content(const rect clipSpace) const override;
	};
	export
	struct chamber_ui_elem : ui::table{
	private:
		ecs::entity_ref chamber_entity{};
		stalled_bar* hitpoint_bar{};
		build_tile_status_elem* status_{};

		void build();

	public:
		[[nodiscard]] chamber_ui_elem(scene* scene, group* group, ecs::entity_ref chamber_entity)
			: table(scene, group), chamber_entity{std::move(chamber_entity)}{
			// set_style();
			build();
		}

		void update(float delta_in_ticks) override;
	};


}
