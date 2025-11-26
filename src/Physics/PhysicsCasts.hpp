#pragma once

#include <glm/vec2.hpp>
#include <memory>
#include <vector>

class Entity;
class ACollider;
class AABB;

namespace PhysicsCasts {

struct CastFilter {
    // Skip a specific entity (often the caller) and control whether triggers are considered.
    const Entity* ignore{nullptr};
    bool includeTriggers{false};
    uint32_t layerMask{0xFFFFFFFFu}; // Bitmask of layers to hit (1<<layer).
};

struct CastHit {
    bool hit{false};
    glm::vec2 point{0.0f};
    glm::vec2 normal{0.0f};
    float distance{0.0f};
    Entity* entity{nullptr};
    ACollider* collider{nullptr};
};

// Cast a ray and return the closest hit.
CastHit rayCast(const glm::vec2& origin,
                const glm::vec2& direction,
                float maxDistance,
                const std::vector<std::unique_ptr<Entity>>& entities,
                CastFilter filter = {});

// Sweep an axis-aligned box (AABB) from its current position along a direction.
CastHit boxCast(const AABB& box,
                const glm::vec2& direction,
                float maxDistance,
                const std::vector<std::unique_ptr<Entity>>& entities,
                CastFilter filter = {});

// Sweep a capsule defined by world-space endpoints and radius.
CastHit capsuleCast(const glm::vec2& a,
                    const glm::vec2& b,
                    float radius,
                    const glm::vec2& direction,
                    float maxDistance,
                    const std::vector<std::unique_ptr<Entity>>& entities,
                    CastFilter filter = {});

// Find all colliders overlapping a circle.
std::vector<CastHit> overlapCircle(const glm::vec2& center,
                                   float radius,
                                   const std::vector<std::unique_ptr<Entity>>& entities,
                                   CastFilter filter = {});

} // namespace PhysicsCasts
