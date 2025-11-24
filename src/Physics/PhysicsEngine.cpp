//
// PhysicsEngine.cpp
//

#include "PhysicsEngine.hpp"

#include <algorithm>
#include <glm/glm.hpp>

#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "GameObjects/Entity.hpp"
#include "Physics/Collision/ACollider.hpp"
#include "Physics/Collision/CollisionDispatcher.hpp"
#include "Physics/RigidBody.hpp"

namespace {
bool isTrigger(const ACollider* c) {
    return c && c->isTrigger();
}
}

PhysicsEngine::PhysicsEngine(glm::vec2 gravity)
        : m_gravity(gravity) {}

void PhysicsEngine::gather(const std::vector<std::unique_ptr<Entity>> &entities) {
    m_entries.clear();
    m_entries.reserve(entities.size());

    for (const auto &ePtr: entities) {
        if (!ePtr) continue;
        auto *rbComp = ePtr->getComponent<RigidBodyComponent>();
        if (!rbComp) continue;
        rbComp->ensureBound(*ePtr);
        auto *body = rbComp->body();
        if (!body) continue;

        auto *colliderComp = ePtr->getComponent<ColliderComponent>();
        ACollider *collider = nullptr;
        if (colliderComp) {
            colliderComp->ensureCollider(*ePtr);
            collider = colliderComp->collider();
        }

        m_entries.push_back(BodyEntry{
            ePtr.get(),
            rbComp,
            colliderComp,
            body,
            collider
        });
    }
}

void PhysicsEngine::integrateBodies(float dt) {
    for (auto &entry : m_entries) {
        if (!entry.body) continue;
        if (entry.body->getBodyType() == RigidBodyType::DYNAMIC && entry.body->getMass() > 0.0f) {
            entry.body->applyForce(m_gravity * entry.body->getMass());
        }
        entry.body->integrate(dt);
    }
}

void PhysicsEngine::resolveCollisions() {
    for (size_t i = 0; i < m_entries.size(); ++i) {
        for (size_t j = i + 1; j < m_entries.size(); ++j) {
            auto &a = m_entries[i];
            auto &b = m_entries[j];
            if (!a.collider || !b.collider) continue;

            auto hit = CollisionDispatcher::dispatch(*a.collider, *b.collider);
            if (!hit || !hit->collided || hit->penetration <= 0.0f) continue;

            const bool triggerOnly = isTrigger(a.collider) || isTrigger(b.collider);
            if (triggerOnly) {
                // Triggers handled separately by TriggerSystem.
                continue;
            }

            const float invMassA = a.body ? a.body->getInvMass() : 0.0f;
            const float invMassB = b.body ? b.body->getInvMass() : 0.0f;
            const float totalInvMass = invMassA + invMassB;
            if (totalInvMass <= 0.0f) {
                continue;
            }

            const glm::vec2 separation = hit->normal * hit->penetration;
            if (a.body) {
                const float factor = invMassA / totalInvMass;
                a.body->setPosition(a.body->getPosition() + separation * factor);
                auto vel = a.body->getVelocity();
                const float vn = glm::dot(vel, hit->normal);
                // Remove velocity component driving the bodies together (normal points from B to A).
                if (vn < 0.0f) {
                    vel -= hit->normal * vn;
                    a.body->setVelocity(vel);
                }
            }
            if (b.body) {
                const float factor = invMassB / totalInvMass;
                b.body->setPosition(b.body->getPosition() - separation * factor);
                auto vel = b.body->getVelocity();
                const float vn = glm::dot(vel, hit->normal);
                if (vn > 0.0f) {
                    vel -= hit->normal * vn;
                    b.body->setVelocity(vel);
                }
            }
        }
    }
}

void PhysicsEngine::step(float dt, const std::vector<std::unique_ptr<Entity>> &entities) {
    gather(entities);
    integrateBodies(dt);
    resolveCollisions();
}
