module mo_yanxi.game.ui.building_pane;

import mo_yanxi.core.global.graphic;
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
