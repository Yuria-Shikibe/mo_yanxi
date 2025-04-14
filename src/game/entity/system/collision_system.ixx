module;

#include <gch/small_vector.hpp>
#include "../src/ext/adapted_attributes.hpp"

export module mo_yanxi.game.ecs.system.collision;

export import mo_yanxi.game.ecs.component_manager;

export import mo_yanxi.game.ecs.component.manifold;
export import mo_yanxi.game.ecs.component.physical_property;
export import mo_yanxi.game.ecs.quad_tree;
export import mo_yanxi.shared_stack;
export import mo_yanxi.math.intersection;

import std;

namespace mo_yanxi::game::ecs{

	export
	template <>
	struct quad_tree_trait_adaptor<manifold::collision_object, float> : quad_tree_adaptor_base<manifold::collision_object, float>{
		[[nodiscard]] static rect_type get_bound(const_reference self) noexcept{
			return self.manifold->val().hitbox.max_wrap_bound();
		}

		[[nodiscard]] static bool intersect_with(const_reference self, const_reference other){
			return self.manifold->val().hitbox.collide_with_exact(other.manifold->val().hitbox);
		}

		[[nodiscard]] static bool contains(const_reference self, vector_type::const_pass_t point){
			return self.manifold->val().hitbox.contains(point);
		}

		[[nodiscard]] static bool equals(const_reference self, const_reference other) noexcept{
			return self.manifold == other.manifold;
		}
	};

	export
	struct collision_system{
	private:
		std::vector<manifold*> pre_passed_manifolds{};

		quad_tree<manifold::collision_object> tree{{{0, 0}, 50000}};


		FORCE_INLINE static void rigidCollideWith(
			const manifold::collision_object& sbj,
			const manifold::collision_object& obj,
			const manifold::intersection& intersection,
			const float energyScale) noexcept{
			assert(sbj != obj);

			// if(object->isOverrideCollisionTo(this)) {
			// 	const_cast<Game::RealityEntity*>(object)->overrideCollisionTo(this, data.intersection);
			// 	return;
			// }
			// if(std::ranges::contains(collisionContext.lastCollided, &object))return;

			using math::vec2;

			//Origin Point To Hit Point
			const vec2 dst_to_sbj = intersection.pos - sbj.motion->val().pos();
			const vec2 dst_to_obj = intersection.pos - obj.motion->val().pos();

			const vec2 sbj_vel = sbj.motion->val().vel_at(dst_to_sbj);
			const vec2 obj_vel = obj.motion->val().vel_at(dst_to_obj);

			const vec2 rel_vel = (obj_vel - sbj_vel) * energyScale;

			const vec2 hit_normal = intersection.normal;

			const auto sbj_mass = sbj.rigid->val().inertial_mass;
			const auto obj_mass = obj.rigid->val().inertial_mass;

			const auto scaled_mass = 1 / sbj_mass + 1 / obj_mass;
			const auto sbj_rotational_inertia = sbj.manifold->val().hitbox.getRotationalInertia(sbj_mass, sbj.rigid->val().rotational_inertia_scale);
			const auto obj_rotational_inertia = obj.manifold->val().hitbox.getRotationalInertia(obj_mass, obj.rigid->val().rotational_inertia_scale);

			const vec2 vert_hit_vel{rel_vel.copy().project_scaled(hit_normal)};
			const vec2 hori_hit_vel{(rel_vel - vert_hit_vel)};

			const vec2 hit_tangent = hori_hit_vel.equals({})
				                                 ? hit_normal.copy().rotate_rt_clockwise()
				                                 : hori_hit_vel.copy().normalize();

			const vec2 impulse_normal = vert_hit_vel * (1 +
				math::min(sbj.rigid->val().restitution, obj.rigid->val().restitution)) /
				(scaled_mass +
					math::sqr(dst_to_obj.cross(hit_normal)) / obj_rotational_inertia +
					math::sqr(dst_to_sbj.cross(hit_normal)) / sbj_rotational_inertia
				);

			const vec2 impulse_tangent = (hori_hit_vel * hori_hit_vel.length() /
						(scaled_mass +
							math::sqr(dst_to_obj.cross(hit_tangent)) / obj_rotational_inertia +
							math::sqr(dst_to_sbj.cross(hit_tangent)) / sbj_rotational_inertia))
				.limit_max_length(std::sqrt(sbj.rigid->val().friction_coefficient * obj.rigid->val().friction_coefficient) * impulse_normal.length());

			const vec2 impulse = impulse_normal + impulse_tangent;

			sbj.manifold->val().collision_trans_sum.vec += impulse / sbj.rigid->val().inertial_mass;
			sbj.manifold->val().collision_trans_sum.rot += dst_to_sbj.cross(impulse) / sbj_rotational_inertia * math::rad_to_deg;

			// vec2 correctionVec;

			// collision correction, should be collision quiet
			if(std::ranges::contains(sbj.manifold->val().last_collided, obj.id(), &entity_ref::id)
				&& sbj_mass <= obj_mass){
				vec2 correction = sbj.manifold->val().hitbox[intersection.index.sbj_idx].box
				.depart_vector_to(
					obj.manifold->val().hitbox[intersection.index.obj_idx].box);
				// if(correctionVec.equals({}) || correctionVec.dot(approach) < .0f){
				// 	correctionVec = approach.copy().setLength(8);
				// } else{
				// 	correctionVec.limitClamp(5, 15);
				// }
				//
				// auto sbjBox = static_cast<Geom::RectBoxBrief>(hitbox[intersection.index.subject].box);
				// const auto& objBox = object.hitbox[intersection.index.object].box;
				//
				// const Geom::Vec2 begin{sbjBox.v0};
				//
				// while(sbjBox.overlapRough(objBox) && sbjBox.overlapExact(objBox)){
				// 	sbjBox.move(correctionVec);
				// }

				// correctionVec.setLength(3.);
				// manifold.collisionTestTempVel.vec += correctionVec;
				sbj.manifold->val().collision_correction_vec += correction + correction.set_length(2);

				sbj.manifold->val().is_under_correction = true;


			}
		}


		[[nodiscard]] FORCE_INLINE static bool roughIntersectWith(
			const manifold::collision_object& sbj,
			const manifold::collision_object& obj
		) noexcept{
			if(std::abs(sbj.motion->val().depth - obj.motion->val().depth) > sbj.manifold->val().thickness + obj.manifold->val().thickness) return
				false;

			// if(ignoreCollisionBetween(object))return false;

			return sbj.manifold->val().hitbox.collide_with_rough(obj.manifold->val().hitbox);
		}

		FORCE_INLINE static bool check_pre_collision(manifold& manifold, mech_motion& motion) noexcept{
			//TODO is this necessary?
			const bool rst = manifold.filter();

			if(rst){
				//Push trans to hitpos
				if(!manifold.no_backtrace_correction && !manifold.is_under_correction){
					const auto backTrace = manifold.hitbox.get_back_trace_move();

					motion.trans.vec += backTrace;
				}
			} else{
				manifold.is_under_correction = false;
			}

			return rst;
		}

		FORCE_INLINE static void collapse_possible_collisions(component<manifold>& manifold, mech_motion& motion) noexcept{
			for(const auto& collision : manifold->possible_collision){
				assert(manifold.id() != collision.get_other_id());
				for(const auto index : collision.data.indices){

					const auto& sbj_box = manifold->hitbox;
					const auto& obj_box = collision.other.manifold->val().hitbox;

					const auto& sbj = sbj_box[index.sbj_idx].box;
					const auto& obj = obj_box[index.obj_idx].box;

					manifold::collision_confirmed* validPostData{};

					math::rect_box<float> sbjBox = sbj;

					const auto sbjMove = sbj_box.get_back_trace_unit_move();

					if(sbj_box.get_back_trace_move_full().length2() > obj_box.get_back_trace_move_full().length2()){
						//Only apply over-pierce correction to object with faster speed
						const auto correction = sbj.depart_vector_to_on_vel(obj, -sbjMove);
						validPostData = &manifold->confirmed_collision.emplace_back(collision.other, correction);
						sbjBox.move(correction);
					} else{
						validPostData = &manifold->confirmed_collision.emplace_back(collision.other);
					}

					sbjBox.expand_unchecked(5);
					const auto avg = math::rect_rough_avg_intersection(sbjBox, obj);
					assert(avg);

					validPostData->intersection = {
							.pos = avg.pos,
							.normal = math::avg_edge_normal(obj, avg.pos).normalize(),
							.index = index
						};
					assert(!validPostData->intersection.normal.is_NaN());
				}
			}

			manifold->possible_collision.clear();

			if(!manifold->confirmed_collision.empty()){
				math::vec2 off{};
				for(const auto& post_data : manifold->confirmed_collision){
					off += post_data.correction;
				}

				off /= manifold->confirmed_collision.size();
				motion.trans.vec += off;
			}
		}

		FORCE_INLINE static void collisionsPostProcess_3(manifold::collision_object object) noexcept{
			for(const auto& data : object.manifold->val().confirmed_collision){
				rigidCollideWith(object, data.other, data.intersection, 1);
			}
		}

		FORCE_INLINE static void collisionsPostProcess_4(manifold& manifold, mech_motion& motion) noexcept{
			motion.trans.vec += manifold.collision_correction_vec;
			motion.vel += manifold.collision_trans_sum;
			motion.vel.rot.clamp(20.f);
			manifold.collision_correction_vec = {};
			manifold.collision_trans_sum = {};

			// afterCollision();

			manifold.markLast();
			//TODO fire an post-collision event?



			//Fetch manifold pos to final trans pos, necessary to make next ccd correct
			manifold.hitbox.unchecked_set_trans(motion.trans);
		}

	public:
		void insert_all(component_manager& component_manager) noexcept{
			tree.reserved_clear();

			// auto manifolds = component_manager.get_slice_of<manifold>();
			// for (const auto & tuple : manifolds){
			//
			// }
			component_manager.sliced_each([this](
				component<manifold>& manifold,
				component<mech_motion>& motion,
				component<physical_rigid>& rigid
			){
					tree.insert({&manifold, &motion, &rigid});
				});
		}

		void run(component_manager& component_manager){
			component_manager.sliced_each([this](
				component<manifold>& mf,
				component<mech_motion>& motion,
				component<physical_rigid>& rigid
			){
				tree.intersect_all({&mf, &motion, &rigid}, [&](const manifold::collision_object& sbj, const manifold::collision_object& obj){
					if(sbj == obj)return;
					sbj.manifold->val().test_intersection_with(obj);
				}, roughIntersectWith);
			});

			component_manager.sliced_each([](
				component<manifold>& mf,
				component<mech_motion>& motion
			){
					if(!check_pre_collision(mf, motion)) return;

					collapse_possible_collisions(mf, motion);
				});

			component_manager.sliced_each([](
				component<manifold>& mf,
				component<mech_motion>& motion,
				component<physical_rigid>& rigid
			){
					collisionsPostProcess_3({&mf, &motion, &rigid});
				});
			component_manager.sliced_each([](
				component<manifold>& mf,
				component<mech_motion>& motion
			){
					collisionsPostProcess_4(mf, motion);
				});
		}
	};
}
