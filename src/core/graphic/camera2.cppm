export module mo_yanxi.graphic.camera;

import mo_yanxi.math;
import mo_yanxi.math.rect_ortho;
import mo_yanxi.math.vector2;
import mo_yanxi.math.matrix3;
import mo_yanxi.math.rand;

import std;

namespace mo_yanxi::graphic{
	//TODO poor design
	//TODO 3D support maybe in the future?
	export
	class camera2{
	public:
		static constexpr float DefMaximumScale = 5.0f;
		static constexpr float DefMinimumScale = 0.5f;
		static constexpr float DefScaleSpeed = 0.095f;

		static constexpr float ShakeActivateThreshold = 0.005f;
		static constexpr float ShakeMinSpacing = 1 / 30.f;
		static constexpr float ShakeMinCorrectionSpeed = 0.1f;

	protected:
		bool changed{};

		math::mat3 worldToScreen{};
		math::mat3 screenToWorld{};

		//TODO is a rect necessary??
		math::vec2 screenSize{};

		math::vec2 stablePos{};
		math::frect viewport{};

		math::frect lastViewport{};

		float minScale{std::log(DefMinimumScale)};
		float maxScale{std::log(DefMaximumScale)};

		float scale{get_target_scale_def()};
		float targetScale{get_target_scale_def()};

		float shakeIntensity{0.0f};
		float shakeCorrectionSpeed{ShakeMinCorrectionSpeed};

		float shakeReload{};

		math::rand rand{};

		constexpr void setOrtho(float width, float height) noexcept{
			width /= scale;
			height /= scale;

			viewport.set_size(width, height);
		}

	public:
		math::vec2 move_speed_scale{1, 1};
		float speed_scale{1.};
		float clip_margin{50};

		camera2() = default;

		void resumeSpeed() noexcept{
			speed_scale = 1.f;
		}

		void lock() noexcept{
			speed_scale = 0.f;
		}

		void shake(const float intensity, const float fadeSpeed) noexcept{
			shakeIntensity = math::max(intensity, shakeIntensity);
			shakeCorrectionSpeed = math::max(ShakeMinCorrectionSpeed, (shakeCorrectionSpeed + std::ceil(fadeSpeed)) * 0.5f);
		}

		void set_scale_range(const math::range range = {DefMinimumScale, DefMaximumScale}) noexcept{
			auto [min, max] = range.to_ordered();
			minScale = std::log(min);
			maxScale = std::log(max);
		}

		void resize_screen(const float w, const float h) noexcept /*override*/ { // NOLINT(*-make-member-function-const)
			screenSize.set(w, h);
		}

		[[nodiscard]] constexpr math::frect get_viewport() const noexcept {
			return viewport;
		}

		constexpr void move(const float x, const float y) noexcept {
			stablePos.add(x * speed_scale * move_speed_scale.x, y * speed_scale * move_speed_scale.y);
		}

		constexpr void move(const math::vec2 vec2) noexcept {
			stablePos.add(vec2 * speed_scale * move_speed_scale);
		}

		/**
		 * @return Return the stable position
		 */
		[[nodiscard]] constexpr math::vec2 get_stable_center() const noexcept {
			return stablePos;
		}

		/**
		 * @return Return the viewport center position
		 */
		[[nodiscard]] constexpr math::vec2 get_viewport_center() const noexcept {
			return viewport.get_center();
		}

		/**
		 * @return Return the viewport center position
		 */
		[[nodiscard]] constexpr math::frect get_clip_space() const noexcept {
			return viewport.copy().expand(clip_margin, clip_margin);
		}

		constexpr void set_center(const math::vec2 stablePos) noexcept {
			this->stablePos.set(stablePos);
		}

		[[nodiscard]] constexpr math::vec2 get_screen_center() const noexcept{
			return screenSize / 2.f;
		}

		template <typename T>
		[[nodiscard]] T map_scale(const T& src, const T& dst) const noexcept{
			return math::map(std::log(scale), minScale, maxScale, src, dst);
		}

		void update(const float delta, bool applyShake = true){
			setOrtho(screenSize.x, screenSize.y);

			scale = std::exp(std::lerp(
				std::log(scale), std::log(targetScale), delta * DefScaleSpeed
			));

			viewport.set_center(stablePos);

			if(applyShake && !math::zero(shakeIntensity, ShakeActivateThreshold)){
				float shakeIntensityScl = .25f;
				if(shakeReload >= ShakeMinSpacing){
					shakeReload = 0;
					shakeIntensityScl = 1.f;
				}else{
					shakeReload += delta;
				}

				auto randVec = math::vec2::from_polar(rand.random(360.0f), rand.random(shakeIntensity));
				const auto dstScl = math::curve(randVec.dst(viewport.get_center() - get_stable_center()) / shakeIntensity, 0.85f, 1.65f) * .35f;

				randVec.scl((1 - dstScl) * shakeIntensityScl);

				viewport.move(randVec);

				shakeIntensity = math::approach(shakeIntensity, 0, shakeCorrectionSpeed * delta);
			}

			if(math::zero(scale - targetScale, 0.00025f)) {
				scale = targetScale;
			}

			if(viewport != lastViewport){
				changed = true;

				worldToScreen.set_orthogonal_flip_y(viewport.get_src(), viewport.size());
				screenToWorld.set(worldToScreen).inv();
			}

			lastViewport = viewport;
		}

		bool check_changed() noexcept{
			return std::exchange(changed, false);
		}
		//
		// [[nodiscard]] math::mat3& get_world_to_uniformed() noexcept {
		// 	return worldToScreen;
		// }

		[[nodiscard]] const math::mat3& get_world_to_uniformed() const noexcept {
			return worldToScreen;
		}

		[[nodiscard]] math::mat3 get_world_to_uniformed_flip_y() const noexcept {
			return math::mat3{}.set_orthogonal(viewport.get_src(), viewport.size());
		}

		[[nodiscard]] const math::mat3& get_uniformed_to_world() const noexcept {
			return screenToWorld;
		}

		[[nodiscard]] float get_scale() const noexcept{
			return scale;
		}

		void set_scale(const float scale) noexcept{
			this->scale = scale;
		}

		[[nodiscard]] float get_target_scale() const noexcept{
			return std::log(targetScale);
		}

		void set_target_scale(const float targetScale) noexcept{
			this->targetScale = std::exp(math::clamp(targetScale, minScale, maxScale));
		}

		void set_target_scale_def() noexcept{
			this->targetScale = get_target_scale_def();
		}

		[[nodiscard]] float get_target_scale_def() const noexcept{
			return std::exp(math::clamp(0.0f, minScale, maxScale));
		}

		[[nodiscard]] math::vec2 get_screen_size() const noexcept{
			return screenSize;
		}

		[[nodiscard]] math::vec2 uniform(const math::vec2 screen_pos) const noexcept{
			return (screen_pos / get_screen_size()).sub(0.5f, 0.5f).scl(2, -2);
		}

		[[nodiscard]] math::vec2 get_screen_to_world(const math::vec2 where, const math::vec2 offset = {}) const{
			return screenToWorld * uniform(where - offset);
		}

		[[nodiscard]] math::vec2 get_world_to_screen(const math::vec2 inWorld, const bool flip_y = true) const noexcept{
			return (worldToScreen * inWorld).scl(1, flip_y ? -1 : 1).add(1.f, 1.f).scl(.5f, .5f) * screenSize.as<float>();
		}
	};
}

