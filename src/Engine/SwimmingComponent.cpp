#include "SwimmingComponent.hpp"

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

void SwimmingComponent::consumeActionEvent(const ActionEvent& event) {
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
        m_swimUpQueued = true;
    }
}

void SwimmingComponent::update(Entity& owner, double dt) {
    auto* waterState = owner.getComponent<WaterStateComponent>();
    const bool submergedEnough = waterState && waterState->submersion() >= m_activationSubmersion;

    m_axisUpdated = false;
    for (const auto& evt : m_inputService.getActionEvents()) {
        consumeActionEvent(evt);
    }

    if (!submergedEnough) {
        m_swimUpQueued = false;
        return;
    }

    auto* rbComp = owner.getComponent<RigidBodyComponent>();
    auto* body = rbComp ? rbComp->body() : nullptr;
    if (!body) {
        m_swimUpQueued = false;
        return;
    }

    const float dtf = static_cast<float>(dt);
    float axis = (std::abs(m_axisX) >= kGamepadDeadzone) ? m_axisX : 0.0f;
    if (!m_axisUpdated && axis == 0.0f) {
        axis = (m_moveRight ? 1.0f : 0.0f) - (m_moveLeft ? 1.0f : 0.0f);
    }

    glm::vec2 vel = body->getVelocity();
    const float targetX = axis * m_swimSpeed;
    const float accelStep = m_swimAcceleration * dtf;
    if (std::abs(targetX - vel.x) <= accelStep) {
        vel.x = targetX;
    } else {
        vel.x += (targetX > vel.x ? 1.0f : -1.0f) * accelStep;
    }

    if (m_swimUpQueued) {
        vel.y = std::max(vel.y, m_upStrokeImpulse);
        m_swimUpQueued = false;
    }

    if (waterState) {
        vel += (waterState->flowVelocity() - vel) * (m_flowAdherence * dtf);
    }

    vel.y = std::clamp(vel.y, -m_maxDownwardSpeed, m_maxUpwardSpeed);
    body->setVelocity(vel);
}
