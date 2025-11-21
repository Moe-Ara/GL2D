//
// Created by Mohamad on 19/11/2025.
//
#include <cmath>
#include <algorithm>
#include <limits>

#include <glm/common.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include "Graphics/Camera/Camera.hpp"
#include "Graphics/Camera/Events/ScreenShakeEffect.hpp"

namespace {

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
            const float falloff = 1.0f - progress;
            const float oscillation = std::sin(m_elapsed * glm::pi<float>() * 4.0f);
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

Camera::Camera(float viewportWidth, float viewportHeight) : m_viewportSize(viewportWidth, viewportHeight) {
    m_transform.setPos(glm::vec2(0.0f));
    m_transform.setScale(glm::vec2(1.0f));
    m_transform.Rotation = 0.0f;

    m_deadZoneHalfSize = 0.25f * glm::vec2(viewportWidth, viewportHeight);
}

void Camera::setZoom(float zoom) {
    m_zoom = std::max(zoom, 0.01f);
    m_dirty = true;
}

void Camera::setViewportSize(float width, float height) {
    m_viewportSize = {width, height};
    m_dirty = true;
}

void Camera::update(double deltaTime) {
    if (m_target) {
        glm::vec2 targetPos = m_target->Position + m_targetOffset;
        followTargetPosition(targetPos, deltaTime);
        m_prevTargetPos = targetPos;
        m_hasPrevTargetPos = true;
    } else {
        m_hasPrevTargetPos = false;
        const auto dtf = static_cast<float>(deltaTime);
        const float decayAlpha = glm::clamp(m_lookAheadSmoothing * dtf, 0.0f, 1.0f);
        m_currentLookAhead = glm::mix(m_currentLookAhead, glm::vec2(0.0f), decayAlpha);
    }
    updateEffects(deltaTime);
}

void Camera::followTargetPosition(const glm::vec2 &targetPos, float deltaTime) {
    glm::vec2 camPos = m_transform.Position;
    glm::vec2 desiredPos = camPos;
    const glm::vec2 offset = targetPos - camPos;
    switch (m_followMode) {
        case CameraFollowMode::CenterOnTarget:
            desiredPos = targetPos;
            break;
        case CameraFollowMode::HardLock:
            desiredPos = clampToWorldBounds(targetPos);
            m_transform.setPos(desiredPos);
            m_dirty = true;
            m_currentLookAhead = glm::vec2(0.0f);
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

    if (m_followMode != CameraFollowMode::None) {
        glm::vec2 targetVelocity{0.0f};
        const float velocityDt = std::max(0.0001f, deltaTime);
        if (m_hasPrevTargetPos) {
            targetVelocity = (targetPos - m_prevTargetPos) / velocityDt;
        }
        glm::vec2 desiredLookAhead = targetVelocity * m_lookAheadMultiplier;
        desiredLookAhead = glm::clamp(desiredLookAhead, -m_lookAheadLimits, m_lookAheadLimits);
        const float lookAheadAlpha = glm::clamp(m_lookAheadSmoothing * velocityDt, 0.0f, 1.0f);
        m_currentLookAhead = glm::mix(m_currentLookAhead, desiredLookAhead, lookAheadAlpha);
        desiredPos += m_currentLookAhead;
    } else {
        m_currentLookAhead = glm::vec2(0.0f);
    }

    const float smoothingDt = static_cast<float>(deltaTime);
    const float smoothingAlpha = 1.0f - std::exp(-m_damping * smoothingDt);

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

float Camera::getZoom() {
    return m_zoom;
}

void Camera::setDeadZoneSize(const glm::vec2 &halfSize) {
    m_deadZoneHalfSize = halfSize;
}

void Camera::setDamping(float damping) {
    m_damping = damping;
}

void Camera::setTarget(const Transform *target, const glm::vec2 &offset) {
    m_target = target;
    m_targetOffset = offset;

}

void Camera::setWorldBounds(const glm::vec4 &bounds) {
    m_worldBounds = bounds;
    m_worldBoundsEnabled = true;
}

void Camera::clearWorldBounds() {
    m_worldBoundsEnabled = false;
}

void Camera::setLookAheadMultiplier(float multiplier) {
    m_lookAheadMultiplier = multiplier;
}

void Camera::setLookAheadLimits(const glm::vec2 &maxOffsets) {
    m_lookAheadLimits = glm::abs(maxOffsets);
}

void Camera::setLookAheadSmoothing(float smoothing) {
    m_lookAheadSmoothing = std::max(0.0f, smoothing);
}

void Camera::onEvent(const CameraEvent &event) {
    switch (event.type) {
        case CameraEventType::Shake:
            m_effects.push_back(std::make_unique<ScreenShakeEffect>(event.magnitude, event.duration, event.worldPos));
            break;
        case CameraEventType::Pulse:
            m_effects.push_back(std::make_unique<ZoomPulseEffect>(event.magnitude, event.duration));
            break;
        case CameraEventType::ZoomTo:
            setZoom(event.targetZoom > 0.0f ? event.targetZoom : m_zoom);
            break;
        case CameraEventType::SnapTo:
            m_transform.setPos(event.worldPos);
            m_dirty = true;
            break;
        case CameraEventType::HitStop:
        case CameraEventType::Fade:
        default:
            break;
    }
}

void Camera::updateEffects(double deltaTime) {
    m_effectState=CameraEffectState{};
    auto it=m_effects.begin();
    while(it!=m_effects.end()){
        bool alive=(*it)->update(deltaTime, m_effectState);
        if(!alive)
            it=m_effects.erase(it);
        else
            ++it;
    }
    if(m_effectState.zoomMultiplier<=0.0f)
        m_effectState.zoomMultiplier=1.0f;
    m_dirty=true;
}

const glm::mat4 & Camera::getViewProjection() {
    if (m_dirty)
        recalcViewProjection();
    return m_viewProjection;
}

glm::vec2 Camera::screenToWorld(const glm::vec2 &screenPoint) {
    const glm::mat4 invVP = glm::inverse(getViewProjection());
    glm::vec2 ndc;
    ndc.x = (2.0f * screenPoint.x / m_viewportSize.x) - 1.0f;
    ndc.y = 1.0f - (2.0f * screenPoint.y / m_viewportSize.y);
    glm::vec4 clip(ndc, 0.0f, 1.0f);
    glm::vec4 world = invVP * clip;
    return glm::vec2(world);
}

glm::vec2 Camera::worldToScreen(const glm::vec2 &worldPoint) {
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
    const glm::vec2 finalPos = m_transform.Position + m_effectState.posOffset;
    const float finalRot = m_transform.Rotation + m_effectState.rotOffsetDeg;
    view = glm::translate(view, glm::vec3(-finalPos, 0.0f));
    view = glm::rotate(view, glm::radians(-finalRot), glm::vec3(0.0f, 0.0f, 1.0f));
    m_viewProjection = proj * view;
    m_dirty = false;
}

CameraDebugData Camera::getDebugData() const {
    CameraDebugData data{};
    data.position = m_transform.Position;
    data.deadZoneHalfSize = m_deadZoneHalfSize;
    data.lookAhead = m_currentLookAhead;
    data.targetPosition = m_target ? (m_target->Position + m_targetOffset) : m_transform.Position;
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
    const float appliedZoom = std::max(0.01f, m_zoom);
    return 0.5f * (m_viewportSize / appliedZoom);
}

glm::vec4 Camera::getViewBounds(float paddingFactor) const {
    const glm::vec2 halfView = computeHalfViewSize();
    const glm::vec2 paddedHalf = halfView * (1.0f + paddingFactor);
    const glm::vec2 center = m_transform.Position;
    return glm::vec4(center.x - paddedHalf.x, center.y - paddedHalf.y,
                     center.x + paddedHalf.x, center.y + paddedHalf.y);
}
