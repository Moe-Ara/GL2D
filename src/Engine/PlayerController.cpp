#include "PlayerController.hpp"


#include <algorithm>
#include <cmath>

#include "GameObjects/Entity.hpp"
#include "GameObjects/Sprite.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Components/SpriteComponent.hpp"
#include "InputSystem/InputTypes.hpp"

namespace {
    constexpr float kAxisEpsilon = 0.001f;
}

void PlayerController::consumeActionEvent(const ActionEvent &event) {
    const bool pressed = event.eventType == InputEventType::ButtonPressed;
    const bool released = event.eventType == InputEventType::ButtonReleased;

    if (event.actionName == "MoveLeft") {
        if (pressed) {
            m_moveLeft = true;
        } else if (released) {
            m_moveLeft = false;
        }
    } else if (event.actionName == "MoveRight") {
        if (pressed) {
            m_moveRight = true;
        } else if (released) {
            m_moveRight = false;
        }
    } else if (event.actionName == "Jump" && pressed) {
        m_jumpQueued = true;
    }
}

void PlayerController::update(Entity &entity, double deltaTime) {
    auto *transformComp = entity.getComponent<TransformComponent>();
    if (!transformComp) {
        return;
    }

    auto *spriteComp = entity.getComponent<SpriteComponent>();
    auto *sprite = spriteComp ? spriteComp->sprite() : nullptr;
    auto &transform = transformComp->getTransform();

    const auto &actionEvents = m_inputService.getActionEvents();
    for (const auto &event: actionEvents) {
        consumeActionEvent(event);
    }

    if (!m_groundInitialized) {
        m_groundLevel = transform.Position.y;
        m_groundInitialized = true;
    }

    const float dtf = static_cast<float>(deltaTime);
    const float desiredDir = (m_moveRight ? 1.0f : 0.0f) - (m_moveLeft ? 1.0f : 0.0f);

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

    if (m_jumpQueued && m_isGrounded) {
        m_velocity.y = m_jumpImpulse;
        m_isGrounded = false;
        m_jumpQueued = false;
    } else if (m_jumpQueued && !m_isGrounded) {
        m_jumpQueued = false;
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
void PlayerController::applyFeeling(const FeelingsSystem::FeelingSnapshot &snapshot) {
    // Reset to baseline first.
    resetFeelingOverrides();
    const float speedMul = snapshot.entitySpeedMul.value_or(1.0f);
    const float animMul = snapshot.animationSpeedMul.value_or(1.0f);

    m_moveSpeed = m_baseMoveSpeed * speedMul;
    m_acceleration = m_baseAcceleration * speedMul;
    m_deceleration = m_baseDeceleration * speedMul;
    m_jumpImpulse = m_baseJumpImpulse * speedMul;
    m_gravity = m_baseGravity * animMul;
}

void PlayerController::resetFeelingOverrides() {
    m_moveSpeed = m_baseMoveSpeed;
    m_acceleration = m_baseAcceleration;
    m_deceleration = m_baseDeceleration;
    m_jumpImpulse = m_baseJumpImpulse;
    m_gravity = m_baseGravity;
}

