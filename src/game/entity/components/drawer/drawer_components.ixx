module;

#include "../src/ext/adapted_attributes.hpp"

export module mo_yanxi.game.ecs.component.drawer;

export import mo_yanxi.math.trans2;
export import mo_yanxi.game.ecs.entity;
export import mo_yanxi.game.world.graphic;

import mo_yanxi.graphic.image_manage;
import mo_yanxi.graphic.draw;
import mo_yanxi.graphic.draw.func;

import std;

namespace mo_yanxi::game::ecs::drawer{
	export using draw_acquirer = graphic::draw::world_acquirer<>;

	using trans_t = math::transform2<float>;

	export
	struct part_transform : trans_t{
		float z_offset;

		using part_transform::operator|=;

		[[nodiscard]] FORCE_INLINE constexpr trans_t get_trans() const noexcept{
			return static_cast<trans_t>(*this);
		}

		FORCE_INLINE constexpr part_transform& operator|=(const part_transform parentRef) noexcept{
			trans_t::operator|=(static_cast<trans_t>(parentRef)); // NOLINT(*-slicing)
			z_offset += parentRef.z_offset;

			return *this;
		}

		[[nodiscard]] FORCE_INLINE  constexpr friend part_transform operator|(part_transform self, const part_transform parentRef) noexcept{
			return self |= parentRef;
		}

		FORCE_INLINE constexpr part_transform& operator+=(const part_transform parentRef) noexcept{
			trans_t::operator+=(static_cast<trans_t>(parentRef)); // NOLINT(*-slicing)
			z_offset += parentRef.z_offset;

			return *this;
		}

		FORCE_INLINE constexpr part_transform friend operator+( part_transform lhs, const part_transform& rhs) noexcept{
			return lhs += rhs;
		}

		[[nodiscard]] constexpr part_transform operator*(const float scl) const noexcept{
			part_transform state = *this;
			state.trans_t::operator*=(scl);
			state.z_offset *= scl;

			return state;
		}
	};


	export
	struct part_pos_transform{
		math::vec2 offset;
		float z_offset;

		constexpr part_pos_transform& operator|=(const part_transform parentRef) noexcept{
			offset |= parentRef.get_trans();
			z_offset += parentRef.z_offset;

			return *this;
		}

		[[nodiscard]] constexpr friend part_pos_transform operator|(part_pos_transform self, const part_transform parentRef) noexcept{
			return self |= parentRef;
		}

		constexpr part_pos_transform& operator|=(const trans_t rhs) noexcept{
			offset |= rhs;

			return *this;
		}

		[[nodiscard]] constexpr friend part_pos_transform operator|(part_pos_transform self, const trans_t parentRef) noexcept{
			return self |= parentRef;
		}

		FORCE_INLINE constexpr part_pos_transform& operator+=(const part_pos_transform lhs) noexcept{
			offset += lhs.offset;
			z_offset += lhs.z_offset;

			return *this;
		}

		FORCE_INLINE constexpr part_pos_transform friend operator+(part_pos_transform lhs, const part_pos_transform& rhs) noexcept{
			return lhs += rhs;
		}
	};


	export
	struct texture_region{
		graphic::cached_image_region image{};
		math::vec2 draw_extent{};

		graphic::color color_scl{graphic::colors::white};

		constexpr void set_region_and_size(const graphic::cached_image_region& region) noexcept{
			set_region(region);
			draw_extent = image->uv.get_region_size<float>();
		}

		constexpr void set_region_and_try_init_size(const graphic::cached_image_region& region) noexcept{
			set_region(region);
			if(draw_extent.is_zero())draw_extent = image->uv.get_region_size<float>();
		}

		constexpr void set_region(const graphic::cached_image_region& region) noexcept{
			image = region;
		}
	};


	export
	struct texture_drawer{
		part_transform transform{};
		texture_region region{};

		FORCE_INLINE void draw(const world::graphic_context& draw_ctx, part_transform current_trans) const{
			using namespace graphic;

			current_trans |= transform;

			draw_acquirer acquirer{draw_ctx.renderer().batch, region.image.get_cache()};
			acquirer.proj.depth = current_trans.z_offset;

			draw::fill::rect(acquirer.get(), static_cast<const math::trans2&>(current_trans), region.draw_extent, region.color_scl);
		}
	};

	export
	struct rect_drawer{
		math::vec2 extent{};
		graphic::color color_scl{graphic::colors::white};

		part_transform transform{};

		FORCE_INLINE void draw(const world::graphic_context& draw_ctx, part_transform current_trans) const{
			using namespace graphic;

			current_trans = transform | current_trans;

			draw_acquirer acquirer{draw_ctx.renderer().batch, draw::white_region};
			acquirer.proj.depth = current_trans.z_offset;

			draw::fill::rect(acquirer.get(), static_cast<const math::trans2&>(current_trans), extent, color_scl);
		}
	};

	template <typename T>
	concept generic_drawable = requires(const T& t, world::graphic_context& ctx, part_transform trans){
		t.draw(ctx, trans);
	};

	export
	struct local_drawer{
		using drawables =
		std::variant<
			std::monostate,
			texture_drawer,
			rect_drawer
		>;
		drawables drawer{};

		local_drawer& operator=(const drawables& drawables) noexcept{
			drawer = drawables;
			return *this;
		}

		local_drawer& operator=(drawables&& drawables) noexcept{
			drawer = std::move(drawables);
			return *this;
		}

		void draw(const world::graphic_context& draw_ctx, part_transform current_trans) const{
			std::visit([&]<typename T>(const T& d){
				if constexpr (generic_drawable<T>){
					d.draw(draw_ctx, current_trans);
				}
			}, drawer);
		}
	};

	export
	struct entity_drawer{
		float clipspace_margin{};
		std::optional<math::frect> clip{};

		virtual ~entity_drawer() = default;

		virtual void draw(const world::graphic_context& draw_ctx) const{

		}

		virtual void post_effect(const world::graphic_context& draw_ctx) const{

		}
	};
}
