#include "PlayerController.hpp"

#include <algorithm>
#include <cmath>

#include "GameObjects/Entity.hpp"
#include "GameObjects/Sprite.hpp"
#include "InputSystem/InputTypes.hpp"
#include "Utils/EntityAttributes.hpp"

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
    auto *sprite = entity.getSprite();
    auto &transform = entity.getTransform();

    const auto &actionEvents = m_inputService.getActionEvents();
    for (const auto &event: actionEvents) {
        consumeActionEvent(event);
    }

    if (!m_groundInitialized) {
        m_groundLevel = transform.Position.y;
        m_groundInitialized = true;
    }

    auto &attr = entity.getAttributes();
    const float dtf = static_cast<float>(deltaTime);
    const float desiredDir = (m_moveRight ? 1.0f : 0.0f) - (m_moveLeft ? 1.0f : 0.0f);

    if (std::abs(desiredDir) > kAxisEpsilon) {
        m_velocity.x += desiredDir * attr.acceleration * dtf;
        m_velocity.x = std::clamp(m_velocity.x, -attr.moveSpeed, attr.moveSpeed);
    } else {
        const float decel = attr.deceleration * dtf;
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
    entity.tickComponents(deltaTime);
}
