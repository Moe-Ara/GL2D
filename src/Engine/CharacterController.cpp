#include "CharacterController.hpp"

#include <algorithm>
#include <cmath>

#include "GameObjects/Entity.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Components/SpriteComponent.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "GameObjects/Components/GroundSensorComponent.hpp"
#include "Physics/RigidBody.hpp"

namespace {
constexpr float kAxisEpsilon = 0.001f;
}

void CharacterController::update(Entity &entity, double deltaTime) {
    auto *transformComp = entity.getComponent<TransformComponent>();
    if (!transformComp) {
        return;
    }

    auto *spriteComp = entity.getComponent<SpriteComponent>();
    auto *sprite = spriteComp ? spriteComp->sprite() : nullptr;
    auto &transform = transformComp->getTransform();
    auto *rbComp = entity.getComponent<RigidBodyComponent>();
    RigidBody* body = rbComp ? rbComp->body() : nullptr;

    Intent intent = gatherIntent(entity, deltaTime);

    if (!m_groundInitialized) {
        // Assume ground at world y=0 for simple platformer movement; adjust as needed per level.
        m_groundLevel = 0.0f;
        m_groundInitialized = true;
    }

    const float dtf = static_cast<float>(deltaTime);
    const float desiredDir = std::clamp(intent.moveAxis, -1.0f, 1.0f);
    const float targetVelX = std::abs(desiredDir) > kAxisEpsilon ? desiredDir * m_moveSpeed * std::abs(desiredDir) : 0.0f;

    // Smoothly approach target velocity using acceleration as the rate.
    const float accelStep = m_acceleration * dtf;
    if (std::abs(targetVelX - m_velocity.x) <= accelStep) {
        m_velocity.x = targetVelX;
    } else {
        m_velocity.x += (targetVelX > m_velocity.x ? 1.0f : -1.0f) * accelStep;
    }

    auto* sensor = entity.getComponent<GroundSensorComponent>();
    if (sensor) {
        if (m_worldEntities) {
            sensor->setWorldEntities(m_worldEntities);
        }
        if (!m_sensorCallbacksBound) {
            sensor->setCallbacks([this](Entity& e) { onLanded(e); },
                                 [this](Entity& e) { onLeftGround(e); });
            m_sensorCallbacksBound = true;
        }
        sensor->refresh(entity); // Avoid ordering issues if controller runs before sensor component.
        m_wallContact = sensor->hasWallContact();
        m_wallNormal = sensor->wallNormal();
    } else {
        m_wallContact = false;
        m_wallNormal = glm::vec2{0.0f};
    }
    const bool groundedBySensor = sensor && sensor->isGrounded();

    if (body) {
        auto vel = body->getVelocity();
        vel.x = m_velocity.x;

        const float groundedVelEps = PhysicsUnits::toUnits(0.015f);
        m_isGrounded = groundedBySensor || (std::abs(vel.y) <= groundedVelEps);
        if (intent.jumpPressed && m_isGrounded) {
            vel.y = m_jumpImpulse;
            m_isGrounded = false;
        }
        if (!m_isGrounded) {
            vel.y -= m_gravity * dtf;
        }

        body->setVelocity(vel);
    } else {
        const bool groundedByVel = std::abs(m_velocity.y) <= PhysicsUnits::toUnits(0.015f);
        m_isGrounded = groundedBySensor || groundedByVel;
        if (intent.jumpPressed && m_isGrounded) {
            m_velocity.y = m_jumpImpulse;
            m_isGrounded = false;
        }

        if (!m_isGrounded) {
            m_velocity.y -= m_gravity * dtf;
        }

        glm::vec2 position = transform.Position;
        position += m_velocity * dtf;

        if (position.y <= m_groundLevel) {
            position.y = m_groundLevel;
            m_velocity.y = 0.0f;
            m_isGrounded = true;
        }

        transform.setPos(position);
        if (sprite) {
            sprite->setPosition(position);
        }
    }
}

void CharacterController::applyFeeling(const FeelingsSystem::FeelingSnapshot &snapshot) {
    resetFeelingOverrides();
    const float speedMul = snapshot.entitySpeedMul.value_or(1.0f);
    const float animMul = snapshot.animationSpeedMul.value_or(1.0f);

    m_moveSpeed = m_baseMoveSpeed * speedMul;
    m_acceleration = m_baseAcceleration * speedMul;
    m_deceleration = m_baseDeceleration * speedMul;
    m_jumpImpulse = m_baseJumpImpulse * speedMul;
    m_gravity = m_baseGravity * animMul;
}

void CharacterController::resetFeelingOverrides() {
    m_moveSpeed = m_baseMoveSpeed;
    m_acceleration = m_baseAcceleration;
    m_deceleration = m_baseDeceleration;
    m_jumpImpulse = m_baseJumpImpulse;
    m_gravity = m_baseGravity;
}
