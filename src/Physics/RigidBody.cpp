//
// Created by Mohamad on 23/11/2025.
//

#include "RigidBody.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <vector>

namespace {
bool finite(const glm::vec2& value) {
    return std::isfinite(value.x) && std::isfinite(value.y);
}
}

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
    if (!finite(pos)) {
        throw std::invalid_argument("RigidBody position must be finite");
    }
    m_position = pos;
    if (m_transform) {
        m_transform->setPos(pos);
    }
    if (m_collider) {
        m_collider->setTransform(m_transform);
    }
}

void RigidBody::setRotation(float radians) {
    if (!std::isfinite(radians)) {
        throw std::invalid_argument("RigidBody rotation must be finite");
    }
    m_rotation = radians;
    if (m_transform) {
        m_transform->setRotation(glm::degrees(radians));
    }
    if (m_collider) {
        m_collider->setTransform(m_transform);
    }
}

void RigidBody::setMass(float mass) {
    if (!std::isfinite(mass) || mass < 0.0f ||
        (m_bodyType == RigidBodyType::DYNAMIC && mass == 0.0f)) {
        throw std::invalid_argument(
            "RigidBody mass must be finite and positive for dynamic bodies");
    }
    m_mass = mass;
    updateInverseMassAndInertia();
}

void RigidBody::setBodyType(RigidBodyType type) {
    if (type == RigidBodyType::DYNAMIC && m_mass <= 0.0f) {
        throw std::invalid_argument(
            "Cannot make a zero-mass RigidBody dynamic");
    }
    m_bodyType = type;
    if (type == RigidBodyType::STATIC) {
        m_velocity = {0.0f, 0.0f};
        m_angularVelocity = 0.0f;
    }
    if (type != RigidBodyType::DYNAMIC) {
        m_forces = {0.0f, 0.0f};
        m_torque = 0.0f;
    }
    updateInverseMassAndInertia();
}

void RigidBody::setGravityScale(float scale) {
    if (!std::isfinite(scale) || scale < 0.0f) {
        throw std::invalid_argument(
            "RigidBody gravity scale must be finite and non-negative");
    }
    m_gravityScale = scale;
}

void RigidBody::setFriction(float friction) {
    if (!std::isfinite(friction) || friction < 0.0f) {
        throw std::invalid_argument(
            "RigidBody friction must be finite and non-negative");
    }
    m_friction = friction;
}

void RigidBody::setRestitution(float restitution) {
    if (!std::isfinite(restitution) || restitution < 0.0f || restitution > 1.0f) {
        throw std::invalid_argument(
            "RigidBody restitution must be finite and within [0, 1]");
    }
    m_restitution = restitution;
}

void RigidBody::setLinearDamping(float damping) {
    if (!std::isfinite(damping) || damping < 0.0f) {
        throw std::invalid_argument(
            "RigidBody linear damping must be finite and non-negative");
    }
    m_linearDamping = damping;
}

void RigidBody::setAngularDamping(float damping) {
    if (!std::isfinite(damping) || damping < 0.0f) {
        throw std::invalid_argument(
            "RigidBody angular damping must be finite and non-negative");
    }
    m_angularDamping = damping;
}

void RigidBody::setInertia(float inertia) {
    if (!std::isfinite(inertia) || inertia < 0.0f) {
        throw std::invalid_argument(
            "RigidBody inertia must be finite and non-negative");
    }
    m_inertia = inertia;
    updateInverseMassAndInertia();
}

void RigidBody::updateInverseMassAndInertia() noexcept {
    const bool dynamic = m_bodyType == RigidBodyType::DYNAMIC;
    m_invMass = dynamic && m_mass > 0.0f ? 1.0f / m_mass : 0.0f;
    m_invInertia = dynamic && m_inertia > 0.0f ? 1.0f / m_inertia : 0.0f;
}

void RigidBody::setVelocity(const glm::vec2& velocity) {
    if (!finite(velocity)) {
        throw std::invalid_argument("RigidBody velocity must be finite");
    }
    m_velocity = velocity;
}

void RigidBody::setAngularVelocity(float velocity) {
    if (!std::isfinite(velocity)) {
        throw std::invalid_argument("RigidBody angular velocity must be finite");
    }
    m_angularVelocity = velocity;
}

void RigidBody::applyForce(const glm::vec2& force) {
    if (!finite(force)) {
        throw std::invalid_argument("RigidBody force must be finite");
    }
    m_forces += force;
}

void RigidBody::applyImpulse(const glm::vec2& impulse) {
    if (!finite(impulse)) {
        throw std::invalid_argument("RigidBody impulse must be finite");
    }
    m_velocity += impulse * m_invMass;
}

void RigidBody::applyTorque(float torque) {
    if (!std::isfinite(torque)) {
        throw std::invalid_argument("RigidBody torque must be finite");
    }
    m_torque += torque;
}

void RigidBody::addForceGenerator(std::function<bool(RigidBody &, float)> generator) {
    if (!generator) {
        throw std::invalid_argument("RigidBody force generator cannot be empty");
    }
    m_forceGenerators.push_back(std::move(generator));
}

void RigidBody::clearForceGenerators() {
    m_forceGenerators.clear();
}

void RigidBody::integrate(float dt) {
    if (!std::isfinite(dt) || dt < 0.0f) {
        throw std::invalid_argument(
            "RigidBody::integrate requires finite, non-negative delta time");
    }
    if (dt == 0.0f) {
        return;
    }
    const StepLoads loads = prepareStep(dt);
    integratePrepared(dt, loads);
}

RigidBody::StepLoads RigidBody::prepareStep(float dt) {
    if (m_bodyType == RigidBodyType::STATIC) {
        m_forces = glm::vec2{0.0f};
        m_torque = 0.0f;
        return {};
    }

    if (m_bodyType == RigidBodyType::DYNAMIC) {
        m_forceGenerators.erase(
            std::remove_if(m_forceGenerators.begin(), m_forceGenerators.end(),
                           [&](auto &gen) { return !gen(*this, dt); }),
            m_forceGenerators.end());
        const StepLoads loads{m_forces, m_torque};
        m_forces = glm::vec2{0.0f};
        m_torque = 0.0f;
        return loads;
    }

    m_forces = glm::vec2{0.0f};
    m_torque = 0.0f;
    return {};
}

void RigidBody::integratePrepared(float dt, const StepLoads& loads) {
    if (m_bodyType == RigidBodyType::STATIC || dt == 0.0f) {
        return;
    }

    if (m_bodyType == RigidBodyType::DYNAMIC) {
        const glm::vec2 acceleration = loads.force * m_invMass;
        m_velocity += acceleration * dt;
        m_velocity *= std::exp(-m_linearDamping * dt);
        m_angularVelocity += loads.torque * m_invInertia * dt;
        m_angularVelocity *= std::exp(-m_angularDamping * dt);
    }

    m_position += m_velocity * dt;
    m_rotation += m_angularVelocity * dt;

    // Push to transform and collider if present.
    if (m_transform) {
        m_transform->setPos(m_position);
        m_transform->setRotation(glm::degrees(m_rotation));
    }
    if (m_collider) {
        m_collider->setTransform(m_transform);
    }
}
