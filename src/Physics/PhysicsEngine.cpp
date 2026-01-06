//
// PhysicsEngine.cpp
//

#include "PhysicsEngine.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Components/HingeComponent.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "GameObjects/Entity.hpp"
#include "Physics/Collision/ACollider.hpp"
#include "Physics/Collision/CollisionDispatcher.hpp"
#include "Physics/RigidBody.hpp"
#include "Physics/Quadtree.hpp"

#include <unordered_map>
namespace {
bool isTrigger(const ACollider* c) {
    return c && c->isTrigger();
}

float normalizeAngle(float angle) {
    const float twoPi = glm::two_pi<float>();
    angle = std::fmod(angle, twoPi);
    if (angle <= -glm::pi<float>()) {
        angle += twoPi;
    } else if (angle > glm::pi<float>()) {
        angle -= twoPi;
    }
    return angle;
}

glm::vec2 rotateLocal(const glm::vec2& vec, float angle) {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    return glm::vec2{vec.x * c - vec.y * s, vec.x * s + vec.y * c};
}
} // namespace

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

    std::unordered_map<Entity*, BodyEntry*> entryMap;
    entryMap.reserve(m_entries.size());
    for (auto& entry : m_entries) {
        if (entry.entity) {
            entryMap[entry.entity] = &entry;
        }
    }

    m_hingeEntries.clear();
    for (const auto& ePtr : entities) {
        if (!ePtr) continue;
        auto* hinge = ePtr->getComponent<HingeComponent>();
        if (!hinge || !hinge->isEnabled()) continue;
        Entity* target = hinge->target();
        if (!target) continue;
        const auto selfIt = entryMap.find(ePtr.get());
        const auto targetIt = entryMap.find(target);
        if (selfIt == entryMap.end() || targetIt == entryMap.end()) {
            continue;
        }
        m_hingeEntries.push_back(HingeEntry{
            selfIt->second->body,
            targetIt->second->body,
            hinge->anchorSelf(),
            hinge->anchorTarget(),
            hinge->referenceAngle(),
            hinge->limitsEnabled(),
            hinge->lowerLimit(),
            hinge->upperLimit(),
            hinge->limitStiffness(),
            hinge->limitDamping(),
            hinge->maxLimitTorque(),
            hinge->motorEnabled(),
            hinge->motorSpeed(),
            hinge->motorStiffness(),
            hinge->maxMotorTorque()
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
    // Build bounds from colliders.
    glm::vec2 minBounds{std::numeric_limits<float>::max()};
    glm::vec2 maxBounds{-std::numeric_limits<float>::max()};
    bool hasCollider = false;
    for (const auto& e : m_entries) {
        if (!e.collider) continue;
        const AABB aabb = e.collider->getAABB();
        minBounds.x = std::min(minBounds.x, aabb.getMin().x);
        minBounds.y = std::min(minBounds.y, aabb.getMin().y);
        maxBounds.x = std::max(maxBounds.x, aabb.getMax().x);
        maxBounds.y = std::max(maxBounds.y, aabb.getMax().y);
        hasCollider = true;
    }
    if (!hasCollider) {
        return;
    }
    // Ensure non-degenerate bounds.
    const glm::vec2 extent = maxBounds - minBounds;
    if (extent.x < 1.0f) {
        minBounds.x -= 0.5f;
        maxBounds.x += 0.5f;
    }
    if (extent.y < 1.0f) {
        minBounds.y -= 0.5f;
        maxBounds.y += 0.5f;
    }

    Quadtree::Config cfg{};
    cfg.maxDepth = 6;
    cfg.maxObjectsPerNode = 6;
    cfg.minSize = 1.0f;
    Quadtree tree(AABB(minBounds, maxBounds), cfg);

    for (auto& e : m_entries) {
        if (e.collider) {
            tree.insert(e.collider->getAABB(), &e);
        }
    }

    std::vector<void*> candidates;
    candidates.reserve(16);

    for (auto& a : m_entries) {
        if (!a.collider) continue;
        candidates.clear();
        tree.query(a.collider->getAABB(), candidates);
        for (void* cand : candidates) {
            auto* b = static_cast<BodyEntry*>(cand);
            if (!b || &a >= b) continue; // ensure each pair once
            if (!b->collider) continue;

            auto hit = CollisionDispatcher::dispatch(*a.collider, *b->collider);
            if (!hit || !hit->collided || hit->penetration <= 0.0f) continue;

            const bool triggerOnly = isTrigger(a.collider) || isTrigger(b->collider);
            if (triggerOnly) {
                // Triggers handled separately by TriggerSystem.
                continue;
            }

            const float invMassA = a.body ? a.body->getInvMass() : 0.0f;
            const float invMassB = b->body ? b->body->getInvMass() : 0.0f;
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
            if (b->body) {
                const float factor = invMassB / totalInvMass;
                b->body->setPosition(b->body->getPosition() - separation * factor);
                auto vel = b->body->getVelocity();
                const float vn = glm::dot(vel, hit->normal);
                if (vn > 0.0f) {
                    vel -= hit->normal * vn;
                    b->body->setVelocity(vel);
                }
            }
        }
    }
}

void PhysicsEngine::step(float dt, const std::vector<std::unique_ptr<Entity>> &entities) {
    gather(entities);
    integrateBodies(dt);
    resolveHinges();
    resolveCollisions();
}

void PhysicsEngine::resolveHinges() {
    constexpr float kMinDistance = 1e-4f;
    for (const auto& hinge : m_hingeEntries) {
        if (!hinge.bodyA && !hinge.bodyB) {
            continue;
        }
        const float angleA = hinge.bodyA ? hinge.bodyA->getRotation() : 0.0f;
        const float angleB = hinge.bodyB ? hinge.bodyB->getRotation() : 0.0f;
        const glm::vec2 anchorA = hinge.bodyA ? hinge.bodyA->getPosition() + rotateLocal(hinge.anchorA, angleA) : glm::vec2{0.0f};
        const glm::vec2 anchorB = hinge.bodyB ? hinge.bodyB->getPosition() + rotateLocal(hinge.anchorB, angleB) : glm::vec2{0.0f};
        glm::vec2 delta = anchorB - anchorA;
        const float sqrDist = glm::dot(delta, delta);
        const float dist = std::sqrt(sqrDist);
        const float invMassA = hinge.bodyA ? hinge.bodyA->getInvMass() : 0.0f;
        const float invMassB = hinge.bodyB ? hinge.bodyB->getInvMass() : 0.0f;
        const float totalInvMass = invMassA + invMassB;
        if (totalInvMass > 0.0f && dist > kMinDistance) {
            const glm::vec2 correctionDir = delta / dist;
            const float weightA = invMassA / totalInvMass;
            const float weightB = invMassB / totalInvMass;
            if (hinge.bodyA) {
                hinge.bodyA->setPosition(hinge.bodyA->getPosition() + correctionDir * dist * weightA);
            }
            if (hinge.bodyB) {
                hinge.bodyB->setPosition(hinge.bodyB->getPosition() - correctionDir * dist * weightB);
            }

            glm::vec2 relVel{0.0f};
            if (hinge.bodyA) {
                relVel += hinge.bodyA->getVelocity();
            }
            if (hinge.bodyB) {
                relVel -= hinge.bodyB->getVelocity();
            }
            const float velocityAlongAxis = glm::dot(relVel, correctionDir);
            if (velocityAlongAxis != 0.0f) {
                const float impulse = velocityAlongAxis / totalInvMass;
                if (hinge.bodyA) {
                    hinge.bodyA->setVelocity(hinge.bodyA->getVelocity() - correctionDir * (impulse * invMassA));
                }
                if (hinge.bodyB) {
                    hinge.bodyB->setVelocity(hinge.bodyB->getVelocity() + correctionDir * (impulse * invMassB));
                }
            }
        }

        const float relAngle = normalizeAngle(angleB - angleA - hinge.referenceAngle);
        const float relAngVel = (hinge.bodyB ? hinge.bodyB->getAngularVelocity() : 0.0f) -
                                 (hinge.bodyA ? hinge.bodyA->getAngularVelocity() : 0.0f);
        const float invInertiaA = hinge.bodyA ? hinge.bodyA->getInvInertia() : 0.0f;
        const float invInertiaB = hinge.bodyB ? hinge.bodyB->getInvInertia() : 0.0f;
        const float invInertiaSum = invInertiaA + invInertiaB;
        const auto applyAngularImpulse = [&](float torque) {
            if (invInertiaSum <= 0.0f) {
                return;
            }
            const float angularImpulse = torque / invInertiaSum;
            if (hinge.bodyA) {
                hinge.bodyA->setAngularVelocity(hinge.bodyA->getAngularVelocity() -
                                                 angularImpulse * invInertiaA);
            }
            if (hinge.bodyB) {
                hinge.bodyB->setAngularVelocity(hinge.bodyB->getAngularVelocity() +
                                                 angularImpulse * invInertiaB);
            }
        };

        if (hinge.limitsEnabled && invInertiaSum > 0.0f) {
            float limitError = 0.0f;
            if (relAngle < hinge.lowerLimit) {
                limitError = hinge.lowerLimit - relAngle;
            } else if (relAngle > hinge.upperLimit) {
                limitError = hinge.upperLimit - relAngle;
            }
            if (limitError != 0.0f) {
                float torque = hinge.limitStiffness * limitError - hinge.limitDamping * relAngVel;
                torque = glm::clamp(torque, -hinge.maxLimitTorque, hinge.maxLimitTorque);
                applyAngularImpulse(torque);
            }
        }

        if (hinge.motorEnabled && invInertiaSum > 0.0f) {
            float torque = hinge.motorStiffness * (hinge.motorSpeed - relAngVel);
            torque = glm::clamp(torque, -hinge.maxMotorTorque, hinge.maxMotorTorque);
            applyAngularImpulse(torque);
        }
    }
}
