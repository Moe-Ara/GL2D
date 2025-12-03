#include "VehicleController.hpp"

#include <algorithm>
#include <cmath>
#include <algorithm>
#include <glm/glm.hpp>

#include "GameObjects/Components/ControllerComponent.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Components/GroundSensorComponent.hpp"
#include "GameObjects/Entity.hpp"
#include "InputSystem/InputTypes.hpp"
#include "GameObjects/Components/WaterStateComponent.hpp"
#include "Engine/CharacterController.hpp"
#include "Engine/PlayerController.hpp"

namespace {
constexpr float kAxisEpsilon = 0.001f;
constexpr float kGamepadDeadzone = 0.1f;
}

void VehicleController::consumeActionEvent(const ActionEvent& event) {
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
        m_jumpPressed = true;
    }
}

void VehicleController::mount(Entity& rider) {
    if (m_rider == &rider) {
        return;
    }
    if (m_riderController) {
        m_riderController->setEnabled(true);
    }

    m_rider = &rider;
    m_riderController = rider.getComponent<ControllerComponent>();
    if (m_riderController) {
        m_riderController->setEnabled(false);
    }
    m_riderCharacterController = m_riderController
        ? dynamic_cast<CharacterController*>(m_riderController->controller())
        : nullptr;
    m_moveLeft = false;
    m_moveRight = false;
    m_axisX = 0.0f;
    m_axisUpdated = false;
    m_jumpPressed = false;
    m_resetVelocityOnMount = true;
}

void VehicleController::dismount() {
    if (m_riderController) {
        m_riderController->setEnabled(true);
        if (auto* playerCtrl = dynamic_cast<PlayerController*>(m_riderController->controller())) {
            playerCtrl->resetInputState();
        }
    }
    if (m_rider) {
        if (auto* riderRbComp = m_rider->getComponent<RigidBodyComponent>()) {
            if (auto* riderBody = riderRbComp->body()) {
                riderBody->setVelocity(glm::vec2{0.0f});
            }
        }
        if (m_riderCharacterController) {
            m_riderCharacterController->resetVelocity();
        }
    }
    m_rider = nullptr;
    m_riderController = nullptr;
    m_jumpPressed = false;
    m_riderCharacterController = nullptr;
}

void VehicleController::syncRiderToSeat(Entity& vehicle) {
    if (!m_rider) return;
    auto* vehicleTransform = vehicle.getComponent<TransformComponent>();
    auto* riderTransform = m_rider->getComponent<TransformComponent>();
    if (!vehicleTransform || !riderTransform) return;

    const glm::vec2 seatPos = vehicleTransform->getTransform().Position + m_seatOffset;
    riderTransform->setPosition(seatPos);

    if (auto* riderRb = m_rider->getComponent<RigidBodyComponent>()) {
        if (auto* rb = riderRb->body()) {
            rb->setVelocity(glm::vec2{0.0f});
            rb->setPosition(seatPos);
        }
    }
}

void VehicleController::update(Entity& entity, double dt) {
    if (!m_rider) {
        return;
    }

    m_axisUpdated = false;
    m_jumpPressed = false;
    for (const auto& evt : m_inputService.getActionEvents()) {
        consumeActionEvent(evt);
    }

    if (m_jumpPressed) {
        dismount();
        return;
    }

    auto* rbComp = entity.getComponent<RigidBodyComponent>();
    auto* body = rbComp ? rbComp->body() : nullptr;
    if (!body) {
        return;
    }
    if (m_resetVelocityOnMount) {
        // Drop any leftover motion from before mounting so the first input direction is not cancelled.
        body->setVelocity(glm::vec2{0.0f});
        m_resetVelocityOnMount = false;
    }

    const float dtf = static_cast<float>(dt);
    auto* waterState = entity.getComponent<WaterStateComponent>();
    const float submersion = waterState ? waterState->submersion() : 0.0f;
    const bool inWater = submersion >= m_minSubmersionForMotion;

    glm::vec2 vel = body->getVelocity();
    const auto* sensor = entity.getComponent<GroundSensorComponent>();
    const bool groundContact = sensor && sensor->isGrounded();
    if (!inWater || groundContact) {
        const float dragFactor = std::clamp(1.0f - (m_brakeDrag * 2.0f) * dtf, 0.0f, 1.0f);
        vel.x *= dragFactor;
        if (std::abs(vel.x) < PhysicsUnits::toUnits(0.1f)) {
            vel.x = 0.0f;
        }
        body->setVelocity(vel);
        syncRiderToSeat(entity);
        return;
    }

    float axis = (std::abs(m_axisX) >= kGamepadDeadzone) ? m_axisX : 0.0f;
    if (!m_axisUpdated && std::abs(axis) < kAxisEpsilon) {
        axis = (m_moveRight ? 1.0f : 0.0f) - (m_moveLeft ? 1.0f : 0.0f);
    }

    const float targetX = axis * m_maxSpeed;
    const float accelStep = m_acceleration * dtf;
    if (std::abs(targetX - vel.x) <= accelStep) {
        vel.x = targetX;
    } else {
        vel.x += (targetX > vel.x ? 1.0f : -1.0f) * accelStep;
    }

    // Apply brake drag when no input to stop quickly.
    if (std::abs(axis) < kAxisEpsilon) {
        const float dragFactor = std::clamp(1.0f - m_brakeDrag * dtf, 0.0f, 1.0f);
        vel.x *= dragFactor;
    }

    body->setVelocity(vel);
    syncRiderToSeat(entity);
}
