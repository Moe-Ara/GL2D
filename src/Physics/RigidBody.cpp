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

void RigidBody::setTransform(Transform *transform) {
    m_transform = transform;
    if (m_transform) {
        m_position = m_transform->Position;
        m_rotation = glm::radians(m_transform->Rotation);
    }
    if (m_collider) {
        m_collider->setTransform(m_transform);
    }
}

void RigidBody::setCollider(ICollider *collider) {
    m_collider = collider;
    if (m_collider && m_transform) {
        m_collider->setTransform(m_transform);
    }
}

void RigidBody::setPosition(const glm::vec2 &pos) {
    m_position = pos;
    if (m_transform) {
        m_transform->setPos(pos);
    }
    if (m_collider) {
        m_collider->setTransform(m_transform);
    }
}

void RigidBody::setRotation(float radians) {
    m_rotation = radians;
    if (m_transform) {
        m_transform->setRotation(glm::degrees(radians));
    }
    if (m_collider) {
        m_collider->setTransform(m_transform);
    }
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

void RigidBody::setInertia(float inertia) {
    m_inertia = inertia;
    if (inertia > 0.0f) {
        m_invInertia = 1.0f / inertia;
    } else {
        m_invInertia = 0.0f;
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

    m_angularVelocity += m_torque * m_invInertia * dt;
    m_angularVelocity *= std::max(0.0f, 1.0f - m_angularDamping * dt);
    m_rotation += m_angularVelocity * dt;
    m_torque = 0.0f;

    // Push to transform and collider if present.
    if (m_transform) {
        m_transform->setPos(m_position);
        m_transform->setRotation(glm::degrees(m_rotation));
    }
    if (m_collider) {
        m_collider->setTransform(m_transform);
    }
}
