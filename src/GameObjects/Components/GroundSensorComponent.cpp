#include "GroundSensorComponent.hpp"

#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Entity.hpp"
#include "Physics/Collision/AABB.hpp"
#include "Physics/PhysicsCasts.hpp"
#include <glm/glm.hpp>
#include <GL/glew.h>

namespace {
constexpr float kEpsilon = 1e-5f;
}

GroundSensorComponent::HitInfo GroundSensorComponent::castSensor(const glm::vec2& origin,
                                                                 const glm::vec2& dir,
                                                                 float maxDistance,
                                                                 uint32_t mask,
                                                                 Entity& owner) const {
    HitInfo result{};
    if (!m_worldEntities) {
        return result;
    }
    const float dirLen = glm::length(dir);
    if (dirLen <= kEpsilon) {
        return result;
    }

    const glm::vec2 n = dir / dirLen;
    auto hit = PhysicsCasts::rayCast(origin,
                                     n,
                                     maxDistance,
                                     *m_worldEntities,
                                     PhysicsCasts::CastFilter{
                                         .ignore = &owner,
                                         .includeTriggers = m_includeTriggers,
                                         .layerMask = mask});
    if (!hit.hit) {
        return result;
    }

    result.hit = true;
    result.normal = hit.normal;
    result.point = hit.point;
    result.distance = hit.distance;
    return result;
}

void GroundSensorComponent::updateGroundState(Entity& owner, bool groundedNow) {
    m_justLanded = (!m_wasGrounded && groundedNow);
    m_justLeftGround = (m_wasGrounded && !groundedNow);

    if (m_justLanded && m_onLanded) {
        m_onLanded(owner);
    }
    if (m_justLeftGround && m_onLeftGround) {
        m_onLeftGround(owner);
    }

    m_wasGrounded = groundedNow;
}

void GroundSensorComponent::refresh(Entity& owner) {
    m_groundHit = {};
    m_leftWallHit = {};
    m_rightWallHit = {};
    m_wallContact = false;
    m_wallNormal = glm::vec2{0.0f};
    m_grounded = false;
    m_justLanded = false;
    m_justLeftGround = false;

    if (!m_worldEntities) {
        updateGroundState(owner, false);
        return;
    }

    auto* colliderComp = owner.getComponent<ColliderComponent>();
    if (!colliderComp) {
        updateGroundState(owner, false);
        return;
    }
    colliderComp->ensureCollider(owner);
    auto* collider = colliderComp->collider();
    if (!collider) {
        updateGroundState(owner, false);
        return;
    }

    const AABB bounds = collider->getAABB();
    const glm::vec2 center = bounds.center();
    const glm::vec2 bottomCenter{center.x, bounds.getMin().y - m_groundOffset};
    const glm::vec2 leftCenter{bounds.getMin().x - m_wallOffset, center.y};
    const glm::vec2 rightCenter{bounds.getMax().x + m_wallOffset, center.y};

    m_groundHit = castSensor(bottomCenter,
                             glm::vec2{0.0f, -1.0f},
                             m_groundProbeDistance,
                             m_groundLayerMask,
                             owner);

    m_leftWallHit = castSensor(leftCenter,
                               glm::vec2{-1.0f, 0.0f},
                               m_wallProbeDistance,
                               m_wallLayerMask,
                               owner);

    m_rightWallHit = castSensor(rightCenter,
                                glm::vec2{1.0f, 0.0f},
                                m_wallProbeDistance,
                                m_wallLayerMask,
                                owner);

    m_wallContact = m_leftWallHit.hit || m_rightWallHit.hit;
    if (m_leftWallHit.hit && m_rightWallHit.hit) {
        m_wallNormal = (m_leftWallHit.distance <= m_rightWallHit.distance)
                           ? glm::vec2{1.0f, 0.0f}
                           : glm::vec2{-1.0f, 0.0f};
    } else if (m_leftWallHit.hit) {
        m_wallNormal = glm::vec2{1.0f, 0.0f};
    } else if (m_rightWallHit.hit) {
        m_wallNormal = glm::vec2{-1.0f, 0.0f};
    }

    const bool groundedNow = m_groundHit.hit && m_groundHit.normal.y >= m_minGroundNormalDot;
    m_grounded = groundedNow;
    updateGroundState(owner, groundedNow);
}

void GroundSensorComponent::update(Entity& owner, double /*dt*/) {
    refresh(owner);
}

void GroundSensorComponent::debugDraw(const glm::mat4& viewProj,
                                      Entity& owner,
                                      const glm::vec3& groundColor,
                                      const glm::vec3& wallColor) const {
    auto* colliderComp = owner.getComponent<ColliderComponent>();
    if (!colliderComp) return;

    colliderComp->ensureCollider(owner);
    auto* collider = colliderComp->collider();
    if (!collider) return;

    const AABB bounds = collider->getAABB();
    const glm::vec2 center = bounds.center();
    const glm::vec2 bottomCenter{center.x, bounds.getMin().y - m_groundOffset};
    const glm::vec2 leftCenter{bounds.getMin().x - m_wallOffset, center.y};
    const glm::vec2 rightCenter{bounds.getMax().x + m_wallOffset, center.y};

    glUseProgram(0);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(&viewProj[0][0]);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glLineWidth(1.0f);

    auto drawRay = [](const glm::vec2& from, const glm::vec2& dir, float len, const glm::vec3& color) {
        glColor3f(color.r, color.g, color.b);
        glBegin(GL_LINES);
        glVertex2f(from.x, from.y);
        const glm::vec2 to = from + dir * len;
        glVertex2f(to.x, to.y);
        glEnd();
        // tip
        glPointSize(4.0f);
        glBegin(GL_POINTS);
        glVertex2f(to.x, to.y);
        glEnd();
    };

    // Draw last-known probes based on stored hits/distances; fall back to probe max.
    // Ground
    {
        const glm::vec2 dir{0.0f, -1.0f};
        const float len = m_groundHit.hit ? m_groundHit.distance : m_groundProbeDistance;
        drawRay(bottomCenter, dir, len, groundColor);
    }

    // Left wall
    {
        const glm::vec2 dir{-1.0f, 0.0f};
        const float len = m_leftWallHit.hit ? m_leftWallHit.distance : m_wallProbeDistance;
        drawRay(leftCenter, dir, len, wallColor);
    }

    // Right wall
    {
        const glm::vec2 dir{1.0f, 0.0f};
        const float len = m_rightWallHit.hit ? m_rightWallHit.distance : m_wallProbeDistance;
        drawRay(rightCenter, dir, len, wallColor);
    }
}
