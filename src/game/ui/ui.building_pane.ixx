

export module mo_yanxi.game.ui.building_pane;

export import mo_yanxi.ui.primitives;

import mo_yanxi.game.ecs.component.chamber;

import mo_yanxi.ui.elem.list;
import mo_yanxi.ui.elem.label;
import mo_yanxi.game.ui.chamber_ui_elem;
import mo_yanxi.game.ui.bars;

import std;

namespace mo_yanxi::game::ui{
	export using namespace mo_yanxi::ui;

	struct building_info_pane{

		static constexpr float size_hp_bar = 20;
		static constexpr float size_energy_bar = 20;
		static constexpr float size_reload_bar = 20;



		ecs::chamber::building_entity_ref entity{};

		energy_bar_drawer energy_bar{};
		stalled_hp_bar_drawer hp_bar{};

		std::optional<reload_bar_drawer> reload_bar{};

		[[nodiscard]] building_info_pane() = default;

		[[nodiscard]] explicit building_info_pane(const ecs::chamber::building_entity_ref& entity)
					: entity(entity){
		}

		void set_reload(float progress){
			if(!reload_bar){
				reload_bar.emplace();
			}

			reload_bar->current_reload_value = progress;
		}

		void set_state(){
			if(!entity)return;
			auto& data = entity.data();
			energy_bar.set_bar_state({
				.power_current = data.get_energy_dynamic_status().power,
				.power_total = data.energy_status.power,
				.power_minimal_expected = data.ideal_energy_acquisition.minimum_count,
				.power_expected = data.ideal_energy_acquisition.count,
				.power_assigned = data.valid_energy,
				.power_valid = static_cast<unsigned>(std::abs(data.get_max_usable_energy())),
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

		[[nodiscard]] float acquire_size(float major_size) const noexcept{
			auto sum = size_hp_bar;
			if(energy_bar)sum += size_energy_bar;
			if(reload_bar)sum += size_reload_bar;
			if(entity){
				sum += build_tile_status_drawer::get_required_extent({major_size, std::numeric_limits<float>::infinity()}, entity.data().region().extent()).y;
			}
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

			if(entity){
				build_tile_status_drawer::draw(math::frect{offset, region.get_end()}, opacity, renderer, entity.data());
			}
		}
	};

	struct bars_pane : ui::elem{
		building_info_pane info{};

		[[nodiscard]] bars_pane(scene* scene, group* group, ecs::chamber::building_entity_ref entity)
			: elem(scene, group), info(std::move(entity)){
			interactivity = interactivity::disabled;
		}

	protected:
		std::optional<math::vec2> pre_acquire_size_impl(optional_mastering_extent extent) override{
			return math::vec2{extent.potential_width(), info.acquire_size(extent.potential_width())};
		}

		void update(float delta_in_ticks) override{
			info.set_state();
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
			ecs::chamber::building_entity_ref e = {})
			: list(scene, parent),
			  entity(std::move(e)){
			interactivity = interactivity::enabled;


			auto name = emplace<ui::label>();
			name->set_fit();
			name->set_style();
			name->prop().boarder.set(4);
			name.cell().pad.set(8);
			name.cell().set_size(40);

			{
				auto p = emplace<bars_pane>(entity);
				p->set_style();
				p.cell().set_external();

				if(entity){
					auto& data = this->entity.data();
					if(auto meta = data.get_meta()){
						name->set_text(meta->get_meta_info().name);
					}
					p->info.set_state();
				}
			}


		}

		ecs::chamber::building_entity_ref entity{};


		[[nodiscard]] bars_pane& get_bar_pane() const noexcept{
			return this->at<bars_pane>(1);
		}

	private:
		void on_focus_changed(bool is_focused) override{
			elem::on_focus_changed(is_focused);

			if(is_focused){
				(void)insert_independent_draw({0});
			}else{
				(void)erase_independent_draw();
			}
		}

		void update(float delta_in_ticks) override{
			list::update(delta_in_ticks);
		}

		void draw_content(const rect clipSpace) const override;

		void draw_independent() const override;
	};
}