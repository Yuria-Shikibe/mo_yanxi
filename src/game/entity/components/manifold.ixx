module;

#include <gch/small_vector.hpp>

export module mo_yanxi.game.ecs.component.manifold;

export import mo_yanxi.game.ecs.component.hitbox;
export import mo_yanxi.game.ecs.component.physical_property;
export import mo_yanxi.game.ecs.entity;

export import mo_yanxi.array_stack;

import std;

namespace mo_yanxi::game::ecs{

	export
	struct manifold{

		struct collision_object{
			component<manifold>* manifold;
			component<mech_motion>* motion;
			component<physical_rigid>* rigid;

			[[nodiscard]] constexpr entity_id id() const noexcept{
				return manifold->id();
			}

			constexpr friend bool operator==(const collision_object& lhs, const collision_object& rhs) noexcept = default;
		};

		struct collision_possible{
			collision_object other;
			collision_data data;

			[[nodiscard]] entity_id get_other_id() const noexcept{
				return other.id();
			}
		};

		struct intersection{
			math::vec2 pos;
			math::nor_vec2 normal;
			hitbox_collision_indices index;
		};

		struct collision_confirmed{
			collision_object other{};
			math::vec2 correction{};
			intersection intersection{};

			[[nodiscard]] entity_id get_other_id() const noexcept{
				return other.id();
			}
		};


		[[nodiscard]] manifold() = default;

		float thickness{0.05f};
		hitbox hitbox{};
		bool no_backtrace_correction{false};

		math::trans2 collision_trans_sum{};
		math::vec2 collision_correction_vec{};
		bool is_under_correction{false};
		gch::small_vector<entity_ref, 2> last_collided{};
		gch::small_vector<collision_possible, 2> possible_collision{};
		gch::small_vector<collision_confirmed, 2> confirmed_collision{};


		[[nodiscard]] bool collided() const noexcept{
			return !possible_collision.empty();
		}

		bool filter() noexcept{
			using gch::erase_if;
			using std::erase_if;

			auto backtraceIndex = hitbox.get_intersect_move_index();

			erase_if(possible_collision, [backtraceIndex](const collision_possible& c){
				return c.data.sbj_transition > backtraceIndex || c.data.obj_transition > c.other.manifold->val().hitbox.get_intersect_move_index();
			});

			return collided();
		}



		bool test_intersection_with(collision_object object){
			return hitbox.collide_with_exact(object.manifold->val().hitbox, [this, &object](const unsigned index, const unsigned indexObj) {
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