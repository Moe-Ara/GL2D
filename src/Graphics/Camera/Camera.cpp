//
// Created by Mohamad on 19/11/2025.
//
#include <cmath>
#include <algorithm>
#include <limits>
#include <stdexcept>

#include <glm/common.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include "Graphics/Camera/Camera.hpp"
#include "Graphics/Camera/Events/ScreenShakeEffect.hpp"

namespace {
    constexpr float kMaximumFollowDelay = 5.0f;
    constexpr std::size_t kMaximumConcurrentEffects = 32U;

    bool finite(const glm::vec2& value) {
        return std::isfinite(value.x) && std::isfinite(value.y);
    }

    bool finite(const glm::vec4& value) {
        return std::isfinite(value.x) && std::isfinite(value.y) &&
               std::isfinite(value.z) && std::isfinite(value.w);
    }

    class ZoomPulseEffect final : public ICameraEffect {
    public:
        ZoomPulseEffect(float magnitude, float duration)
                : m_magnitude(std::max(0.0f, magnitude)),
                  m_duration(duration > 0.0f ? duration : 0.2f) {}

        bool update(double deltaTime, CameraEffectState &state) override {
            m_elapsed += static_cast<float>(deltaTime);
            if (m_elapsed >= m_duration) {
                return false;
            }
            const float progress = m_elapsed / m_duration;
            const float falloff = (1.0f - progress) * (1.0f - progress);
            const float oscillation = std::sin(progress * glm::pi<float>());
            const float pulse = 1.0f + oscillation * m_magnitude * falloff;
            state.zoomMultiplier *= std::max(0.2f, pulse);
            return true;
        }

    private:
        float m_magnitude;
        float m_duration;
        float m_elapsed{0.0f};
    };
}

Camera::Camera(float viewportWidth, float viewportHeight) {
    m_transform.setPos(glm::vec2(0.0f));
    m_transform.setScale(glm::vec2(1.0f));
    m_transform.setRotation(0.0f);

    setViewportSize(viewportWidth, viewportHeight);
    m_deadZoneHalfSize = 0.25f * m_viewportSize;
    m_baseZoom = m_zoom;
    m_baseDamping = m_damping;
}

void Camera::setZoom(float zoom) {
    if (!std::isfinite(zoom) || zoom <= 0.0f) {
        throw std::invalid_argument("Camera zoom must be finite and positive");
    }
    m_baseZoom = std::max(zoom, 0.01f);
    m_zoomTransition.reset();
    refreshFeelingOverrides();
}

void Camera::setViewportSize(float width, float height) {
    const float safeWidth = std::isfinite(width) ? std::max(width, 1.0f) : 1.0f;
    const float safeHeight = std::isfinite(height) ? std::max(height, 1.0f) : 1.0f;
    m_viewportSize = {safeWidth, safeHeight};
    m_transform.setPos(clampToWorldBounds(m_transform.Position));
    m_dirty = true;
}

void Camera::setFollowMode(CameraFollowMode mode) {
    m_followMode = mode;
    m_dirty = true;
}

void Camera::update(double deltaTime) {
    if (!std::isfinite(deltaTime) || deltaTime < 0.0 ||
        deltaTime > static_cast<double>(std::numeric_limits<float>::max())) {
        throw std::invalid_argument("Camera delta time must be finite and non-negative");
    }
    const float dt = static_cast<float>(deltaTime);
    updateZoomTransition(dt);
    if (m_target) {
        const glm::vec2 currentTargetPosition = m_target->Position + m_targetOffset;
        if (!finite(currentTargetPosition)) {
            throw std::invalid_argument("Camera target position must be finite");
        }
        const glm::vec2 targetPosition = sampleDelayedTarget(currentTargetPosition, deltaTime);
        followTargetPosition(targetPosition, dt);
        m_prevTargetPos = targetPosition;
        m_hasPrevTargetPos = true;
    } else {
        m_hasPrevTargetPos = false;
        m_targetHistory.clear();
        const float decayAlpha = m_lookAheadSmoothing <= 0.0f
            ? 1.0f : 1.0f - std::exp(-m_lookAheadSmoothing * dt);
        m_currentLookAhead = glm::mix(m_currentLookAhead, glm::vec2(0.0f), decayAlpha);
    }
    updateEffects(deltaTime);
    m_transform.setPos(clampToWorldBounds(m_transform.Position));
}

void Camera::followTargetPosition(const glm::vec2 &targetPos, float deltaTime) {
    glm::vec2 camPos = m_transform.Position;
    glm::vec2 desiredPos = camPos;
    const glm::vec2 offset = targetPos - camPos;
    const bool hardLock = m_followMode == CameraFollowMode::HardLock;
    switch (m_followMode) {
        case CameraFollowMode::CenterOnTarget:
            desiredPos = targetPos;
            break;
        case CameraFollowMode::HardLock:
            m_currentLookAhead = glm::vec2(0.0f);
            m_transform.setPos(clampToWorldBounds(targetPos));
            m_dirty = true;
            return;
        case CameraFollowMode::DeadZone:
            if (offset.x > m_deadZoneHalfSize.x)
                desiredPos.x = (targetPos.x - m_deadZoneHalfSize.x);
            else if (offset.x < -m_deadZoneHalfSize.x) desiredPos.x = targetPos.x + m_deadZoneHalfSize.x;

            if (offset.y > m_deadZoneHalfSize.y) desiredPos.y = targetPos.y - m_deadZoneHalfSize.y;
            else if (offset.y < -m_deadZoneHalfSize.y) desiredPos.y = targetPos.y + m_deadZoneHalfSize.y;
            break;
        case CameraFollowMode::None:
        default:
            desiredPos = camPos;
            break;
    }

    if (m_followMode != CameraFollowMode::None && !hardLock) {
        glm::vec2 targetVelocity{0.0f};
        const float velocityDt = std::max(0.0001f, deltaTime);
        if (m_hasPrevTargetPos) {
            targetVelocity = (targetPos - m_prevTargetPos) / velocityDt;
        }
        glm::vec2 desiredLookAhead = targetVelocity * m_lookAheadMultiplier;
        desiredLookAhead = glm::clamp(desiredLookAhead, -m_lookAheadLimits, m_lookAheadLimits);
        const float lookAheadAlpha = m_lookAheadSmoothing <= 0.0f
            ? 1.0f : 1.0f - std::exp(-m_lookAheadSmoothing * velocityDt);
        m_currentLookAhead = glm::mix(m_currentLookAhead, desiredLookAhead, lookAheadAlpha);
        desiredPos += m_currentLookAhead;
    } else {
        m_currentLookAhead = glm::vec2(0.0f);
    }

    const float smoothingDt = static_cast<float>(deltaTime);
    const float smoothingAlpha = m_damping <= 0.0f
                                 ? 1.0f
                                 : 1.0f - std::exp(-m_damping * smoothingDt);

    glm::vec2 smoothedPos = glm::mix(camPos, desiredPos, smoothingAlpha);
    smoothedPos = clampToWorldBounds(smoothedPos);
    m_transform.setPos(smoothedPos);
    m_dirty = true;
}

Transform &Camera::getTransfrom() {
    return m_transform;
}

const Transform &Camera::getTransfrom() const {
    return m_transform;
}

Transform &Camera::getTransform() {
    return m_transform;
}

const Transform &Camera::getTransform() const {
    return m_transform;
}

float Camera::getZoom() const {
    return m_zoom;
}

void Camera::setDeadZoneSize(const glm::vec2 &halfSize) {
    if (!finite(halfSize)) {
        throw std::invalid_argument("Camera dead-zone size must be finite");
    }
    m_deadZoneHalfSize = glm::abs(halfSize);
}

void Camera::setDamping(float damping) {
    if (!std::isfinite(damping) || damping < 0.0f) {
        throw std::invalid_argument("Camera damping must be finite and non-negative");
    }
    m_baseDamping = damping;
    refreshFeelingOverrides();
}

void Camera::setFollowDelay(float seconds) {
    if (!std::isfinite(seconds) || seconds < 0.0f ||
        seconds > kMaximumFollowDelay) {
        throw std::invalid_argument(
            "Camera follow delay must be finite and in [0, 5] seconds");
    }
    if (m_followDelaySeconds == seconds) return;
    m_followDelaySeconds = seconds;
    resetFollowTracking();
}

float Camera::getFollowDelay() const noexcept {
    return m_followDelaySeconds;
}

void Camera::setTarget(const Transform *target, const glm::vec2 &offset) {
    if (!finite(offset)) {
        throw std::invalid_argument("Camera target offset must be finite");
    }
    const bool changed = m_target != target || m_targetOffset != offset;
    m_target = target;
    m_targetOffset = offset;
    if (changed) resetFollowTracking();
}

void Camera::setWorldBounds(const glm::vec4 &bounds) {
    if (!finite(bounds)) {
        throw std::invalid_argument("Camera world bounds must be finite");
    }
    m_worldBounds = {
        std::min(bounds.x, bounds.z), std::min(bounds.y, bounds.w),
        std::max(bounds.x, bounds.z), std::max(bounds.y, bounds.w)
    };
    m_worldBoundsEnabled = true;
    m_transform.setPos(clampToWorldBounds(m_transform.Position));
    m_dirty = true;
}

void Camera::clearWorldBounds() {
    m_worldBoundsEnabled = false;
}

void Camera::setLookAheadMultiplier(float multiplier) {
    if (!std::isfinite(multiplier) || multiplier < 0.0f) {
        throw std::invalid_argument(
            "Camera look-ahead multiplier must be finite and non-negative");
    }
    m_lookAheadMultiplier = multiplier;
}

void Camera::setLookAheadLimits(const glm::vec2 &maxOffsets) {
    if (!finite(maxOffsets)) {
        throw std::invalid_argument("Camera look-ahead limits must be finite");
    }
    m_lookAheadLimits = glm::abs(maxOffsets);
}

void Camera::setLookAheadSmoothing(float smoothing) {
    if (!std::isfinite(smoothing) || smoothing < 0.0f) {
        throw std::invalid_argument(
            "Camera look-ahead smoothing must be finite and non-negative");
    }
    m_lookAheadSmoothing = smoothing;
}

glm::vec2 Camera::sampleDelayedTarget(const glm::vec2& currentPosition,
                                      double deltaTime) {
    m_followTime += deltaTime;
    m_delayedTargetPosition = currentPosition;
    if (m_followDelaySeconds <= 0.0f) {
        m_targetHistory.clear();
        return currentPosition;
    }

    if (m_targetHistory.empty()) {
        // Prime the history with the current position so assigning a target
        // never causes the camera to wait in an unrelated location.
        m_targetHistory.push_back(
            TargetSample{m_followTime - m_followDelaySeconds, currentPosition});
    }
    if (m_targetHistory.back().time == m_followTime) {
        m_targetHistory.back().position = currentPosition;
    } else {
        m_targetHistory.push_back(TargetSample{m_followTime, currentPosition});
    }

    const double sampleTime = m_followTime - m_followDelaySeconds;
    while (m_targetHistory.size() >= 2U &&
           m_targetHistory[1].time <= sampleTime) {
        m_targetHistory.pop_front();
    }

    if (m_targetHistory.size() == 1U || sampleTime <= m_targetHistory[0].time) {
        m_delayedTargetPosition = m_targetHistory.front().position;
        return m_delayedTargetPosition;
    }

    const TargetSample& first = m_targetHistory[0];
    const TargetSample& second = m_targetHistory[1];
    const double interval = second.time - first.time;
    const float alpha = interval <= 0.0
        ? 1.0f
        : static_cast<float>(std::clamp((sampleTime - first.time) / interval,
                                        0.0, 1.0));
    m_delayedTargetPosition = glm::mix(first.position, second.position, alpha);
    return m_delayedTargetPosition;
}

void Camera::resetFollowTracking() noexcept {
    m_hasPrevTargetPos = false;
    m_prevTargetPos = glm::vec2(0.0f);
    m_currentLookAhead = glm::vec2(0.0f);
    m_targetHistory.clear();
    m_followTime = 0.0;
    m_delayedTargetPosition = m_target
        ? m_target->Position + m_targetOffset
        : m_transform.Position;
}

void Camera::transitionToZoom(float zoom, float durationSeconds) {
    if (!std::isfinite(zoom) || zoom <= 0.0f ||
        !std::isfinite(durationSeconds) || durationSeconds < 0.0f) {
        throw std::invalid_argument(
            "Camera zoom transition requires a positive zoom and non-negative finite duration");
    }
    if (durationSeconds == 0.0f) {
        setZoom(zoom);
        return;
    }
    m_zoomTransition = ZoomTransition{
        m_baseZoom, std::max(zoom, 0.01f), durationSeconds, 0.0f};
}

void Camera::updateZoomTransition(float deltaTime) {
    if (!m_zoomTransition) return;
    ZoomTransition& transition = *m_zoomTransition;
    transition.elapsed = std::min(transition.duration,
                                  transition.elapsed + deltaTime);
    const float t = transition.duration <= 0.0f
        ? 1.0f : transition.elapsed / transition.duration;
    const float eased = t * t * (3.0f - 2.0f * t);
    m_baseZoom = glm::mix(transition.start, transition.target, eased);
    refreshFeelingOverrides();
    if (t >= 1.0f) m_zoomTransition.reset();
}

void Camera::shake(float magnitude, float durationSeconds, float roughness,
                   const glm::vec2& seed) {
    if (!std::isfinite(magnitude) || magnitude < 0.0f ||
        !std::isfinite(durationSeconds) || durationSeconds <= 0.0f ||
        !std::isfinite(roughness) || roughness <= 0.0f || !finite(seed)) {
        throw std::invalid_argument(
            "Camera shake requires finite non-negative magnitude, positive duration/roughness, and finite seed");
    }
    if (magnitude == 0.0f) return;
    if (m_effects.size() >= kMaximumConcurrentEffects) {
        m_effects.erase(m_effects.begin());
    }
    const float sequence = static_cast<float>(m_effectSequence++ % 65521U);
    const glm::vec2 uniqueSeed = seed + glm::vec2{sequence * 0.7548777f,
                                                  sequence * 0.5698403f};
    m_effects.push_back(std::make_unique<ScreenShakeEffect>(
        magnitude, durationSeconds, roughness, uniqueSeed));
}

void Camera::pulseZoom(float magnitude, float durationSeconds) {
    if (!std::isfinite(magnitude) || magnitude < 0.0f ||
        !std::isfinite(durationSeconds) || durationSeconds <= 0.0f) {
        throw std::invalid_argument(
            "Camera zoom pulse requires finite non-negative magnitude and positive duration");
    }
    if (magnitude == 0.0f) return;
    if (m_effects.size() >= kMaximumConcurrentEffects) {
        m_effects.erase(m_effects.begin());
    }
    m_effects.push_back(std::make_unique<ZoomPulseEffect>(magnitude, durationSeconds));
}

void Camera::setShakeLimits(const glm::vec2& maxTranslation,
                            float maxRotationDegrees) {
    if (!finite(maxTranslation) || !std::isfinite(maxRotationDegrees) ||
        maxRotationDegrees < 0.0f) {
        throw std::invalid_argument(
            "Camera shake limits must be finite and rotation cannot be negative");
    }
    m_shakeTranslationLimit = glm::abs(maxTranslation);
    m_shakeRotationLimit = maxRotationDegrees;
}

void Camera::clearEffects() noexcept {
    m_effects.clear();
    m_effectState = CameraEffectState{};
    m_dirty = true;
}

void Camera::applyFeeling(const FeelingsSystem::FeelingSnapshot& feelingsnapshot) {
    float zoomMultiplier = 1.0f;
    if (feelingsnapshot.zoomMul.has_value()) {
        if (!std::isfinite(*feelingsnapshot.zoomMul) || *feelingsnapshot.zoomMul <= 0.0f) {
            throw std::invalid_argument("Feeling camera zoom multiplier must be finite and positive");
        }
        zoomMultiplier = *feelingsnapshot.zoomMul;
    }

    float dampingMultiplier = 1.0f;
    if (feelingsnapshot.followSpeedMul.has_value()) {
        if (!std::isfinite(*feelingsnapshot.followSpeedMul) ||
            *feelingsnapshot.followSpeedMul < 0.0f) {
            throw std::invalid_argument(
                "Feeling camera follow multiplier must be finite and non-negative");
        }
        dampingMultiplier = *feelingsnapshot.followSpeedMul;
    }

    glm::vec2 feelingOffset{0.0f};
    if (feelingsnapshot.offset.has_value()) {
        if (!finite(*feelingsnapshot.offset)) {
            throw std::invalid_argument("Feeling camera offset must be finite");
        }
        feelingOffset = *feelingsnapshot.offset;
    }

    float shakeMagnitude = 0.0f;
    if (feelingsnapshot.shakeMagnitude.has_value()) {
        if (!std::isfinite(*feelingsnapshot.shakeMagnitude) ||
            *feelingsnapshot.shakeMagnitude < 0.0f) {
            throw std::invalid_argument(
                "Feeling camera shake magnitude must be finite and non-negative");
        }
        shakeMagnitude = *feelingsnapshot.shakeMagnitude;
    }

    float shakeRoughness = shakeMagnitude > 0.0f ? 8.0f : 0.0f;
    if (feelingsnapshot.shakeRoughness.has_value()) {
        if (!std::isfinite(*feelingsnapshot.shakeRoughness) ||
            *feelingsnapshot.shakeRoughness < 0.0f) {
            throw std::invalid_argument(
                "Feeling camera shake roughness must be finite and non-negative");
        }
        shakeRoughness = *feelingsnapshot.shakeRoughness;
    }

    m_feelingZoomMultiplier = zoomMultiplier;
    m_feelingDampingMultiplier = dampingMultiplier;
    m_feelingOffset = feelingOffset;
    m_feelingShakeMagnitude = shakeMagnitude;
    m_feelingShakeRoughness = shakeRoughness;
    refreshFeelingOverrides();
}

void Camera::resetFeelingOverrides() {
    m_feelingOffset = glm::vec2(0.0f);
    m_feelingZoomMultiplier = 1.0f;
    m_feelingDampingMultiplier = 1.0f;
    m_feelingShakeMagnitude = 0.0f;
    m_feelingShakeRoughness = 0.0f;
    refreshFeelingOverrides();
}

void Camera::refreshFeelingOverrides() noexcept {
    m_zoom = std::max(0.01f, m_baseZoom * m_feelingZoomMultiplier);
    m_damping = std::max(0.0f, m_baseDamping * m_feelingDampingMultiplier);
    m_dirty = true;
}

void Camera::onEvent(const CameraEvent &event) {
    if (!std::isfinite(event.duration) || event.duration < 0.0f) {
        throw std::invalid_argument(
            "Camera event duration must be finite and non-negative");
    }
    switch (event.type) {
        case CameraEventType::Shake:
            shake(event.magnitude, event.duration > 0.0f ? event.duration : 0.15f,
                  event.roughness, event.worldPos);
            break;
        case CameraEventType::Pulse:
            pulseZoom(event.magnitude, event.duration > 0.0f ? event.duration : 0.2f);
            break;
        case CameraEventType::ZoomTo:
            transitionToZoom(event.targetZoom, event.duration);
            break;
        case CameraEventType::SnapTo:
            if (!finite(event.worldPos)) {
                throw std::invalid_argument("Camera snap position must be finite");
            }
            m_transform.setPos(clampToWorldBounds(event.worldPos));
            resetFollowTracking();
            m_dirty = true;
            break;
        case CameraEventType::HitStop:
        case CameraEventType::Fade:
        default:
            break;
    }
}

void Camera::updateEffects(double deltaTime) {
    m_effectState = CameraEffectState{};
    m_effectTime += deltaTime;
    auto it = m_effects.begin();
    while (it != m_effects.end()) {
        const bool alive = (*it)->update(deltaTime, m_effectState);
        if (!alive) {
            it = m_effects.erase(it);
        } else {
            ++it;
        }
    }

    if (m_feelingShakeMagnitude > 0.0f && m_feelingShakeRoughness > 0.0f) {
        const double wrappedTime = std::fmod(m_effectTime, 4096.0);
        const float phase = static_cast<float>(wrappedTime) *
                            m_feelingShakeRoughness * glm::two_pi<float>();
        const glm::vec2 ambientNoise{
            std::sin(phase) * 0.65f + std::sin(phase * 2.31f + 0.4f) * 0.35f,
            std::cos(phase * 1.27f + 0.8f) * 0.65f +
                std::cos(phase * 2.17f) * 0.35f};
        m_effectState.posOffset += ambientNoise * m_feelingShakeMagnitude;
        m_effectState.rotOffsetDeg += std::sin(phase * 0.61f) *
                                      m_feelingShakeMagnitude * 0.04f;
    }

    if (!finite(m_effectState.posOffset)) m_effectState.posOffset = glm::vec2(0.0f);
    m_effectState.posOffset = glm::clamp(
        m_effectState.posOffset, -m_shakeTranslationLimit, m_shakeTranslationLimit);
    if (!std::isfinite(m_effectState.rotOffsetDeg)) m_effectState.rotOffsetDeg = 0.0f;
    m_effectState.rotOffsetDeg = std::clamp(
        m_effectState.rotOffsetDeg, -m_shakeRotationLimit, m_shakeRotationLimit);
    if (!std::isfinite(m_effectState.zoomMultiplier) ||
        m_effectState.zoomMultiplier <= 0.0f) {
        m_effectState.zoomMultiplier = 1.0f;
    }
    m_effectState.zoomMultiplier = std::clamp(m_effectState.zoomMultiplier, 0.25f, 4.0f);
    m_dirty = true;
}

const glm::mat4 & Camera::getViewProjection() {
    if (m_dirty || m_transform.Position != m_cachedPosition ||
        m_transform.Rotation != m_cachedRotation) {
        recalcViewProjection();
    }
    return m_viewProjection;
}

glm::vec2 Camera::screenToWorld(const glm::vec2 &screenPoint) {
    if (!finite(screenPoint)) {
        throw std::invalid_argument("Camera screen point must be finite");
    }
    const glm::mat4 invVP = glm::inverse(getViewProjection());
    glm::vec2 ndc;
    ndc.x = (2.0f * screenPoint.x / m_viewportSize.x) - 1.0f;
    ndc.y = 1.0f - (2.0f * screenPoint.y / m_viewportSize.y);
    glm::vec4 clip(ndc, 0.0f, 1.0f);
    glm::vec4 world = invVP * clip;
    return glm::vec2(world);
}

glm::vec2 Camera::worldToScreen(const glm::vec2 &worldPoint) {
    if (!finite(worldPoint)) {
        throw std::invalid_argument("Camera world point must be finite");
    }
    glm::vec4 clip = getViewProjection() * glm::vec4(worldPoint, 0.0f, 1.0f);
    glm::vec2 ndc = glm::vec2(clip) / clip.w;
    glm::vec2 screen;
    screen.x = (ndc.x + 1.0f) * 0.5f * m_viewportSize.x;
    screen.y = (1.0f - ndc.y) * 0.5f * m_viewportSize.y;
    return screen;
}

void Camera::recalcViewProjection() {
    const float appliedZoom = std::max(0.01f, m_zoom * std::max(0.1f, m_effectState.zoomMultiplier));
    const auto hw = (m_viewportSize.x * 0.5f) / appliedZoom;
    const auto hh = (m_viewportSize.y * 0.5f) / appliedZoom;

    glm::mat4 proj = glm::ortho(-hw, hw, -hh, hh, -1.0f, 1.0f);
    glm::mat4 view{1.0f};
    const glm::vec2 finalPos = m_transform.Position + m_feelingOffset + m_effectState.posOffset;
    const float finalRot = m_transform.Rotation + m_effectState.rotOffsetDeg;
    view = glm::rotate(view, glm::radians(-finalRot), glm::vec3(0.0f, 0.0f, 1.0f));
    view = glm::translate(view, glm::vec3(-finalPos, 0.0f));
    m_viewProjection = proj * view;
    m_cachedPosition = m_transform.Position;
    m_cachedRotation = m_transform.Rotation;
    m_dirty = false;
}

CameraDebugData Camera::getDebugData() const {
    CameraDebugData data{};
    data.position = m_transform.Position;
    data.deadZoneHalfSize = m_deadZoneHalfSize;
    data.lookAhead = m_currentLookAhead;
    data.targetPosition = m_target ? (m_target->Position + m_targetOffset) : m_transform.Position;
    data.delayedTargetPosition = m_target ? m_delayedTargetPosition : m_transform.Position;
    data.effectOffset = m_effectState.posOffset;
    data.effectRotationDegrees = m_effectState.rotOffsetDeg;
    data.effectZoomMultiplier = m_effectState.zoomMultiplier;
    data.followDelaySeconds = m_followDelaySeconds;
    data.worldBounds = m_worldBounds;
    data.worldBoundsEnabled = m_worldBoundsEnabled;
    return data;
}



glm::vec2 Camera::clampToWorldBounds(const glm::vec2 &position) const {
    if (!m_worldBoundsEnabled) {
        return position;
    }
    glm::vec2 halfView = computeHalfViewSize();
    glm::vec2 minPos = glm::vec2(m_worldBounds.x, m_worldBounds.y) + halfView;
    glm::vec2 maxPos = glm::vec2(m_worldBounds.z, m_worldBounds.w) - halfView;

    if (minPos.x > maxPos.x) {
        const float midX = (minPos.x + maxPos.x) * 0.5f;
        minPos.x = maxPos.x = midX;
    }
    if (minPos.y > maxPos.y) {
        const float midY = (minPos.y + maxPos.y) * 0.5f;
        minPos.y = maxPos.y = midY;
    }
    return glm::clamp(position, minPos, maxPos);
}

glm::vec2 Camera::computeHalfViewSize() const {
    const float appliedZoom = std::max(0.01f, m_zoom * std::max(0.1f, m_effectState.zoomMultiplier));
    const glm::vec2 halfView = 0.5f * (m_viewportSize / appliedZoom);
    const float angle = glm::radians(m_transform.Rotation + m_effectState.rotOffsetDeg);
    const float c = std::abs(std::cos(angle));
    const float s = std::abs(std::sin(angle));
    return {c * halfView.x + s * halfView.y,
            s * halfView.x + c * halfView.y};
}

glm::vec4 Camera::getViewBounds(float paddingFactor) const {
    if (!std::isfinite(paddingFactor)) {
        throw std::invalid_argument("Camera view padding factor must be finite");
    }
    const glm::vec2 halfView = computeHalfViewSize();
    const glm::vec2 paddedHalf = halfView * std::max(0.0f, 1.0f + paddingFactor);
    const glm::vec2 center = m_transform.Position + m_feelingOffset + m_effectState.posOffset;
    return glm::vec4(center.x - paddedHalf.x, center.y - paddedHalf.y,
                     center.x + paddedHalf.x, center.y + paddedHalf.y);
}
