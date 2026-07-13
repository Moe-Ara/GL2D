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
#include <cmath>
#include <limits>
#include <stdexcept>

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
          m_climbSpeed(PhysicsUnits::toUnits(2.25f)),
          m_releaseSpeed(PhysicsUnits::toUnits(3.5f)) {}

void RopeHangComponent::setDetectionRadius(float radius) {
    if (!std::isfinite(radius) || radius <= 0.0f) {
        throw std::invalid_argument(
            "Rope detection radius must be finite and positive");
    }
    m_detectionRadius = radius;
}

void RopeHangComponent::setClimbSpeed(float speed) {
    if (!std::isfinite(speed) || speed < 0.0f) {
        throw std::invalid_argument(
            "Rope climb speed must be finite and non-negative");
    }
    m_climbSpeed = speed;
}

void RopeHangComponent::setReleaseSpeed(float speed) {
    if (!std::isfinite(speed) || speed < 0.0f) {
        throw std::invalid_argument(
            "Rope release speed must be finite and non-negative");
    }
    m_releaseSpeed = speed;
}

void RopeHangComponent::update(Entity& owner, double dt) {
    if (!std::isfinite(dt) || dt < 0.0) {
        throw std::invalid_argument(
            "RopeHangComponent requires finite, non-negative delta time");
    }
    m_grabRequested = false;
    m_releaseRequested = false;
    m_axisUpdated = false;

    if (m_lastActionFrame != m_inputService.actionFrame()) {
        for (const auto& evt : m_inputService.getActionEvents()) {
            consumeActionEvent(evt);
        }
        m_lastActionFrame = m_inputService.actionFrame();
    }

    if (m_isHanging) {
        if (!ropeInfo(m_currentSegment)) {
            stopHang(owner);
            return;
        }
        if (m_releaseRequested) {
            stopHang(owner, true);
            return;
        }
        updateHangMovement(owner, dt);
        return;
    }

    if (m_grabRequested) {
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
        m_axisY = -event.value.x;
        m_moveUp = event.value.x <= -kGamepadDeadzone;
        m_moveDown = event.value.x >= kGamepadDeadzone;
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
    Entity* nearest = nullptr;
    float nearestDistanceSquared = std::numeric_limits<float>::max();
    for (const auto& hit : hits) {
        if (!hit.hit || !hit.entity) {
            continue;
        }
        if (hit.entity == &owner) {
            continue;
        }
        if (auto* rope = hit.entity->getComponent<RopeSegmentComponent>()) {
            const auto [start, end] = rope->worldEndpoints(*hit.entity);
            const glm::vec2 segment = end - start;
            const float lengthSquared = glm::dot(segment, segment);
            const float parameter = lengthSquared > 1e-6f
                ? std::clamp(glm::dot(center - start, segment) / lengthSquared,
                             0.0f, 1.0f)
                : 0.0f;
            const glm::vec2 closest = start + segment * parameter;
            const float distanceSquared = glm::dot(center - closest,
                                                    center - closest);
            if (distanceSquared < nearestDistanceSquared ||
                (distanceSquared == nearestDistanceSquared && nearest &&
                 hit.entity->getId() < nearest->getId())) {
                nearest = hit.entity;
                nearestDistanceSquared = distanceSquared;
            }
        }
    }
    return nearest;
}

bool RopeHangComponent::applyRopePosition(Entity& owner) {
    RopeSegmentComponent* rope = ropeInfo(m_currentSegment);
    if (!m_isHanging || !rope) {
        return false;
    }
    const auto [start, end] = rope->worldEndpoints(*m_currentSegment);
    const float length = glm::length(end - start);
    if (length <= 1e-6f) {
        return false;
    }
    m_segmentDistance = std::clamp(m_segmentDistance, 0.0f, length);
    const glm::vec2 target = start + (end - start) *
        (m_segmentDistance / length);
    if (auto* transform = owner.getComponent<TransformComponent>()) {
        transform->setPosition(target);
    }
    if (auto* bodyComp = owner.getComponent<RigidBodyComponent>()) {
        if (auto* body = bodyComp->body()) {
            body->setPosition(target);
            body->setVelocity(glm::vec2{0.0f});
        }
    }
    return true;
}

void RopeHangComponent::startHang(Entity& owner, Entity& ropeEntity, RopeSegmentComponent& ropeInfo) {
    if (m_isHanging) {
        return;
    }

    m_currentSegment = &ropeEntity;

    const auto [start, end] = ropeInfo.worldEndpoints(ropeEntity);
    const glm::vec2 segment = end - start;
    const float length = glm::length(segment);
    if (length <= 1e-6f) {
        m_currentSegment = nullptr;
        return;
    }
    if (auto* transform = owner.getComponent<TransformComponent>()) {
        const glm::vec2 rel = transform->getTransform().Position - start;
        m_segmentDistance = std::clamp(
            glm::dot(rel, segment / length), 0.0f, length);
    } else {
        m_segmentDistance = 0.0f;
    }

    if (auto* bodyComp = owner.getComponent<RigidBodyComponent>()) {
        if (auto* body = bodyComp->body()) {
            m_previousBodyType = body->getBodyType();
            m_previousGravityScale = body->getGravityScale();
            body->setVelocity(glm::vec2{0.0f});
            body->setBodyType(RigidBodyType::KINEMATIC);
        }
    }

    if (auto* playerCtrl = dynamic_cast<PlayerController*>(m_controller.controller())) {
        playerCtrl->resetInputState();
    }
    m_controllerWasEnabled = m_controller.isEnabled();
    m_controller.setEnabled(false);
    m_isHanging = true;
    m_axisY = 0.0f;
    m_moveUp = false;
    m_moveDown = false;
    m_axisUpdated = false;

    if (!applyRopePosition(owner)) {
        stopHang(owner);
    }
}

void RopeHangComponent::stopHang(Entity& owner, bool jumpRelease) {
    if (!m_isHanging) {
        return;
    }
    glm::vec2 inheritedVelocity{0.0f};
    if (containsWorldEntity(m_currentSegment)) {
        if (auto* segmentBody = m_currentSegment->getComponent<RigidBodyComponent>()) {
            if (segmentBody->body()) {
                inheritedVelocity = segmentBody->body()->getVelocity();
            }
        }
    }
    if (auto* bodyComp = owner.getComponent<RigidBodyComponent>()) {
        if (auto* body = bodyComp->body()) {
            body->setBodyType(m_previousBodyType);
            body->setGravityScale(m_previousGravityScale);
            if (m_previousBodyType != RigidBodyType::STATIC) {
                body->setVelocity(inheritedVelocity +
                                  glm::vec2{0.0f, jumpRelease ? m_releaseSpeed : 0.0f});
            }
        }
    }
    m_isHanging = false;
    m_currentSegment = nullptr;
    m_controller.setEnabled(m_controllerWasEnabled);
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
    m_segmentDistance -= climbInput * m_climbSpeed * static_cast<float>(dt);
    while (RopeSegmentComponent* current = ropeInfo(m_currentSegment)) {
        const float length = current->segmentLength();
        if (m_segmentDistance < 0.0f) {
            Entity* previous = current->previous();
            RopeSegmentComponent* previousInfo = ropeInfo(previous);
            if (!previousInfo) {
                m_segmentDistance = 0.0f;
                break;
            }
            m_currentSegment = previous;
            m_segmentDistance += previousInfo->segmentLength();
            continue;
        }
        if (m_segmentDistance > length) {
            Entity* next = current->next();
            if (!ropeInfo(next)) {
                m_segmentDistance = length;
                break;
            }
            m_currentSegment = next;
            m_segmentDistance -= length;
            continue;
        }
        break;
    }
    if (!applyRopePosition(owner)) {
        stopHang(owner);
    }
}

bool RopeHangComponent::containsWorldEntity(const Entity* entity) const {
    if (!entity || !m_worldEntities) {
        return false;
    }
    return std::ranges::any_of(*m_worldEntities, [entity](const auto& candidate) {
        return candidate.get() == entity;
    });
}

RopeSegmentComponent* RopeHangComponent::ropeInfo(Entity* entity) const {
    return containsWorldEntity(entity)
        ? entity->getComponent<RopeSegmentComponent>() : nullptr;
}
