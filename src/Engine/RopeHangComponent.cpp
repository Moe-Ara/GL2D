#include "RopeHangComponent.hpp"

#include "GameObjects/Components/ControllerComponent.hpp"
#include "GameObjects/Components/GroundSensorComponent.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Entity.hpp"
#include "InputSystem/InputService.hpp"
#include "Engine/PlayerController.hpp"
#include "Physics/PhysicsCasts.hpp"
#include "Physics/PhysicsUnits.hpp"
#include "Physics/RigidBody.hpp"

#include <glm/glm.hpp>
#include <algorithm>

namespace {
constexpr float kAxisEpsilon = 0.001f;
constexpr float kGamepadDeadzone = 0.1f;
}

RopeHangComponent::RopeHangComponent(InputService& inputService,
                                     ControllerComponent& controller,
                                     std::vector<std::unique_ptr<Entity>>* worldEntities)
        : m_inputService(inputService),
          m_controller(controller),
          m_worldEntities(worldEntities),
          m_detectionRadius(PhysicsUnits::toUnits(1.2f)),
          m_climbSpeed(PhysicsUnits::toUnits(2.25f)) {}

void RopeHangComponent::update(Entity& owner, double dt) {
    m_grabRequested = false;
    m_releaseRequested = false;
    m_axisUpdated = false;
    auto* sensor = owner.getComponent<GroundSensorComponent>();
    const bool grounded = sensor && sensor->isGrounded();

    for (const auto& evt : m_inputService.getActionEvents()) {
        consumeActionEvent(evt);
    }

    if (m_isHanging) {
        if (m_releaseRequested) {
            stopHang(owner);
            return;
        }
        updateHangMovement(owner, dt);
        return;
    }

    const bool canAttach = !grounded || m_grabRequested;
    if (canAttach) {
        if (Entity* ropeEntity = findNearbyRope(owner)) {
            if (auto* ropeInfo = ropeEntity->getComponent<RopeSegmentComponent>()) {
                startHang(owner, *ropeEntity, *ropeInfo);
            }
        }
    }
}

void RopeHangComponent::consumeActionEvent(const ActionEvent& event) {
    const bool pressed = event.eventType == InputEventType::ButtonPressed;
    const bool released = event.eventType == InputEventType::ButtonReleased;
    const bool axisChanged = event.eventType == InputEventType::AxisChanged;

    if (event.actionName == m_grabAction && pressed) {
        m_grabRequested = true;
    } else if (event.actionName == m_releaseAction && pressed) {
        m_releaseRequested = true;
    }

    if (event.actionName == "MoveVertical" && axisChanged) {
        m_axisY = -event.value.y;
        m_moveUp = event.value.y <= -kGamepadDeadzone;
        m_moveDown = event.value.y >= kGamepadDeadzone;
        m_axisUpdated = true;
    } else if (event.actionName == "MoveUp") {
        if (pressed) {
            m_moveUp = true;
        } else if (released) {
            m_moveUp = false;
        }
    } else if (event.actionName == "MoveDown") {
        if (pressed) {
            m_moveDown = true;
        } else if (released) {
            m_moveDown = false;
        }
    }
}

Entity* RopeHangComponent::findNearbyRope(Entity& owner) const {
    if (!m_worldEntities) {
        return nullptr;
    }
    auto* transform = owner.getComponent<TransformComponent>();
    if (!transform) {
        return nullptr;
    }
    const glm::vec2 center = transform->getTransform().Position;
    const auto hits = PhysicsCasts::overlapCircle(center,
                                                  m_detectionRadius,
                                                  *m_worldEntities,
                                                  PhysicsCasts::CastFilter{.ignore = &owner});
    for (const auto& hit : hits) {
        if (!hit.hit || !hit.entity) {
            continue;
        }
        if (hit.entity == &owner) {
            continue;
        }
        if (hit.entity->getComponent<RopeSegmentComponent>()) {
            return hit.entity;
        }
    }
    return nullptr;
}

void RopeHangComponent::applyRopePosition(Entity& owner) {
    if (!m_isHanging) {
        return;
    }
    const glm::vec2 target = m_ropeTop + m_ropeDirection * m_ropeParam;
    if (auto* transform = owner.getComponent<TransformComponent>()) {
        transform->setPosition(target);
    }
    if (auto* bodyComp = owner.getComponent<RigidBodyComponent>()) {
        if (auto* body = bodyComp->body()) {
            body->setPosition(target);
            body->setVelocity(glm::vec2{0.0f});
        }
    }
}

void RopeHangComponent::startHang(Entity& owner, Entity& ropeEntity, RopeSegmentComponent& ropeInfo) {
    if (m_isHanging) {
        return;
    }

    m_ropeDirection = ropeInfo.direction();
    m_ropeTop = ropeInfo.ropeTop();
    m_ropeBottom = ropeInfo.ropeBottom();
    m_ropeLength = ropeInfo.ropeLength();
    m_currentSegment = &ropeEntity;

    if (auto* transform = owner.getComponent<TransformComponent>()) {
        const glm::vec2 rel = transform->getTransform().Position - m_ropeTop;
        m_ropeParam = glm::dot(rel, m_ropeDirection);
        m_ropeParam = std::clamp(m_ropeParam, 0.0f, m_ropeLength);
    } else {
        m_ropeParam = 0.0f;
    }

    if (auto* bodyComp = owner.getComponent<RigidBodyComponent>()) {
        if (auto* body = bodyComp->body()) {
            body->setVelocity(glm::vec2{0.0f});
            body->setBodyType(RigidBodyType::KINEMATIC);
        }
    }

    if (auto* playerCtrl = dynamic_cast<PlayerController*>(m_controller.controller())) {
        playerCtrl->resetInputState();
    }
    m_controller.setEnabled(false);
    m_isHanging = true;
    m_axisY = 0.0f;
    m_moveUp = false;
    m_moveDown = false;
    m_axisUpdated = false;

    applyRopePosition(owner);
}

void RopeHangComponent::stopHang(Entity& owner) {
    if (!m_isHanging) {
        return;
    }
    m_isHanging = false;
    m_currentSegment = nullptr;
    if (auto* bodyComp = owner.getComponent<RigidBodyComponent>()) {
        if (auto* body = bodyComp->body()) {
            body->setBodyType(RigidBodyType::DYNAMIC);
            body->setVelocity(glm::vec2{0.0f});
        }
    }
    m_controller.setEnabled(true);
    m_axisY = 0.0f;
    m_moveUp = false;
    m_moveDown = false;
    m_axisUpdated = false;
}

void RopeHangComponent::updateHangMovement(Entity& owner, double dt) {
    if (!m_isHanging) {
        return;
    }
    float climbInput = 0.0f;
    const float axisValue = (std::abs(m_axisY) >= kGamepadDeadzone) ? m_axisY : 0.0f;
    climbInput = axisValue;
    if (!m_axisUpdated && climbInput == 0.0f) {
        climbInput = (m_moveUp ? 1.0f : 0.0f) - (m_moveDown ? 1.0f : 0.0f);
    }
    if (std::abs(climbInput) >= kAxisEpsilon && m_ropeLength > 0.0f) {
        m_ropeParam = std::clamp(m_ropeParam + climbInput * m_climbSpeed * static_cast<float>(dt),
                                 0.0f,
                                 m_ropeLength);
        applyRopePosition(owner);
    }
}
