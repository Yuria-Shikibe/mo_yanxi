module;

#include <gch/small_vector.hpp>


export module mo_yanxi.game.ecs.component.chamber.turret;

export import mo_yanxi.game.ecs.component.chamber;
export import mo_yanxi.game.meta.projectile;
import mo_yanxi.math;

namespace mo_yanxi::game::ecs{
	namespace chamber{
		struct shoot_mode{
			float reload_duration{};
			unsigned burst_count{};
			float burst_spacing{};
			float inaccuracy{};

			meta::projectile* projectile_type{};
		};
		struct turret_metadata{
			math::range range{};
			float rotation_speed{};
			float default_angle{};
			float shooting_field_angle{};
			float shoot_cone_tolerance{};

			shoot_mode mode{};
		};

		struct turret_build : building{
			turret_metadata meta{};

			math::uniform_trans2 turret_center_local_trans{};
			float reload{};
			unsigned shoot_counter{};
			traced_target target{};

			[[nodiscard]] bool validate_target(const math::vec2 target_local_trs) const noexcept{
				return meta.range.within_open(target_local_trs.length());
			}

			void update(chamber_manifold& grid, const chunk_meta& meta, float delta_in_tick) override{

				if(!target.update(turret_center_local_trans | data().get_trans())){
					return;
				}

				auto& motion = target.entity->at<mech_motion>();
				auto target_trs = turret_center_local_trans.apply_inv_to(motion.trans);
				if(!validate_target(target_trs.vec)){
					target.entity.reset(nullptr);
				}
			}
		};
	}


}

/*
//
// Created by Matrix on 2024/11/30.
//


export module Game.Chamber.TraitedChamber.Spec.Turret;

export import Math;
export import Geom.Transform;
export import Game.Chamber.TraitedChamber;
export import Game.Entity;
export import Game.World.RealEntity;
export import Game.World.Spec.Projectile;
import std;

namespace Game{
	// export class RealEntity;
}

namespace Game::Chamber{
	export
	// struct ChamberedEntity_ChamberBase;
	template <typename Impl>
	struct Interface : ChamberTrait_Interface<Impl, ChamberedEntity_ChamberBase>{
		using EntityType = RealEntity;
	};

	export
	struct Turret : Interface<Turret>{
		[[nodiscard]] Turret() = default;

		struct ShootType{
			unsigned count{1};
			float spacing{0.};
			float charge{30.};

			float angleInaccuracy{};
			float lifetimeScaleInaccuracy{};
			float velocityScaleInaccuracy{};
			float damageInaccuracy{};


			[[nodiscard]] constexpr float shootDuration() const noexcept{
				return static_cast<float>(count) * spacing + charge;
			}
		};

		ShootType shoot{};

		float baseRotateSpeed{2};
		float reloadInterval{30};

		float angleTolerance{30};
		Math::Range range{50, 1000};

		Traits::TraitReference<Traits::ProjectileTrait> projectile{};

		struct DataType{

			/**
			 * @brief local space
			 * turret [x, y, face angle]
			 #1#
			Geom::UniformTransform turretTrans{};
			float rotateSpeed{};

			/**
			 * @brief local space
			 #1#
			std::optional<Geom::Vec2> target{};

			bool autoFire{true};
			bool autoTarget{true};

			struct ShootState{
				bool shooting{};
				float reload{};
				unsigned count{};
				float shootTimer{};
			};

			ShootState shootState;
		};

		struct BuildType : BuildType_Base<DataType>{
			[[nodiscard]] explicit BuildType(const Traits::TraitReference<Turret>& trait)
				: BuildType_Base(trait){
			}

			void update(float delta, EntityType& entity) override;

			[[nodiscard]] Math::Angle angleToTarget() const noexcept{
				if(data.target){
					return {Math::Unchecked, data.turretTrans.vec.angleTo(data.target.value())};
				}else{
					return data.turretTrans.rot;
				}
			}

			void initAfterAdd() override{
				auto region = getBound();
				data.turretTrans.vec = region.getCenter();
			}

			void tryReloading(const float delta, EntityType& entity){
				if(data.shootState.shooting){
					if(data.shootState.shootTimer < trait->shoot.shootDuration()){
						data.shootState.shootTimer += delta;
						shoot(entity);
					}else{
						endShoot(entity);
					}
				}else{
					if(data.shootState.reload < trait->reloadInterval){
						if(data.autoFire){
							const auto [rld, suc] = Math::forward_approach_then(data.shootState.reload, trait->reloadInterval, delta);
							if(suc){
								if(tryFire(entity)){

								}else{
									data.shootState.reload = rld;
								}
							}else{
								data.shootState.reload = rld;
							}

						}else{
							data.shootState.reload = Math::forward_approach(data.shootState.reload, trait->reloadInterval, delta);
						}
					}else{
						if(data.autoFire && tryFire(entity)){

						}
					}
				}
			}

			void shootBegin(){
				data.shootState = {.shooting = true};
			}

			void endShoot(EntityType& entity){
				for(; data.shootState.count < trait->shoot.count; ++data.shootState.count){
					generateProjectile(entity);
				}

				data.shootState = {};
			}

			void rotateToTarget(float delta){
				data.turretTrans.rot.rotate_toward(angleToTarget(), trait->baseRotateSpeed * delta);
			}

			void shoot(RealEntity& entity);

			bool tryFire(EntityType& entity){
				if(!isShootValid())return false;

				shootBegin();
				shoot(entity);

				return true;
			}

			[[nodiscard]] bool isCharging() const noexcept{
				return data.shootState.shootTimer <= trait->shoot.charge;
			}

			[[nodiscard]] float chargeProgress() const noexcept{
				return Math::clamp(data.shootState.shootTimer / trait->shoot.charge);
			}

			[[nodiscard]] bool isShootValid() const noexcept{
				if(!isTargetValid())return false;

				if(!data.turretTrans.rot.equals(angleToTarget(), trait->angleTolerance)){
					return false;
				}

				return true;
			}

			[[nodiscard]] bool isTargetValid() const noexcept{
				if(!hasTarget())return false;

				if(!data.turretTrans.vec.within(data.target.value(), trait->range.to)){
					return false;
				}

				if(data.turretTrans.vec.within(data.target.value(), trait->range.from)){
					return false;
				}

				return true;
			}


			[[nodiscard]] bool hasTarget() const noexcept{
				return data.target.has_value();
			}

			[[nodiscard]] bool isReloading() const noexcept{
				return !data.shootState.shooting && data.shootState.reload < trait->reloadInterval;
			}

			[[nodiscard]] bool isShooting() const noexcept{
				return data.shootState.shooting;
			}

			[[nodiscard]] float reloadProgress() const noexcept{
				return data.shootState.reload / trait->reloadInterval;
			}

			[[nodiscard]] float shootTimeProgress() const noexcept{
				return data.shootState.shootTimer / trait->shoot.shootDuration();
			}

			[[nodiscard]] float shootCountProgress() const noexcept{
				return static_cast<float>(data.shootState.count) / static_cast<float>(trait->shoot.count);
			}

			Core::UI::Element* buildHUD(Core::UI::FlexTable_Dynamic& table) override;

			void draw_contiguous(const EntityType& entity, Graphic::RendererWorld& renderer) const override;

			void draw_local(const EntityType& entity, Graphic::RendererWorld& renderer) const override{

			}

			void draw_selection(const EntityType& entity, Graphic::RendererWorld& renderer) const override;

		protected:
			Projectile& generateProjectile(EntityType& entity) const;
		};
	};
}
*/
