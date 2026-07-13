#include "VehicleMountComponent.hpp"

#include <cmath>
#include <stdexcept>

#include "Engine/VehicleController.hpp"
#include "GameObjects/Components/ControllerComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Entity.hpp"
#include "InputSystem/InputTypes.hpp"
#include "Physics/Collision/AABB.hpp"

void VehicleMountComponent::setMountRadius(float radius) {
    if (!std::isfinite(radius) || radius < 0.0f) {
        throw std::invalid_argument("Vehicle mount radius must be finite and non-negative");
    }
    m_mountRadius = radius;
}

void VehicleMountComponent::setSeatOffset(const glm::vec2& offset) {
    if (!std::isfinite(offset.x) || !std::isfinite(offset.y)) {
        throw std::invalid_argument("Vehicle seat offset must be finite");
    }
    m_seatOffset = offset;
}

void VehicleMountComponent::setInteractActionName(const std::string& name) {
    if (name.empty()) {
        throw std::invalid_argument("Vehicle interaction action name cannot be empty");
    }
    m_interactAction = name;
}

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

void VehicleMountComponent::update(Entity& owner, double /*dt*/) {
    auto* vehicleController = resolveVehicleController(owner);
    if (!vehicleController || !m_rider) {
        return;
    }

    vehicleController->setSeatOffset(m_seatOffset);
    if (m_mounted && !vehicleController->isMounted()) {
        m_mounted = false;
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
