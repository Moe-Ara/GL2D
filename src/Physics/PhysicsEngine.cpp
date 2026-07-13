//
// PhysicsEngine.cpp
//

#include "PhysicsEngine.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Components/HingeComponent.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "GameObjects/Entity.hpp"
#include "Physics/Collision/ACollider.hpp"
#include "Physics/Collision/CollisionDispatcher.hpp"
#include "Physics/BroadphaseBVH.hpp"
#include "Physics/PhysicsUnits.hpp"
#include "Physics/RigidBody.hpp"

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
        : m_gravity(0.0f) {
    setGravity(gravity);
}

void PhysicsEngine::setGravity(const glm::vec2& gravity) {
    if (!std::isfinite(gravity.x) || !std::isfinite(gravity.y)) {
        throw std::invalid_argument("Physics gravity must be finite");
    }
    m_gravity = gravity;
}

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

    m_entryLookup.clear();
    m_entryLookup.reserve(m_entries.size());
    for (auto& entry : m_entries) {
        if (entry.entity) {
            m_entryLookup[entry.entity] = &entry;
        }
    }

    m_hingeEntries.clear();
    for (const auto& ePtr : entities) {
        if (!ePtr) continue;
        const auto selfIt = m_entryLookup.find(ePtr.get());
        if (selfIt == m_entryLookup.end()) {
            continue;
        }
        for (const auto& component : ePtr->components()) {
            auto* hinge = dynamic_cast<HingeComponent*>(component.get());
            if (!hinge || !hinge->isEnabled() || !hinge->target()) {
                continue;
            }
            const auto targetIt = m_entryLookup.find(hinge->target());
            if (targetIt == m_entryLookup.end()) {
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
}

void PhysicsEngine::integrateBodies(
        float dt,
        const std::vector<glm::vec2>& stepForces,
        const std::vector<float>& stepTorques) {
    for (std::size_t i = 0; i < m_entries.size(); ++i) {
        auto& entry = m_entries[i];
        if (!entry.body) continue;
        entry.body->integratePrepared(
            dt, RigidBody::StepLoads{stepForces[i], stepTorques[i]});
    }
}

unsigned PhysicsEngine::determineSubsteps(
        float dt, const std::vector<glm::vec2>& stepForces) const {
    constexpr unsigned maxSubsteps = 64;
    constexpr float minimumFeatureSize = 0.01f;
    unsigned required = 1;

    for (std::size_t i = 0; i < m_entries.size(); ++i) {
        const BodyEntry& entry = m_entries[i];
        if (!entry.body || !entry.collider ||
            entry.body->getBodyType() != RigidBodyType::DYNAMIC ||
            entry.body->getCollisionDetection() == CollisionDetection::DISCRETE) {
            continue;
        }

        const AABB bounds = entry.collider->getAABB();
        const float featureSize = std::max(
            minimumFeatureSize, std::min(bounds.width(), bounds.height()));
        const glm::vec2 acceleration =
            stepForces[i] * entry.body->getInvMass();
        const float travel = glm::length(entry.body->getVelocity()) * dt +
                             0.5f * glm::length(acceleration) * dt * dt;
        const float maximumTravelPerStep = featureSize * 0.5f;
        const unsigned bodySteps = static_cast<unsigned>(
            std::ceil(travel / maximumTravelPerStep));
        required = std::max(required, std::min(bodySteps, maxSubsteps));
    }
    return required;
}

void PhysicsEngine::resolveCollisions() {
    m_broadphaseScratch.clear();
    m_broadphaseScratch.reserve(m_entries.size());
    for (auto& entry : m_entries) {
        if (entry.collider) {
            m_broadphaseScratch.push_back(
                {entry.collider->getAABB(), &entry});
        }
    }
    if (m_broadphaseScratch.size() < 2) {
        return;
    }

    m_broadphase.build(m_broadphaseScratch);
    m_pairScratch.clear();
    m_broadphase.overlappingPairs(m_pairScratch);
    for (const BroadphaseBVH::Pair& pair : m_pairScratch) {
            auto* a = static_cast<BodyEntry*>(pair.first);
            auto* b = static_cast<BodyEntry*>(pair.second);
            if (!a || !b || !a->collider || !b->collider) continue;

            // Trigger overlap lifetime and one-shot state belong exclusively to
            // TriggerSystem; the impulse solver must remain side-effect free.
            if (isTrigger(a->collider) || isTrigger(b->collider)) {
                continue;
            }

            auto hit = CollisionDispatcher::dispatch(*a->collider, *b->collider);
            if (!hit || !hit->collided || hit->penetration <= 0.0f) continue;

            const float invMassA = a->body ? a->body->getInvMass() : 0.0f;
            const float invMassB = b->body ? b->body->getInvMass() : 0.0f;
            const float totalInvMass = invMassA + invMassB;
            if (totalInvMass <= 0.0f) {
                continue;
            }

            const glm::vec2 separation = hit->normal * hit->penetration;
            if (a->body) {
                const float factor = invMassA / totalInvMass;
                a->body->setPosition(a->body->getPosition() + separation * factor);
            }
            if (b->body) {
                const float factor = invMassB / totalInvMass;
                b->body->setPosition(b->body->getPosition() - separation * factor);
            }

            const glm::vec2 velocityA = a->body
                ? a->body->getVelocity() : glm::vec2{0.0f};
            const glm::vec2 velocityB = b->body
                ? b->body->getVelocity() : glm::vec2{0.0f};
            const glm::vec2 relativeVelocity = velocityA - velocityB;
            const float relativeNormalVelocity =
                glm::dot(relativeVelocity, hit->normal);
            if (relativeNormalVelocity < 0.0f) {
                // Combined material: max restitution, geometric-mean friction.
                // Below a small approach speed restitution is suppressed so
                // resting contacts settle instead of buzzing.
                constexpr float kRestitutionVelocityThreshold =
                    PhysicsUnits::toUnits(0.5f);
                const float frictionA = a->body ? a->body->getFriction() : 0.0f;
                const float frictionB = b->body ? b->body->getFriction() : 0.0f;
                const float friction = std::sqrt(frictionA * frictionB);
                const float restitutionA =
                    a->body ? a->body->getRestitution() : 0.0f;
                const float restitutionB =
                    b->body ? b->body->getRestitution() : 0.0f;
                float restitution = std::max(restitutionA, restitutionB);
                if (-relativeNormalVelocity < kRestitutionVelocityThreshold) {
                    restitution = 0.0f;
                }

                const float normalImpulseMagnitude =
                    -(1.0f + restitution) * relativeNormalVelocity / totalInvMass;
                const glm::vec2 normalImpulse = hit->normal * normalImpulseMagnitude;
                glm::vec2 postVelocityA = velocityA;
                glm::vec2 postVelocityB = velocityB;
                if (a->body && invMassA > 0.0f) {
                    postVelocityA += normalImpulse * invMassA;
                }
                if (b->body && invMassB > 0.0f) {
                    postVelocityB -= normalImpulse * invMassB;
                }

                if (friction > 0.0f) {
                    // Tangential impulse opposes sliding, clamped by the Coulomb
                    // cone (|jt| <= mu * jn) so it can never add energy.
                    const glm::vec2 tangent =
                        relativeVelocity - hit->normal * relativeNormalVelocity;
                    const float tangentSpeed = glm::length(tangent);
                    if (tangentSpeed > 1e-6f) {
                        const glm::vec2 tangentDir = tangent / tangentSpeed;
                        const float tangentImpulseMagnitude =
                            -glm::dot(relativeVelocity, tangentDir) / totalInvMass;
                        const float maxFriction = friction * normalImpulseMagnitude;
                        const float clampedTangent = std::clamp(
                            tangentImpulseMagnitude, -maxFriction, maxFriction);
                        const glm::vec2 frictionImpulse = tangentDir * clampedTangent;
                        if (a->body && invMassA > 0.0f) {
                            postVelocityA += frictionImpulse * invMassA;
                        }
                        if (b->body && invMassB > 0.0f) {
                            postVelocityB -= frictionImpulse * invMassB;
                        }
                    }
                }

                if (a->body && invMassA > 0.0f) {
                    a->body->setVelocity(postVelocityA);
                }
                if (b->body && invMassB > 0.0f) {
                    b->body->setVelocity(postVelocityB);
                }
            }
    }
}

void PhysicsEngine::step(float dt, const std::vector<std::unique_ptr<Entity>> &entities) {
    if (!std::isfinite(dt) || dt <= 0.0f) {
        throw std::invalid_argument(
            "PhysicsEngine::step requires a positive finite delta time");
    }
    gather(entities);

    m_stepForces.assign(m_entries.size(), glm::vec2{0.0f});
    m_stepTorques.assign(m_entries.size(), 0.0f);
    for (std::size_t i = 0; i < m_entries.size(); ++i) {
        RigidBody* body = m_entries[i].body;
        if (!body) continue;
        const RigidBody::StepLoads loads = body->prepareStep(dt);
        m_stepForces[i] = loads.force;
        m_stepTorques[i] = loads.torque;
        if (body->getBodyType() == RigidBodyType::DYNAMIC &&
            body->getMass() > 0.0f) {
            m_stepForces[i] +=
                m_gravity * body->getMass() * body->getGravityScale();
        }
    }

    const unsigned substeps = determineSubsteps(dt, m_stepForces);
    const float substepDelta = dt / static_cast<float>(substeps);
    constexpr unsigned constraintIterations = 8;
    for (unsigned i = 0; i < substeps; ++i) {
        integrateBodies(substepDelta, m_stepForces, m_stepTorques);
        for (unsigned iteration = 0; iteration < constraintIterations; ++iteration) {
            resolveHinges(substepDelta /
                          static_cast<float>(constraintIterations));
        }
        resolveCollisions();
    }
}

void PhysicsEngine::resolveHinges(float dt) {
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
            const float angularImpulse = torque * dt / invInertiaSum;
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
