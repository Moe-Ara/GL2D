#include "BoatController.hpp"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "GameObjects/Components/WaterStateComponent.hpp"
#include "GameObjects/Entity.hpp"
#include "InputSystem/InputTypes.hpp"

namespace {
constexpr float kGamepadDeadzone = 0.1f;
}

void BoatController::consumeActionEvent(const ActionEvent& event) {
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
        m_boostQueued = true;
    }
}

void BoatController::update(Entity& entity, double dt) {
    m_axisUpdated = false;
    for (const auto& evt : m_inputService.getActionEvents()) {
        consumeActionEvent(evt);
    }

    float axis = (std::abs(m_axisX) >= kGamepadDeadzone) ? m_axisX : 0.0f;
    if (!m_axisUpdated && axis == 0.0f) {
        axis = (m_moveRight ? 1.0f : 0.0f) - (m_moveLeft ? 1.0f : 0.0f);
    }

    auto* rbComp = entity.getComponent<RigidBodyComponent>();
    auto* body = rbComp ? rbComp->body() : nullptr;
    if (!body) {
        m_boostQueued = false;
        return;
    }

    auto* waterState = entity.getComponent<WaterStateComponent>();
    const bool inWater = waterState && waterState->submersion() > 0.05f;

    glm::vec2 vel = body->getVelocity();
    const float dtf = static_cast<float>(dt);
    const float targetX = axis * (inWater ? m_maxSpeed : m_maxSpeed * 0.5f);
    const float accelStep = m_acceleration * dtf;
    if (std::abs(targetX - vel.x) <= accelStep) {
        vel.x = targetX;
    } else {
        vel.x += (targetX > vel.x ? 1.0f : -1.0f) * accelStep;
    }

    if (inWater) {
        vel += (waterState->flowVelocity() - vel) * (m_flowMatch * dtf);
        const float dragFactor = std::clamp(1.0f - m_turnDrag * dtf, 0.0f, 1.0f);
        vel.x *= dragFactor;
        if (m_boostQueued) {
            vel.y = std::max(vel.y, m_boostImpulse);
        }
    }

    m_boostQueued = false;
    body->setVelocity(vel);
}
