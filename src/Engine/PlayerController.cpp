#include "PlayerController.hpp"

#include <algorithm>

#include "GameObjects/Entity.hpp"
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

CharacterController::Intent PlayerController::gatherIntent(Entity &, double /*dt*/) {
    const auto &actionEvents = m_inputService.getActionEvents();
    for (const auto &event: actionEvents) {
        consumeActionEvent(event);
    }

    CharacterController::Intent intent{};
    intent.moveAxis = (m_moveRight ? 1.0f : 0.0f) - (m_moveLeft ? 1.0f : 0.0f);
    intent.jumpPressed = m_jumpQueued;

    // Jump queue is single-use; consume it here.
    m_jumpQueued = false;

    // Simple hysteresis to zero small drift.
    if (std::abs(intent.moveAxis) < kAxisEpsilon) {
        intent.moveAxis = 0.0f;
    }

    return intent;
}

