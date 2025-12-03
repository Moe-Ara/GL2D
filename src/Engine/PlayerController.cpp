#include "PlayerController.hpp"

#include <algorithm>
#include <iostream>
#include <glm/glm.hpp>
#include "GameObjects/Entity.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "InputSystem/InputTypes.hpp"

namespace {
    constexpr float kAxisEpsilon = 0.001f;
    constexpr float kGamepadDeadzone = 0.1f;
}

void PlayerController::consumeActionEvent(const ActionEvent &event) {
    const bool pressed = event.eventType == InputEventType::ButtonPressed;
    const bool released = event.eventType == InputEventType::ButtonReleased;
    const bool axisChanged = event.eventType == InputEventType::AxisChanged;

    if (event.actionName == "MoveLeft") {
        if (pressed) {
            m_moveLeft = true;
        } else if (released) {
            m_moveLeft = false;
        }
        if (event.sourceEvent.deviceType != InputDeviceType::Gamepad) {
            m_axisX = 0.0f;
        }
    } else if (event.actionName == "MoveHorizontal" && axisChanged) {
        m_axisX = event.value.x;
        m_moveLeft = event.value.x <= -kGamepadDeadzone;
        m_moveRight = event.value.x >= kGamepadDeadzone;
        m_axisUpdated = true;
    } else if (event.actionName == "MoveRight") {
        if (pressed) {
            m_moveRight = true;
        } else if (released) {
            m_moveRight = false;
        }
        if (event.sourceEvent.deviceType != InputDeviceType::Gamepad) {
            m_axisX = 0.0f;
        }
    } else if (event.actionName == "Jump" && pressed) {
        m_jumpQueued = true;
    }
}

CharacterController::Intent PlayerController::gatherIntent(Entity &entity, double dt) {
    const auto &actionEvents = m_inputService.getActionEvents();
    m_axisUpdated = false;
    for (const auto &event: actionEvents) {
        consumeActionEvent(event);
    }

    CharacterController::Intent intent{};
    // Prioritize analog axis if available, otherwise fall back to digital.
    const float axisVal = (std::abs(m_axisX) >= kGamepadDeadzone) ? m_axisX : 0.0f;
    intent.moveAxis = axisVal;
    if (!m_axisUpdated && intent.moveAxis == 0.0f) {
        intent.moveAxis = (m_moveRight ? 1.0f : 0.0f) - (m_moveLeft ? 1.0f : 0.0f);
    }
    intent.jumpPressed = m_jumpQueued;

    // Jump queue is single-use; consume it here.
    m_jumpQueued = false;

    // Simple hysteresis to zero small drift.
    if (std::abs(intent.moveAxis) < kAxisEpsilon) {
        intent.moveAxis = 0.0f;
    }

    // Periodically log movement inputs and body speed to tune walk/run thresholds.
    m_speedLogTimer += static_cast<float>(dt);
    if (m_speedLogTimer >= 0.25f) {
        float speed = 0.0f;
        float vx = 0.0f;
        float vy = 0.0f;
        if (auto *rb = entity.getComponent<RigidBodyComponent>()) {
            if (auto *body = rb->body()) {
                const auto vel = body->getVelocity();
                vx = vel.x;
                vy = vel.y;
                speed = glm::length(vel);
            }
        }
        std::cout << "[Movement] speed=" << speed << " vx=" << vx << " vy=" << vy
                  << " axis=" << intent.moveAxis << " rawX=" << m_axisX << std::endl;
        m_speedLogTimer = 0.0f;
    }

    return intent;
}

void PlayerController::resetInputState() {
    m_moveLeft = false;
    m_moveRight = false;
    m_jumpQueued = false;
    m_axisX = 0.0f;
    m_axisUpdated = false;
}
