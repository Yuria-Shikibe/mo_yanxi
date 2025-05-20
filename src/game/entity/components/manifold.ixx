module;

#include <gch/small_vector.hpp>
#include "../src/ext/enum_operator_gen.hpp"

export module mo_yanxi.game.ecs.component.manifold;

export import mo_yanxi.game.ecs.component.hitbox;
export import mo_yanxi.game.ecs.component.physical_property;
export import mo_yanxi.game.ecs.entity;

export import mo_yanxi.array_stack;

import std;

namespace mo_yanxi::game::ecs{
	export
	struct manifold;

	export enum struct collision_result{
		none = 0,
		sbj_passed = 1 << 0,
		obj_passed = 1 << 1,
		passed = sbj_passed | obj_passed
	};

	BITMASK_OPS(export, collision_result);

	export
	struct collision_object{
		entity_id id;
		manifold* manifold;
		mech_motion* motion;
		physical_rigid* rigid;

		constexpr friend bool operator==(const collision_object& lhs, const collision_object& rhs) noexcept{
			return lhs.id == rhs.id;
		}
	};

	export
	struct collision_possible{
		collision_object other;
		collision_data data;

		[[nodiscard]] entity_id get_other_id() const noexcept{
			return other.id;
		}
	};

	export
	struct intersection{
		math::vec2 pos;
		math::nor_vec2 normal;
		hitbox_collision_indices index;
	};

	export
	struct collision_confirmed{
		collision_object other{};
		math::vec2 correction{};
		intersection intersection{};

		[[nodiscard]] entity_id get_other_id() const noexcept{
			return other.id;
		}
	};

	export
	struct default_collider{
		static constexpr collision_result try_collide_to(
			const collision_object& sbj,
			const collision_object& obj,
			const intersection& intersection
		) noexcept {
			return collision_result::passed;
		}

		static constexpr bool collide_able_to(
			const collision_object& sbj,
			const collision_object& obj
		) noexcept {
			return true;
		}

		static entity_id get_group_identity(entity_id self, const manifold& manifold){
			return self;
		}
	};

	export
	struct projectile_collider : default_collider{
		collision_result try_collide_to(
			const collision_object& projectile,
			const collision_object& chamber_grid,
			const intersection& intersection
		) noexcept;

		static bool collide_able_to(
			const collision_object& sbj,
			const collision_object& obj
		) noexcept;

		static entity_id get_group_identity(entity_id self, const manifold& manifold);
	};

	using colliders = std::variant<
		default_collider,
		projectile_collider
	>;


	struct manifold{
		[[nodiscard]] manifold() = default;

		float thickness{0.05f};
		hitbox hitbox{};
		colliders collider{};
		bool no_backtrace_correction{false};


		bool is_under_correction{false};
		math::trans2 collision_vel_trans_sum{};
		math::vec2 collision_correction_vec{};

		gch::small_vector<entity_ref, 1> last_collided{};
		gch::small_vector<collision_possible, 1> possible_collision{};
		gch::small_vector<collision_confirmed, 1> confirmed_collision{};


		[[nodiscard]] bool collided() const noexcept{
			return !possible_collision.empty();
		}

		[[nodiscard]] entity_id get_hit_group_id(entity_id self) const noexcept{
			return std::visit<entity_id>([&, this] <std::derived_from<default_collider> T> (T& c){
				return c.get_group_identity(self, *this);
			}, collider);
		}

		static bool is_collideable_between(
			const collision_object& lhs,
			const collision_object& rhs){
			if(lhs.manifold->collider.index() == rhs.manifold->collider.index()){
				return std::visit([&] <std::derived_from<default_collider> T> (T& c){
					return c.collide_able_to(lhs, rhs);
				}, lhs.manifold->collider);
			}else{
				return std::visit([&] <std::derived_from<default_collider> T> (T& c){
					return c.collide_able_to(lhs, rhs);
				}, lhs.manifold->collider) && std::visit([&] <std::derived_from<default_collider> T> (T& c){
					return c.collide_able_to(rhs, lhs);
				}, rhs.manifold->collider);
			}
		}

		/**
		 * @brief
		 * @param sbj
		 * @param obj
		 * @param intersection
		 * @return
		 */
		static collision_result try_collide_to(
			const collision_object& sbj,
			const collision_object& obj,
			const intersection& intersection
		){
			return std::visit<collision_result>([&] <std::derived_from<default_collider> T> (T& c){
				return c.try_collide_to(sbj, obj, intersection);
			}, sbj.manifold->collider);
		}

		bool filter() noexcept{
			using gch::erase_if;
			using std::erase_if;

			auto backtraceIndex = hitbox.get_intersect_move_index();

			erase_if(possible_collision, [backtraceIndex](const collision_possible& c){
				return c.data.sbj_transition > backtraceIndex || c.data.obj_transition > c.other.manifold->hitbox.get_intersect_move_index();
			});

			return collided();
		}



		bool test_intersection_with(collision_object object){
			return hitbox.collide_with_exact(object.manifold->hitbox, [this, &object](const unsigned index, const unsigned indexObj) {
				return &possible_collision.emplace_back(object, collision_data{index, indexObj}).data;
			}, false);
		}

		void markLast(){
			last_collided.clear();
			for (const auto & data : confirmed_collision){
				last_collided.push_back(data.get_other_id());
			}

			confirmed_collision.clear();
		}

		void clear(){
			is_under_correction = false;
			last_collided.clear();
			possible_collision.clear();
			confirmed_collision.clear();
		}
	};

	static_assert(std::is_default_constructible_v<manifold>);
}