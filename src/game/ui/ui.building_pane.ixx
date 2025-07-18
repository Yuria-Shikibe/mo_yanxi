//
// Created by Matrix on 2025/7/18.
//

export module mo_yanxi.game.ui.building_pane;

export import mo_yanxi.ui.primitives;

import mo_yanxi.ui.primitives;
import mo_yanxi.ui.elem.list;

import mo_yanxi.game.ui.bars;

import std;
namespace mo_yanxi::game::ui{
	export using namespace mo_yanxi::ui;

	struct building_info_pane{
		static constexpr float size_hp_bar = 30;
		static constexpr float size_energy_bar = 30;
		static constexpr float size_reload_bar = 30;

		energy_bar_drawer energy_bar;
		stalled_hp_bar_drawer hp_bar;

		std::optional<reload_bar_drawer> reload_bar;

		void set_reload(float progress){
			if(!reload_bar){
				reload_bar.emplace();
			}

			reload_bar->current_reload_value = progress;
		}

		void set_state(const ecs::chamber::building_data& data){
			energy_bar.set_bar_state({
				.power_current = data.get_energy_dynamic_status().power,
				.power_total = data.energy_status.power,
				.power_assigned = data.assigned_energy,
				.charge = data.get_energy_dynamic_status().charge,
				.charge_duration = data.energy_status.charge_duration
			});

			hp_bar.set_value(data.hit_point.factor());
			hp_bar.valid_range = data.hit_point.capability_range / data.hit_point.max;

			if(reload_bar){
				reload_bar->current_target_efficiency = data.get_efficiency();
			}
		}

		void update(float delta_in_tick){
			hp_bar.update(delta_in_tick);
			if(energy_bar)energy_bar.update(delta_in_tick);
			if(reload_bar)reload_bar->update(delta_in_tick);
		}

		float acquire_size(float major_size){
			auto sum = size_hp_bar;
			if(energy_bar)sum += size_energy_bar;
			if(reload_bar)sum += size_reload_bar;
			return sum;
		}

		void draw_content(const rect region, float opacity, graphic::renderer_ui_ref renderer) const{
			auto offset = region.src;

			hp_bar.draw(math::frect{tags::from_extent, offset, {region.width(), size_hp_bar}}, opacity, renderer);
			offset.add_y(size_hp_bar);

			if(energy_bar){
				energy_bar.draw(math::frect{tags::from_extent, offset, {region.width(), size_energy_bar}}, opacity, renderer);
				offset.add_y(size_energy_bar);
			}

			if(reload_bar){
				reload_bar->draw(math::frect{tags::from_extent, offset, {region.width(), size_reload_bar}}, opacity, renderer);
				offset.add_y(size_reload_bar);
			}
		}
	};

	struct bars_pane : ui::elem{
		building_info_pane info{};

		[[nodiscard]] bars_pane(scene* scene, group* group)
			: elem(scene, group){
		}

	protected:
		std::optional<math::vec2> pre_acquire_size_impl(optional_mastering_extent extent) override{
			return math::vec2{extent.potential_width(), info.acquire_size(extent.potential_width())};
		}

		void update(float delta_in_ticks) override{
			info.update(delta_in_ticks);
		}

		void draw_content(const rect clipSpace) const override{
			elem::draw_content(clipSpace);
			info.draw_content(get_content_bound(), gprop().get_opacity(), get_renderer());
		}
	};

	export
	struct building_pane final : ui::list{
		[[nodiscard]] building_pane(
			scene* scene,
			group* parent,
			ecs::chamber::building_entity_ref entity = {})
			: list(scene, parent),
			  entity(std::move(entity)){

			auto p = emplace<bars_pane>();
			p->info.set_state(this->entity.data());
			p->set_style();
			p.cell().set_external();
		}

		ecs::chamber::building_entity_ref entity{};


		[[nodiscard]] bars_pane& get_bar_pane() const noexcept{
			return this->at<bars_pane>(0);
		}

	private:
		void update(float delta_in_ticks) override{
			if(entity){
				auto& bars = get_bar_pane();
				bars.info.set_state(entity.data());
			}

			list::update(delta_in_ticks);
		}
	};
}