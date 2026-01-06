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
    } else if (event.actionName == "MoveVertical" && axisChanged) {
        const float axisY = -event.value.y;
        m_climbAxis = axisY;
        m_moveUp = axisY >= kGamepadDeadzone;
        m_moveDown = axisY <= -kGamepadDeadzone;
        m_climbAxisUpdated = true;
    } else if (event.actionName == "MoveRight") {
        if (pressed) {
            m_moveRight = true;
        } else if (released) {
            m_moveRight = false;
        }
        if (event.sourceEvent.deviceType != InputDeviceType::Gamepad) {
            m_axisX = 0.0f;
        }
    } else if (event.actionName == "MoveUp") {
        if (pressed) {
            m_moveUp = true;
        } else if (released) {
            m_moveUp = false;
        }
        if (event.sourceEvent.deviceType != InputDeviceType::Gamepad) {
            m_climbAxis = 0.0f;
        }
    } else if (event.actionName == "MoveDown") {
        if (pressed) {
            m_moveDown = true;
        } else if (released) {
            m_moveDown = false;
        }
        if (event.sourceEvent.deviceType != InputDeviceType::Gamepad) {
            m_climbAxis = 0.0f;
        }
    } else if (event.actionName == "Jump" && pressed) {
        m_jumpQueued = true;
    }
}

CharacterController::Intent PlayerController::gatherIntent(Entity &entity, double dt) {
    const auto &actionEvents = m_inputService.getActionEvents();
    m_axisUpdated = false;
    m_climbAxisUpdated = false;
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

    const float climbAxisRaw = (std::abs(m_climbAxis) >= kGamepadDeadzone) ? m_climbAxis : 0.0f;
    float climbAxis = climbAxisRaw;
    if (!m_climbAxisUpdated && climbAxis == 0.0f) {
        climbAxis = (m_moveUp ? 1.0f : 0.0f) - (m_moveDown ? 1.0f : 0.0f);
    }
    intent.climbAxis = climbAxis;

    // Jump queue is single-use; consume it here.
    m_jumpQueued = false;

    // Simple hysteresis to zero small drift.
    if (std::abs(intent.moveAxis) < kAxisEpsilon) {
        intent.moveAxis = 0.0f;
    }

    return intent;
}

void PlayerController::resetInputState() {
    m_moveLeft = false;
    m_moveRight = false;
    m_jumpQueued = false;
    m_axisX = 0.0f;
    m_axisUpdated = false;
    m_moveUp = false;
    m_moveDown = false;
    m_climbAxis = 0.0f;
    m_climbAxisUpdated = false;
}
