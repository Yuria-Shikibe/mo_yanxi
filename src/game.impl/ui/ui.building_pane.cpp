module mo_yanxi.game.ui.building_pane;

import mo_yanxi.core.global.graphic;

import mo_yanxi.ui.elem.slider;

import mo_yanxi.ui.graphic;

namespace mo_yanxi::graphic::draw{
	template <
		typename Vtx, std::derived_from<uniformed_rect_uv> UV, typename Proj,
		std::ranges::input_range Rng = std::initializer_list<math::vec2>

	>
	void draw_line_seq(
		auto_batch_acquirer<Vtx, UV, Proj>& acq,
		const Rng& points,
		float stroke = 2,
		graphic::color color = colors::white //TODO support gradient?
	){
		stroke *= .5f;

		auto itr = std::ranges::begin(points);
		auto end = std::ranges::end(points);
		if(itr == end)return;
		math::vec2 last1 = *itr;
		++itr;
		if(itr == end)return;
		math::vec2 last2 = *itr;
		++itr;

		math::vec2 last_line_normal = (last2 - last1).rotate_rt_clockwise().normalize();
		math::vec2 last_extend_vector = last_line_normal * stroke;
		std::size_t acqire_per_draw = 1;
		if constexpr (std::ranges::sized_range<Rng>){
			auto rng = std::ranges::size(points);
			acqire_per_draw = math::min<std::size_t>(rng, 64);
		}

		while(itr != end){
			const math::vec2 cur = *itr;
			const auto current_line_normal = (cur - last2).rotate_rt_clockwise().normalize();
			const auto ang = current_line_normal.angle_between_rad(last_line_normal);
			const auto ext = (current_line_normal + last_line_normal) * (stroke / (math::cos(ang) + 1));

			draw::fill::quad(acq.get_reserved(acqire_per_draw),
				last1 - last_extend_vector,
				last1 + last_extend_vector,
				last2 + ext,
				last2 - ext,
				color
			);

			last_line_normal = current_line_normal;
			last_extend_vector = ext;
			last1 = last2;
			last2 = cur;
			++itr;
		}

		math::vec2 current_normal = (last2 - last1).rotate_rt_clockwise().set_length(stroke);
		draw::fill::quad(acq.get(),
			last1 - last_extend_vector,
			last1 + last_extend_vector,
			last2 + current_normal,
			last2 - current_normal,
			color
		);
	}
}

namespace mo_yanxi::game{
	struct build_energy_slider : mo_yanxi::ui::raw_slider{
		ui::slider2d_slot bar2{};

		ecs::chamber::building_entity_ref ref{};

		[[nodiscard]] build_energy_slider(ui::scene* scene, ui::group* group,
			const ecs::chamber::building_entity_ref& ref)
			: raw_slider(scene, group),
			  ref(ref){

			this->set_hori_only();

			if(ref){
				auto& data = ref.data();
				bar.set_progress_from_segments_x(data.ideal_energy_acquisition.maximum_count, data.energy_status.abs_power());
				bar2.set_progress_from_segments_x(data.ideal_energy_acquisition.minimum_count, data.energy_status.abs_power());
			}
		}

	protected:
		void check_apply_2(){
			if(bar2.apply()){
				if(ref){
					auto& data = ref.data();
					auto power = bar2.get_segments_progress<unsigned>().x;
					data.set_ideal_energy_acquisition(math::min(data.ideal_energy_acquisition.maximum_count, power), data.ideal_energy_acquisition.maximum_count);
				}
			}
		}

		void on_data_changed() override{
			if(ref){
				auto& data = ref.data();
				auto power = bar.get_segments_progress<unsigned>().x;
				auto min_power = bar2.get_segments_progress<unsigned>().x;

				data.set_ideal_energy_acquisition(math::min(power, min_power), power);
			}
		}


		void on_scroll(const ui::input_event::scroll event) override{
			if(event.mode == core::ctrl::mode::alt){
				move_bar(bar2, event);

				check_apply_2();
			}

			if(event.mode == core::ctrl::mode::none){
				move_bar(bar, event);

				check_apply();
			}
		}

		void on_drag(const ui::input_event::drag event) override{
			if(event.code.mode() == core::ctrl::mode::alt){
				bar2.move_progress(event.trans() * sensitivity / content_size());
			}

			if(event.code.mode() == core::ctrl::mode::none){
				bar.move_progress(event.trans() * sensitivity / content_size());
			}
		}

		ui::input_event::click_result on_click(const ui::input_event::click click_event) override{
			elem::on_click(click_event);

			const auto [key, action, mode] = click_event.unpack();

			switch(action){
			case core::ctrl::act::press :{
				if(mode == core::ctrl::mode::ctrl){
					const auto move = (click_event.pos - (bar.get_progress() * content_size() + content_src_pos())) / content_size();
					bar.move_progress(move * sensitivity.copy().sign_or_zero());
					check_apply();
				}
				if(mode == (core::ctrl::mode::ctrl | core::ctrl::mode::alt)){
					const auto move = (click_event.pos - (bar2.get_progress() * content_size() + content_src_pos())) / content_size();
					bar2.move_progress(move * sensitivity.copy().sign_or_zero());
					check_apply_2();
				}
				break;
			}

			case core::ctrl::act::release :{
				check_apply();
				check_apply_2();
			}

			default : break;
			}

			return ui::input_event::click_result::intercepted;
		}

		void draw_content(const ui::rect clipSpace) const override{
			elem::draw_content(clipSpace);

			using namespace graphic::colors;
			static constexpr auto c2 = CRIMSON.create_lerp(light_gray, .5f);
			static constexpr auto c1 = power.create_lerp(light_gray, .2f);

			draw_bar(bar2, c2.copy().mul_rgba(.75f), c2);

			draw_bar(bar, c1.copy().mul_rgba(.75f), c1);
		}

	};
}

void mo_yanxi::game::ui::building_energy_bar_setter::build(){

}

void mo_yanxi::game::ui::building_pane::build(){

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

	set_tooltip_state({
		.layout_info = tooltip_layout_info{
			.follow = tooltip_follow::owner,
			.align_owner = align::pos::top_right,
			.align_tooltip = align::pos::top_left,
		},
		.use_stagnate_time = false,
		.auto_release = true,
	}, [this](ui::list& list){
		list.prop().size.set_minimum_size({400, 0});
		list.template_cell.set_size(50);
		list.template_cell.pad.set(4);

		if(entity && entity.data().energy_status.is_consumer())list.emplace<build_energy_slider>(entity);
		list.emplace<ui::elem>();
		list.emplace<ui::elem>();
		list.emplace<ui::elem>();
	});
}

void mo_yanxi::game::ui::building_pane::draw_content(const rect clipSpace) const{
	list::draw_content(clipSpace);

}

void mo_yanxi::game::ui::building_pane::draw_independent() const{

	if(this->cursor_state.inbound && entity){
		auto& data = entity.data();
		math::frect tileLocalBound{data.get_bound().extent()};
		auto trs = data.get_trans();
		const std::array quad{
			core::global::graphic::world.camera.get_world_to_screen(tileLocalBound.vert_00() | trs, false),
			core::global::graphic::world.camera.get_world_to_screen(tileLocalBound.vert_10() | trs, false),
			core::global::graphic::world.camera.get_world_to_screen(tileLocalBound.vert_11() | trs, false),
			core::global::graphic::world.camera.get_world_to_screen(tileLocalBound.vert_01() | trs, false),
		};

		const auto dest = (quad[0] + quad[1] + quad[2] + quad[3]) / 4;

		auto acq = mo_yanxi::ui::get_draw_acquirer(get_renderer());
		using namespace graphic;

		//TODO line sequence drawer
		draw::line::quad(acq, quad, 2.f);

		auto bound = get_bound();

		const auto p0 = (bound.vert_11() + bound.vert_10()) / 2;
		math::vec2 p1;
		auto diff = dest - p0;
		if(diff.x > 0){
			auto absdiff = diff.copy().to_abs();

			if(absdiff.x > absdiff.y){
				auto reserve = absdiff.y;
				p1 = p0.copy().add_x(absdiff.x - reserve);
			}else{
				auto reserve = absdiff.x;
				p1 = dest.copy().add_y(-std::copysign(absdiff.y - reserve, diff.y));
			}

			draw::draw_line_seq(acq, {p0, p1, dest}, 4, colors::aqua);
		}

	}
}
