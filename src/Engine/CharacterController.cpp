#include "CharacterController.hpp"

#include <algorithm>
#include <cmath>

#include "GameObjects/Entity.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Components/SpriteComponent.hpp"

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

    Intent intent = gatherIntent(entity, deltaTime);

    if (!m_groundInitialized) {
        m_groundLevel = transform.Position.y;
        m_groundInitialized = true;
    }

    const float dtf = static_cast<float>(deltaTime);
    const float desiredDir = std::clamp(intent.moveAxis, -1.0f, 1.0f);

    if (std::abs(desiredDir) > kAxisEpsilon) {
        m_velocity.x += desiredDir * m_acceleration * dtf;
        m_velocity.x = std::clamp(m_velocity.x, -m_moveSpeed, m_moveSpeed);
    } else {
        const float decel = m_deceleration * dtf;
        if (std::abs(m_velocity.x) <= decel) {
            m_velocity.x = 0.0f;
        } else {
            m_velocity.x -= decel * (m_velocity.x > 0.0f ? 1.0f : -1.0f);
        }
    }

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
