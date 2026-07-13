#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <cstdint>
#include <deque>
#include <memory>
#include <glm/vec4.hpp>
#include <optional>
#include <vector>

#include "Utils/Transform.hpp"
#include "CameraEvent.hpp"
#include "ICameraEffect.hpp"
#include "FeelingsSystem/FeelingSnapshot.hpp"

struct CameraDebugData {
    glm::vec2 position{0.0f};
    glm::vec2 deadZoneHalfSize{0.0f};
    glm::vec2 targetPosition{0.0f};
    glm::vec2 delayedTargetPosition{0.0f};
    glm::vec2 lookAhead{0.0f};
    glm::vec2 effectOffset{0.0f};
    float effectRotationDegrees{0.0f};
    float effectZoomMultiplier{1.0f};
    float followDelaySeconds{0.0f};
    glm::vec4 worldBounds{0.0f};
    bool worldBoundsEnabled{false};
};

enum class CameraFollowMode {
    None,
    CenterOnTarget,
    DeadZone,
    HardLock
};

class Camera {
public:
    Camera(float viewportWidth, float viewportHeight);

    virtual ~Camera() = default;

    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;
    Camera(Camera&&) = delete;
    Camera& operator=(Camera&&) = delete;

    void update(double deltaTime);

    Transform &getTransfrom();

    const Transform &getTransfrom() const;

    // Correctly spelled aliases. The original methods remain for source compatibility.
    Transform &getTransform();
    const Transform &getTransform() const;

    void setZoom(float zoom);

    [[nodiscard]] float getZoom() const;

    void setViewportSize(float width, float height);

    void setDeadZoneSize(const glm::vec2 &halfSize);

    void setDamping(float damping);

    // Delays target sampling without delaying gameplay. Small values such as
    // 0.03-0.08 seconds add cinematic weight while preserving responsiveness.
    void setFollowDelay(float seconds);
    [[nodiscard]] float getFollowDelay() const noexcept;

    void setTarget(const Transform *target, const glm::vec2 &offset = glm::vec2(0.0f));

    void setWorldBounds(const glm::vec4 &bounds);
    void clearWorldBounds();

    void setLookAheadMultiplier(float multiplier);
    void setLookAheadLimits(const glm::vec2 &maxOffsets);
    void setLookAheadSmoothing(float smoothing);
    void setFollowMode(CameraFollowMode mode);

    // Smoothly changes the baseline zoom using a smoothstep curve. A zero
    // duration behaves like setZoom().
    void transitionToZoom(float zoom, float durationSeconds);

    // Transient cinematic effects. Translation is expressed in world units,
    // roughness in cycles per second, and duration in seconds.
    void shake(float magnitude, float durationSeconds, float roughness = 22.0f,
               const glm::vec2& seed = glm::vec2(0.0f));
    void pulseZoom(float magnitude, float durationSeconds = 0.2f);
    void setShakeLimits(const glm::vec2& maxTranslation, float maxRotationDegrees);
    void clearEffects() noexcept;

    // Apply feeling-derived overrides (zoom/follow speed/offset/shake).
    void applyFeeling(const FeelingsSystem::FeelingSnapshot& feelingsnapshot);
    // Clear transient overrides back to baseline.
    void resetFeelingOverrides();

    const glm::mat4 & getViewProjection();

    // Returns world-space view bounds as (minX, minY, maxX, maxY).
    // paddingFactor expands half-extents by this fraction of the current half-extents.
    glm::vec4 getViewBounds(float paddingFactor = 0.0f) const;

    glm::vec2 screenToWorld(const glm::vec2 &screenPoint);
    glm::vec2 worldToScreen(const glm::vec2 &worldPoint);
    glm::vec2 getViewportSize() const { return m_viewportSize; }

    void onEvent(const CameraEvent &event);

    CameraDebugData getDebugData() const;

private:
    struct TargetSample {
        double time{0.0};
        glm::vec2 position{0.0f};
    };

    struct ZoomTransition {
        float start{1.0f};
        float target{1.0f};
        float duration{0.0f};
        float elapsed{0.0f};
    };

    void recalcViewProjection();

    void followTargetPosition(const glm::vec2 &targetPos, float deltaTime);

    void updateEffects(double deltaTime);
    void updateZoomTransition(float deltaTime);
    [[nodiscard]] glm::vec2 sampleDelayedTarget(const glm::vec2& currentPosition,
                                                double deltaTime);
    void resetFollowTracking() noexcept;
    void refreshFeelingOverrides() noexcept;

    glm::vec2 clampToWorldBounds(const glm::vec2 &position) const;
    glm::vec2 computeHalfViewSize() const;

    Transform m_transform{};
    float m_zoom = 1.0f;
    float m_baseZoom = 1.0f;
    float m_feelingZoomMultiplier = 1.0f;
    glm::vec2 m_viewportSize{};
    glm::vec2 m_deadZoneHalfSize{1.0f, 1.0f};
    float m_damping = 5.0f;
    float m_baseDamping = 5.0f;
    float m_feelingDampingMultiplier = 1.0f;
    glm::vec2 m_feelingOffset{0.0f};
    float m_feelingShakeMagnitude{0.0f};
    float m_feelingShakeRoughness{0.0f};
    const Transform *m_target = nullptr;
    glm::vec2 m_targetOffset = glm::vec2(0.0f);
    bool m_dirty = true;
    glm::mat4 m_viewProjection{1.0f};
    glm::vec2 m_cachedPosition{0.0f};
    float m_cachedRotation{0.0f};

    std::vector<std::unique_ptr<ICameraEffect>> m_effects;
    CameraEffectState m_effectState;
    double m_effectTime{0.0};
    std::uint64_t m_effectSequence{0};
    glm::vec2 m_shakeTranslationLimit{64.0f, 48.0f};
    float m_shakeRotationLimit{3.0f};
    std::optional<ZoomTransition> m_zoomTransition;

    glm::vec4 m_worldBounds{0.0f};
    bool m_worldBoundsEnabled{false};
    glm::vec2 m_prevTargetPos{0.0f};
    bool m_hasPrevTargetPos{false};
    std::deque<TargetSample> m_targetHistory;
    double m_followTime{0.0};
    float m_followDelaySeconds{0.0f};
    glm::vec2 m_delayedTargetPosition{0.0f};
    glm::vec2 m_currentLookAhead{0.0f};
    glm::vec2 m_lookAheadLimits{150.0f, 100.0f};
    float m_lookAheadMultiplier{0.15f};
    float m_lookAheadSmoothing{6.0f};
    CameraFollowMode m_followMode{CameraFollowMode::DeadZone};
};

#endif
