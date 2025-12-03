#include "WaterSystem.hpp"

#include <algorithm>
#include <glm/glm.hpp>

#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "GameObjects/Components/WaterStateComponent.hpp"
#include "GameObjects/Components/WaterVolumeComponent.hpp"
#include "GameObjects/Entity.hpp"
#include "Physics/Collision/AABB.hpp"
#include "Physics/Collision/ACollider.hpp"
#include "Physics/RigidBody.hpp"

namespace {
constexpr float kEpsilon = 1e-4f;

glm::vec2 gravityDir(const glm::vec2& gravity) {
    const float len = glm::length(gravity);
    if (len <= kEpsilon) {
        return glm::vec2{0.0f, -1.0f};
    }
    return gravity / len;
}
} // namespace

void WaterSystem::update(float dt,
                         const std::vector<std::unique_ptr<Entity>>& entities,
                         const glm::vec2& gravity) {
    std::vector<VolumeEntry> volumes;
    std::vector<BodyEntry> bodies;
    volumes.reserve(entities.size());
    bodies.reserve(entities.size());

    for (const auto& ePtr : entities) {
        if (!ePtr) continue;

        // Gather water volumes.
        if (auto* water = ePtr->getComponent<WaterVolumeComponent>()) {
            auto* colliderComp = ePtr->getComponent<ColliderComponent>();
            if (!colliderComp) {
                continue;
            }
            colliderComp->setTrigger(true);
            colliderComp->ensureCollider(*ePtr);
            auto* collider = colliderComp->collider();
            if (!collider) {
                continue;
            }
            volumes.push_back(VolumeEntry{
                ePtr.get(),
                water,
                colliderComp,
                collider,
                collider->getAABB()
            });
        }

        // Gather dynamic bodies that can be influenced by water.
        auto* rbComp = ePtr->getComponent<RigidBodyComponent>();
        if (!rbComp) continue;
        rbComp->ensureBound(*ePtr);
        auto* body = rbComp->body();
        if (!body || body->getBodyType() != RigidBodyType::DYNAMIC || body->getMass() <= 0.0f) {
            continue;
        }

        auto* colliderComp = ePtr->getComponent<ColliderComponent>();
        if (!colliderComp) {
            continue;
        }
        colliderComp->ensureCollider(*ePtr);
        auto* collider = colliderComp->collider();
        if (!collider) {
            continue;
        }

        auto* waterState = ePtr->getComponent<WaterStateComponent>();
        bodies.push_back(BodyEntry{
            ePtr.get(),
            rbComp,
            waterState,
            collider,
            collider->getAABB()
        });
    }

    const glm::vec2 gDir = gravityDir(gravity);
    const float gMag = glm::length(gravity);

    for (auto& body : bodies) {
        float maxSubmersion = 0.0f;
        glm::vec2 flowAccum{0.0f};
        float flowWeight = 0.0f;
        float surfaceY = body.bounds.getMax().y;

        for (const auto& volume : volumes) {
            if (!volume.bounds.overlaps(body.bounds)) {
                continue;
            }

            const float submersion = computeSubmersion(volume.bounds, body.bounds);
            if (submersion <= 0.0f) {
                continue;
            }

            maxSubmersion = std::max(maxSubmersion, submersion);
            flowAccum += volume.volume->flowVelocity() * submersion;
            flowWeight += submersion;
            surfaceY = std::max(surfaceY, volume.bounds.getMax().y);
            applyForces(volume, body, submersion, dt, gravity);
        }

        if (body.waterState) {
            glm::vec2 averageFlow = (flowWeight > kEpsilon) ? (flowAccum / flowWeight) : glm::vec2{0.0f};
            body.waterState->setState(maxSubmersion > 0.0f,
                                      maxSubmersion,
                                      averageFlow,
                                      surfaceY,
                                      dt);
        }
    }
}

float WaterSystem::computeSubmersion(const AABB& volume, const AABB& body) {
    if (!volume.overlaps(body)) {
        return 0.0f;
    }

    const float bodyHeight = body.height();
    if (bodyHeight <= kEpsilon) {
        return 0.0f;
    }

    const float overlapTop = std::min(volume.getMax().y, body.getMax().y);
    const float overlapBottom = std::max(volume.getMin().y, body.getMin().y);
    const float overlapHeight = std::max(0.0f, overlapTop - overlapBottom);
    return glm::clamp(overlapHeight / bodyHeight, 0.0f, 1.0f);
}

void WaterSystem::applyForces(const VolumeEntry& volume,
                              BodyEntry& body,
                              float submersion,
                              float /*dt*/,
                              const glm::vec2& gravity) const {
    if (!body.rbComp || !body.rbComp->body()) {
        return;
    }

    auto* rb = body.rbComp->body();
    const float mass = rb->getMass();
    if (mass <= 0.0f) {
        return;
    }

    const glm::vec2 gDir = gravityDir(gravity);
    const float gMag = glm::length(gravity);
    const float density = std::max(volume.volume->density(), 0.0f);
    const glm::vec2 buoyancy = -gDir * gMag * mass * density * submersion;
    rb->applyForce(buoyancy);

    const glm::vec2 vel = rb->getVelocity();
    const glm::vec2 flow = volume.volume->flowVelocity();
    const float drag = volume.volume->linearDrag();
    if (drag > 0.0f && submersion >= volume.volume->minSubmersionForDrag()) {
        const glm::vec2 relVel = vel - flow;
        rb->applyForce(-relVel * drag * mass);
    }

    const float follow = volume.volume->flowFollowStrength();
    if (follow > 0.0f) {
        const glm::vec2 flowForce = (flow - vel) * follow * mass;
        rb->applyForce(flowForce);
    }
}
