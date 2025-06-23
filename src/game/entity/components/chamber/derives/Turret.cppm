module;


export module mo_yanxi.game.ecs.component.chamber.turret;

export import mo_yanxi.game.ecs.component.chamber;
export import mo_yanxi.game.meta.content_ref;
export import mo_yanxi.game.meta.projectile;
export import mo_yanxi.game.meta.chamber;
export import mo_yanxi.game.meta.turret;
import mo_yanxi.math;

import std;

namespace mo_yanxi::game::ecs{
	namespace chamber{

		export
		enum struct turret_state{
			standby,
			will_standby,
			reloading,
			shooting
		};

		struct turret_cache{

			float projectile_speed{};
		};

		export
		struct turret_build : building{
			meta::turret::turret_body body;

			math::trans2z transform{};
			float rotate_torque{};
			float shooting_field_angle{};

			math::angle rotation{};
			float standby_reload{};


			float reload{};
			unsigned burst_progress{};
			float burst_reload{};
			target target{};

		private:
			turret_cache cache_{};

		public:

			[[nodiscard]] turret_build() = default;

			void reset_body(const meta::turret::turret_body& body) noexcept{
				this->body = body;
				cache_.projectile_speed = body.shoot_type.projectile->initial_speed;
			}

			void build_hud(ui::table& where, const entity_ref& eref) const override;

			void draw_hud(graphic::renderer_ui& renderer) const override;

			[[nodiscard]] bool validate_target(const math::vec2 target_local_trs) const noexcept{
				return body.range.within_open(target_local_trs.length());
			}

		private:
			[[nodiscard]] bool is_shooting() const noexcept{
				return burst_progress > 0;
			}

			void begin_shoot() noexcept{
				burst_progress = 1;
			}

			void shoot(const chunk_meta& chunk_meta, world::entity_top_world& top_world) const;

			void rotate(){
				if(target){
					const auto& motion = target.entity->at<mech_motion>();
					const auto pos = get_local_to_global_trans(transform.vec);

					if(const auto rst =
						estimate_shoot_hit_delay(
							target.local_pos,
							motion.vel.vec,
							cache_.projectile_speed,
							body.shoot_type.offset.trunk.x)){
						const auto tgt = pos.apply_inv_to(motion.pos() + motion.vel.vec * rst);
						const auto ang = tgt.angle_rad();
						rotation.rotate_toward(ang, get_update_delta());
					}
				}
			}

			void reload_turret(const chunk_meta& chunk_meta, world::entity_top_world& top_world){
				if(!is_shooting()){
					if(const auto rst = math::forward_approach_then(reload, body.reload_duration, get_update_delta())){
						reload = 0;
						begin_shoot();
					}else{
						reload = rst;
					}
				}else{
					if(const auto rst = math::forward_approach_then(burst_reload, body.shoot_type.burst.spacing, get_update_delta())){
						burst_reload = 0;
						if(burst_progress <= body.shoot_type.burst.count){
							++burst_progress;
							shoot(chunk_meta, top_world);
						}else{
							burst_progress = 0;
						}
					}else{
						burst_reload = rst;
					}
				}
			}

		public:
			void update(const chunk_meta& chunk_meta, world::entity_top_world& top_world) override{
				if(target){
					target.update(transform.get_trans() | data().get_trans());
					if(!validate_target(target.local_pos)){
						target = nullptr;
					}else{
						reload_turret(chunk_meta, top_world);
					}
				}else if(auto tgt = data().grid().targets_primary.get_optimal()){
					if(validate_target(data().get_trans().apply_inv_to((*tgt->*&mech_motion::pos)()))){
						target = tgt;
					}else{
						target = nullptr;
					}

				}else{
					target = nullptr;
				}


				rotate();

				/*if(target && !target.update(turret_center_local_trans | data().get_trans())){
					return;
				}

				auto& motion = target.entity->at<mech_motion>();
				auto target_trs = turret_center_local_trans.apply_inv_to(motion.trans);
				if(!validate_target(target_trs.vec)){
					target.entity.reset(nullptr);
				}*/
			}
		};

	}


	export
	struct turret_build_dump{

	};

	template <>
	struct component_custom_behavior<chamber::turret_build> : component_custom_behavior_base<chamber::turret_build>, chamber::building_trait_base<>{
	};

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
