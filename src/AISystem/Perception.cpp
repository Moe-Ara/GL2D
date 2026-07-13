#include "Perception.hpp"

#include <cmath>
#include <stdexcept>

#include "Physics/PhysicsCasts.hpp"
#include "GameObjects/Entity.hpp"

namespace AI {

bool hasLineOfSight(const glm::vec2& from,
                    const glm::vec2& to,
                    const std::vector<std::unique_ptr<Entity>>& entities,
                    uint32_t layerMask,
                    const Entity* ignore,
                    const Entity* target) {
    if (!std::isfinite(from.x) || !std::isfinite(from.y) ||
        !std::isfinite(to.x) || !std::isfinite(to.y)) {
        throw std::invalid_argument("Line-of-sight endpoints must be finite");
    }
    const glm::vec2 dir = to - from;
    const float dist = glm::length(dir);
    if (dist < 1e-4f) {
        return true;
    }
    auto hit = PhysicsCasts::rayCast(from, dir / dist, dist, entities,
                                     PhysicsCasts::CastFilter{.ignore = ignore, .includeTriggers = false, .layerMask = layerMask});
    // A target collider is visible when it is the first collider reached. For a
    // bare point query, a hit effectively at the endpoint is also unobstructed.
    return !hit.hit || (target && hit.entity == target) || hit.distance >= dist - 1e-3f;
}

bool canHear(const glm::vec2& listener,
             float radius,
             const std::vector<std::unique_ptr<Entity>>& entities,
             uint32_t layerMask,
             const Entity* ignore,
             std::vector<Entity*>* outHeard) {
    if (outHeard) outHeard->clear();
    if (!std::isfinite(listener.x) || !std::isfinite(listener.y) ||
        !std::isfinite(radius) || radius < 0.0f) {
        throw std::invalid_argument("Hearing position and radius must be finite; radius cannot be negative");
    }
    if (radius == 0.0f) return false;
    auto hits = PhysicsCasts::overlapCircle(listener, radius, entities,
                                            PhysicsCasts::CastFilter{.ignore = ignore, .includeTriggers = false, .layerMask = layerMask});
    if (outHeard) {
        for (const auto& h : hits) {
            if (h.entity && h.entity != ignore) {
                outHeard->push_back(h.entity);
            }
        }
    }
    return !hits.empty();
}

} // namespace AI
