#ifndef AI_PERCEPTION_HPP
#define AI_PERCEPTION_HPP

#include <glm/glm.hpp>
#include <vector>
#include <memory>
class Entity;

namespace AI {

// Simple line-of-sight check using physics ray casts and layer masks.
bool hasLineOfSight(const glm::vec2& from,
                    const glm::vec2& to,
                    const std::vector<std::unique_ptr<Entity>>& entities,
                    uint32_t layerMask = 0xFFFFFFFFu,
                    const Entity* ignore = nullptr);

// Hearing: returns entities (if out vector provided) within radius, respecting layer mask and ignoring a specific entity.
bool canHear(const glm::vec2& listener,
             float radius,
             const std::vector<std::unique_ptr<Entity>>& entities,
             uint32_t layerMask = 0xFFFFFFFFu,
             const Entity* ignore = nullptr,
             std::vector<Entity*>* outHeard = nullptr);

} // namespace AI

#endif // AI_PERCEPTION_HPP
