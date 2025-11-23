//
// Created by Mohamad on 23/11/2025.
//

#include "RigidBody.hpp"

#include <algorithm>
#include <functional>
#include <vector>

RigidBody::RigidBody(float m, RigidBodyType type)
    : m_mass(m), m_bodyType(type) {
    setBodyType(type);
    setMass(m);
}

void RigidBody::setMass(float mass) {
    m_mass = mass;
    if (m_bodyType == RigidBodyType::DYNAMIC && mass > 0.0f) {
        m_invMass = 1.0f / mass;
    } else {
        m_invMass = 0.0f;
    }
}

void RigidBody::setBodyType(RigidBodyType type) {
    m_bodyType = type;
    if (type != RigidBodyType::DYNAMIC) {
        m_invMass = 0.0f;
    } else if (m_mass > 0.0f) {
        m_invMass = 1.0f / m_mass;
    }
}

void RigidBody::addForceGenerator(std::function<bool(RigidBody &, float)> generator) {
    m_forceGenerators.push_back(std::move(generator));
}

void RigidBody::clearForceGenerators() {
    m_forceGenerators.clear();
}

void RigidBody::integrate(float dt) {
    if (m_bodyType != RigidBodyType::DYNAMIC) {
        m_forces = glm::vec2{0.0f};
        return;
    }

    m_forceGenerators.erase(
        std::remove_if(m_forceGenerators.begin(), m_forceGenerators.end(),
                       [&](auto &gen) {
                           if (!gen) return true;
                           return !gen(*this, dt);
                       }),
        m_forceGenerators.end());

    const glm::vec2 acceleration = m_forces * m_invMass;
    m_velocity += acceleration * dt;

    // Apply simple linear damping.
    m_velocity *= std::max(0.0f, 1.0f - m_linearDamping * dt);

    m_position += m_velocity * dt;
    m_forces = glm::vec2{0.0f};

    // Push to transform and collider if present.
    if (m_transform) {
        m_transform->setPos(m_position);
    }
    if (m_collider) {
        m_collider->setTransform(m_transform);
    }
}
