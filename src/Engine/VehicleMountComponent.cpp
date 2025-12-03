#include "VehicleMountComponent.hpp"

#include <glm/glm.hpp>
#include <GL/glew.h>
#include <cmath>

#include "Engine/VehicleController.hpp"
#include "GameObjects/Components/ControllerComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Entity.hpp"
#include "Graphics/Camera/Camera.hpp"
#include "InputSystem/InputTypes.hpp"
#include "Physics/Collision/AABB.hpp"

bool VehicleMountComponent::isInteractPressedThisFrame() const {
    for (const auto& evt : m_inputService.getActionEvents()) {
        if (evt.actionName == m_interactAction && evt.eventType == InputEventType::ButtonPressed) {
            return true;
        }
    }
    return false;
}

bool VehicleMountComponent::riderIsClose(Entity& owner) const {
    if (!m_rider) return false;

    // Expand the vehicle collider so any contact or slight distance qualifies.
    auto* vehicleColliderComp = owner.getComponent<ColliderComponent>();
    if (!vehicleColliderComp) return false;
    vehicleColliderComp->ensureCollider(owner);
    auto* vehicleCollider = vehicleColliderComp->collider();
    if (!vehicleCollider) return false;
    const AABB vehicleBounds = vehicleCollider->getAABB().expanded(m_mountRadius);

    const auto* riderTransform = m_rider->getComponent<TransformComponent>();
    if (!riderTransform) return false;
    const glm::vec2 riderPos = riderTransform->getTransform().Position;
    return vehicleBounds.contains(riderPos);
}

VehicleController* VehicleMountComponent::resolveVehicleController(Entity& owner) const {
    auto* ctrlComp = owner.getComponent<ControllerComponent>();
    if (!ctrlComp) return nullptr;
    return dynamic_cast<VehicleController*>(ctrlComp->controller());
}

void VehicleMountComponent::drawRadius(Entity& owner) const {
    if (!m_debugCamera) return;
    auto* vehicleTransform = owner.getComponent<TransformComponent>();
    if (!vehicleTransform) return;

    const glm::vec2 center = vehicleTransform->getTransform().Position + m_seatOffset;
    const glm::mat4 viewProj = m_debugCamera->getViewProjection();
    glUseProgram(0);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(&viewProj[0][0]);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glColor3f(0.2f, 0.9f, 0.6f);
    glLineWidth(1.5f);
    const int segments = 48;
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; ++i) {
        const float theta = (static_cast<float>(i) / segments) * 2.0f * 3.1415926f;
        const float x = center.x + std::cos(theta) * m_mountRadius;
        const float y = center.y + std::sin(theta) * m_mountRadius;
        glVertex2f(x, y);
    }
    glEnd();
}

void VehicleMountComponent::update(Entity& owner, double /*dt*/) {
    auto* vehicleController = resolveVehicleController(owner);
    if (!vehicleController || !m_rider) {
        return;
    }

    vehicleController->setSeatOffset(m_seatOffset);
    if (m_mounted && !vehicleController->isMounted()) {
        m_mounted = false;
    }

    const bool close = riderIsClose(owner);
    if (close) {
        drawRadius(owner);
    }

    if (isInteractPressedThisFrame()) {
        if (!m_mounted && riderIsClose(owner)) {
            vehicleController->mount(*m_rider);
            m_mounted = true;
        } else if (m_mounted) {
            vehicleController->dismount();
            m_mounted = false;
        }
    }
}
