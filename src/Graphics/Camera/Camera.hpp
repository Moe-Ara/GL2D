#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/vec4.hpp>
#include <optional>
#include <optional>
#include <memory>
#include <vector>

#include "Utils/Transform.hpp"
#include "CameraEvent.hpp"
#include "ICameraEffect.hpp"
#include "FeelingsSystem/FeelingSnapshot.hpp"

struct CameraDebugData {
    glm::vec2 position{0.0f};
    glm::vec2 deadZoneHalfSize{0.0f};
    glm::vec2 targetPosition{0.0f};
    glm::vec2 lookAhead{0.0f};
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

    void update(double deltaTime);

    Transform &getTransfrom();

    const Transform &getTransfrom() const;

    void setZoom(float zoom);

    float getZoom();

    void setViewportSize(float width, float height);

    void setDeadZoneSize(const glm::vec2 &halfSize);

    void setDamping(float damping);

    void setTarget(const Transform *target, const glm::vec2 &offset = glm::vec2(0.0f));

    void setWorldBounds(const glm::vec4 &bounds);
    void clearWorldBounds();

    void setLookAheadMultiplier(float multiplier);
    void setLookAheadLimits(const glm::vec2 &maxOffsets);
    void setLookAheadSmoothing(float smoothing);
    void setFollowMode(CameraFollowMode mode);

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

    void onEvent(const CameraEvent &event);

    CameraDebugData getDebugData() const;

private:
    void recalcViewProjection();

    void followTargetPosition(const glm::vec2 &targetPos, float deltaTime);

    void updateEffects(double deltaTime);

    glm::vec2 clampToWorldBounds(const glm::vec2 &position) const;
    glm::vec2 computeHalfViewSize() const;

    Transform m_transform{};
    float m_zoom = 1.0f;
    float m_baseZoom = 1.0f;
    glm::vec2 m_viewportSize{};
    glm::vec2 m_deadZoneHalfSize{1.0f, 1.0f};
    float m_damping = 5.0f;
    float m_baseDamping = 5.0f;
    glm::vec2 m_feelingOffset{0.0f};
    const Transform *m_target = nullptr;
    glm::vec2 m_targetOffset = glm::vec2(0.0f);
    bool m_dirty = true;
    glm::mat4 m_viewProjection{1.0f};

    std::vector<std::unique_ptr<ICameraEffect>> m_effects;
    CameraEffectState m_effectState;

    glm::vec4 m_worldBounds{0.0f};
    bool m_worldBoundsEnabled{false};
    glm::vec2 m_prevTargetPos{0.0f};
    bool m_hasPrevTargetPos{false};
    glm::vec2 m_currentLookAhead{0.0f};
    glm::vec2 m_lookAheadLimits{150.0f, 100.0f};
    float m_lookAheadMultiplier{0.15f};
    float m_lookAheadSmoothing{6.0f};
    CameraFollowMode m_followMode{CameraFollowMode::DeadZone};
};

#endif
