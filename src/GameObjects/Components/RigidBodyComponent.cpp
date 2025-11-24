//
// Created by Mohamad on 23/11/2025.
//

#include "RigidBodyComponent.hpp"
#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Entity.hpp"

RigidBodyComponent::RigidBodyComponent(std::unique_ptr<RigidBody> rigidBody)
    : m_body(std::move(rigidBody)) {}

void RigidBodyComponent::setBody(std::unique_ptr<RigidBody> body) {
    m_body = std::move(body);
    m_bound = false;
}

void RigidBodyComponent::ensureBound(Entity &owner) {
    if (!m_bound) {
        bindOwner(owner);
    }
}

void RigidBodyComponent::bindOwner(Entity &owner) {
    if (!m_body || m_bound) {
        return;
    }

    if (auto *transform = owner.getComponent<TransformComponent>()) {
        m_body->setTransform(&transform->getTransform());
    }

    if (auto *colliderComp = owner.getComponent<ColliderComponent>()) {
        colliderComp->ensureCollider(owner);
        m_body->setCollider(colliderComp->collider());
    }
    m_bound = true;
}

void RigidBodyComponent::update(Entity &owner, double dt) {
    if (!m_body) {
        return;
    }

    ensureBound(owner);

    // Integration handled by the physics engine; this component just ensures binding.
}
