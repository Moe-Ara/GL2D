#include "TransformFollowerComponent.hpp"

#include "GameObjects/Entity.hpp"

void TransformFollowerComponent::update(Entity &owner, double /*dt*/) {
    if (!m_target) {
        return;
    }

    auto *targetTransform = m_target->getComponent<TransformComponent>();
    auto *ownerTransform = owner.getComponent<TransformComponent>();
    if (!targetTransform || !ownerTransform) {
        return;
    }

    const auto &source = targetTransform->getTransform();
    ownerTransform->setPosition(source.Position + m_offset);
    if (m_copyRotation) {
        ownerTransform->setRotation(source.Rotation);
    }
}
