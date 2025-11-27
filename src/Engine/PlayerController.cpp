#include "PlayerController.hpp"

#include <algorithm>
#include <iostream>
#include "GameObjects/Entity.hpp"
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

CharacterController::Intent PlayerController::gatherIntent(Entity &, double /*dt*/) {
    const auto &actionEvents = m_inputService.getActionEvents();
    m_axisUpdated = false;
    for (const auto &event: actionEvents) {
        consumeActionEvent(event);
    }
    if (!m_axisUpdated) {
        m_axisX = 0.0f;
    }

    CharacterController::Intent intent{};
    // Prioritize analog axis if available, otherwise fall back to digital.
    const float axisVal = (std::abs(m_axisX) >= kGamepadDeadzone) ? m_axisX : 0.0f;
    intent.moveAxis = axisVal;
    if (intent.moveAxis == 0.0f) {
        intent.moveAxis = (m_moveRight ? 1.0f : 0.0f) - (m_moveLeft ? 1.0f : 0.0f);
    }
    intent.jumpPressed = m_jumpQueued;

    // Jump queue is single-use; consume it here.
    m_jumpQueued = false;

    // Simple hysteresis to zero small drift.
    if (std::abs(intent.moveAxis) < kAxisEpsilon) {
        intent.moveAxis = 0.0f;
    }

    return intent;
}
