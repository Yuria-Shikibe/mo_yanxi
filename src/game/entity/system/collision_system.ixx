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

	constexpr float MinimumSuccessAccuracy = 1. / 32.;
	constexpr float MinimumFailedAccuracy = 1. / 32.;

	math::vec2 approachTest(
		math::rect_box<float>& sbjBox, const math::vec2 sbjMove,
		math::rect_box<float>& objBox, const math::vec2 objMove
	){
		const math::vec2 beginning = sbjBox[0];
		float currentTestStep = .5f;
		float failedSum = .0f;

		bool lastTestResult = sbjBox.overlap_rough(objBox) && sbjBox.overlap_exact(objBox);

		while(true){
			sbjBox.move(sbjMove * currentTestStep);
			objBox.move(objMove * currentTestStep);

			if(sbjBox.overlap_rough(objBox) && sbjBox.overlap_exact(objBox)){
				if(std::abs(currentTestStep) < MinimumSuccessAccuracy){
					break;
				}

				//continue backtrace
				currentTestStep /= lastTestResult ? 2.f : -2.f;
				failedSum = .0f;

				lastTestResult = true;
			}else{
				if(std::abs(currentTestStep) < MinimumFailedAccuracy){
					if(lastTestResult){
						failedSum = -currentTestStep;
					}
					//backtrace to last succeed position
					sbjBox.move(sbjMove * failedSum);
					objBox.move(objMove * failedSum);

					break;
				}

				//failed, perform forward trace
				failedSum -= currentTestStep;
				currentTestStep /= lastTestResult ? -2.f : 2.f;

				lastTestResult = false;
			}
		}

		return sbjBox[0] - beginning;
	}

	export
	template <>
	struct quad_tree_trait_adaptor<manifold::collision_object, float> : quad_tree_adaptor_base<manifold::collision_object, float>{
		[[nodiscard]] static rect_type get_bound(const_reference self) noexcept{
			return self.manifold->hitbox.max_wrap_bound();
		}

		[[nodiscard]] static bool intersect_with(const_reference self, const_reference other){
			return self.manifold->hitbox.collide_with_exact(other.manifold->hitbox);
		}

		[[nodiscard]] static bool contains(const_reference self, vector_type::const_pass_t point){
			return self.manifold->hitbox.contains(point);
		}

		[[nodiscard]] static bool equals(const_reference self, const_reference other) noexcept{
			return self.manifold == other.manifold;
		}
	};

	export
	struct collision_system{
	private:
		std::vector<manifold*> pre_passed_manifolds{};

		quad_tree<manifold::collision_object> tree{{{0, 0}, 100000}};


		FORCE_INLINE static void rigidCollideWith(
			const manifold::collision_object& sbj,
			const manifold::collision_object& obj,
			const manifold::intersection& intersection,
			const math::vec2 sbj_correction,
			const float energyScale) noexcept{
			assert(sbj != obj);

			using math::vec2;

			const vec2 dst_to_sbj = intersection.pos - (sbj.motion->pos() + sbj_correction);
			const vec2 dst_to_obj = intersection.pos - obj.motion->pos();

			const vec2 sbj_vel = sbj.motion->vel_at(dst_to_sbj);
			const vec2 obj_vel = obj.motion->vel_at(dst_to_obj);

			const vec2 rel_vel = (obj_vel - sbj_vel) * energyScale;

			const vec2 hit_normal = intersection.normal;

			const auto sbj_mass = sbj.rigid->inertial_mass;
			const auto obj_mass = obj.rigid->inertial_mass;

			const auto scaled_mass = 1 / sbj_mass + 1 / obj_mass;
			const auto sbj_rotational_inertia = sbj.manifold->hitbox.getRotationalInertia(sbj_mass, sbj.rigid->rotational_inertia_scale);
			const auto obj_rotational_inertia = obj.manifold->hitbox.getRotationalInertia(obj_mass, obj.rigid->rotational_inertia_scale);

			const vec2 vert_hit_vel{rel_vel.copy().project_scaled(hit_normal)};
			const vec2 hori_hit_vel{(rel_vel - vert_hit_vel)};

			const vec2 hit_tangent = hori_hit_vel.equals({})
				                                 ? hit_normal.copy().rotate_rt_clockwise()
				                                 : hori_hit_vel.copy().normalize();

			const vec2 impulse_normal = vert_hit_vel * (1 +
				math::min(sbj.rigid->restitution, obj.rigid->restitution)) /
				(scaled_mass +
					math::sqr(dst_to_obj.cross(hit_normal)) / obj_rotational_inertia +
					math::sqr(dst_to_sbj.cross(hit_normal)) / sbj_rotational_inertia
				);

			const vec2 impulse_tangent = (hori_hit_vel * hori_hit_vel.length() /
						(scaled_mass +
							math::sqr(dst_to_obj.cross(hit_tangent)) / obj_rotational_inertia +
							math::sqr(dst_to_sbj.cross(hit_tangent)) / sbj_rotational_inertia))
				.limit_max_length(std::sqrt(sbj.rigid->friction_coefficient * obj.rigid->friction_coefficient) * impulse_normal.length());

			const vec2 impulse = impulse_normal + impulse_tangent;

			sbj.manifold->collision_vel_trans_sum.vec += impulse / sbj.rigid->inertial_mass;
			sbj.manifold->collision_vel_trans_sum.rot += dst_to_sbj.cross(impulse) / sbj_rotational_inertia * math::rad_to_deg;

			auto sbj_vel_len2 = sbj.motion->vel.vec.length2();
			auto obj_vel_len2 = obj.motion->vel.vec.length2();
			if(math::zero(sbj_vel_len2 - obj_vel_len2, 4.f) && sbj_mass <= obj_mass){
				math::rect_box box{sbj.manifold->hitbox[intersection.index.sbj_idx].box};
				box.move(sbj_correction);

				vec2 min_correction = box.depart_vector_to(obj.manifold->hitbox[intersection.index.obj_idx].box);

				sbj.manifold->collision_correction_vec += min_correction.set_length(1.f);

				sbj.manifold->is_under_correction = true;

			}else if(sbj_vel_len2 > obj_vel_len2/* && std::ranges::contains(sbj.manifold->last_collided, obj.id, &entity_ref::id)*/){
				math::rect_box box{sbj.manifold->hitbox[intersection.index.sbj_idx].box};
				box.move(sbj_correction);

				vec2 min_correction = box.depart_vector_to_on_vel_rough_min(obj.manifold->hitbox[intersection.index.obj_idx].box, sbj.motion->vel.vec);

				sbj.manifold->collision_correction_vec += min_correction.set_length(1.f);

				sbj.manifold->is_under_correction = true;

			}
		}


		[[nodiscard]] FORCE_INLINE static bool roughIntersectWith(
			const manifold::collision_object& sbj,
			const manifold::collision_object& obj
		) noexcept{
			if(std::abs(sbj.motion->depth - obj.motion->depth) > sbj.manifold->thickness + obj.manifold->thickness) return
				false;

			return sbj.manifold->hitbox.collide_with_rough(obj.manifold->hitbox);
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
			math::vec2 pre_correction_sum{};

			for(const auto& collision : manifold.possible_collision){
				for(const auto index : collision.data.indices){

					const auto& sbj_box = manifold.hitbox;
					const auto& obj_box = collision.other.manifold->hitbox;

					math::rect_box sbj = sbj_box[index.sbj_idx].box;
					math::rect_box obj = obj_box[index.obj_idx].box;

					if(!manifold.is_under_correction){
						sbj.move(sbj_box.get_back_trace_move());
					}
					obj.move(obj_box.get_back_trace_move());

					// math::vec2 correction{approachTest(
					// 	sbj, sbj_box.get_back_trace_unit_move(),
					// 	obj, obj_box.get_back_trace_unit_move()
					// )};

					math::vec2 correction{};

					if(motion.vel.vec.length2() > collision.other.motion->vel.vec.length2()){
						//Only apply over-pierce correction to object with faster speed
						correction = sbj.depart_vector_to_on_vel(obj, motion.vel.vec);
						sbj.move(correction);
					}

					const auto avg_s = sbj.expand_unchecked(2);
					const auto avg_o = obj.expand_unchecked(2);

					auto avg = math::rect_rough_avg_intersection(sbj, obj);
					if(!avg)avg.pos = (avg_s + avg_o) / 2;

					pre_correction_sum += correction;

					auto& data = manifold.confirmed_collision.emplace_back(
						collision.other,
						correction, manifold::intersection{
							.pos = avg.pos,
							.normal = math::avg_edge_normal(obj, avg.pos).normalize(),
							.index = index
						});

					assert(!data.intersection.normal.is_NaN());
				}
			}

			manifold.possible_collision.clear();

			if(!manifold.confirmed_collision.empty()){
				pre_correction_sum /= manifold.confirmed_collision.size();
				motion.trans.vec += pre_correction_sum;
			}
		}

		FORCE_INLINE static void collisionsPostProcess_3(manifold::collision_object object) noexcept{
			for(const auto& data : object.manifold->confirmed_collision){
				rigidCollideWith(object, data.other, data.intersection, data.correction, 1 / static_cast<float>(object.manifold->confirmed_collision.size()));
			}
		}

		FORCE_INLINE static void collisionsPostProcess_4(manifold& manifold, mech_motion& motion) noexcept{
			motion.trans.vec += manifold.collision_correction_vec;
			motion.vel += manifold.collision_vel_trans_sum;
			// motion.vel.rot.clamp(20.f);
			manifold.collision_correction_vec = {};
			manifold.collision_vel_trans_sum = {};

			manifold.markLast();

			//Fetch manifold pos to final trans pos, necessary to make next ccd correct
			manifold.hitbox.set_trans_unchecked(motion.trans);
		}

	public:
		void insert_all(component_manager& component_manager) noexcept{
			tree.reserved_clear();

			component_manager.sliced_each([this](
				component<chunk_meta>& meta,
				component<manifold>& manifold,
				component<mech_motion>& motion,
				component<physical_rigid>& rigid
			){
					tree.insert({meta.id(), &manifold, &motion, &rigid});
				});
		}

		void run(component_manager& component_manager){
			component_manager.sliced_each([this](
				const component<chunk_meta>& meta,
				component<manifold>& mf,
				component<mech_motion>& motion,
				component<physical_rigid>& rigid
			){
				tree.intersect_all({meta.id(), &mf, &motion, &rigid}, [&](const manifold::collision_object& sbj, const manifold::collision_object& obj){
					if(sbj == obj)return;
					sbj.manifold->test_intersection_with(obj);
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
				const component<chunk_meta>& meta,
				component<manifold>& mf,
				component<mech_motion>& motion,
				component<physical_rigid>& rigid
			){
					collisionsPostProcess_3({meta.id(), &mf, &motion, &rigid});
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
