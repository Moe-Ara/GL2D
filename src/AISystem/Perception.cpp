#include "Perception.hpp"

#include "Physics/PhysicsCasts.hpp"
#include "GameObjects/Entity.hpp"

namespace AI {

bool hasLineOfSight(const glm::vec2& from,
                    const glm::vec2& to,
                    const std::vector<std::unique_ptr<Entity>>& entities,
                    uint32_t layerMask,
                    const Entity* ignore) {
    const glm::vec2 dir = to - from;
    const float dist = glm::length(dir);
    if (dist < 1e-4f) {
        return true;
    }
    auto hit = PhysicsCasts::rayCast(from, dir / dist, dist, entities,
                                     PhysicsCasts::CastFilter{.ignore = ignore, .includeTriggers = false, .layerMask = layerMask});
    // LOS if nothing hit or the hit point is effectively at the target.
    return !hit.hit || hit.distance >= dist - 1e-3f;
}

bool canHear(const glm::vec2& listener,
             float radius,
             const std::vector<std::unique_ptr<Entity>>& entities,
             uint32_t layerMask,
             const Entity* ignore,
             std::vector<Entity*>* outHeard) {
    if (radius <= 0.0f) return false;
    auto hits = PhysicsCasts::overlapCircle(listener, radius, entities,
                                            PhysicsCasts::CastFilter{.ignore = ignore, .includeTriggers = false, .layerMask = layerMask});
    if (outHeard) {
        outHeard->clear();
        for (const auto& h : hits) {
            if (h.entity && h.entity != ignore) {
                outHeard->push_back(h.entity);
            }
        }
    }
    return !hits.empty();
}

} // namespace AI
