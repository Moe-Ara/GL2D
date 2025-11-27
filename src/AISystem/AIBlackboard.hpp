#ifndef AI_BLACKBOARD_HPP
#define AI_BLACKBOARD_HPP

#include <glm/vec2.hpp>

class Entity;

// Minimal shared state for an AI agent; extend as needed per game.
struct AIBlackboard {
    Entity* target{nullptr};
    glm::vec2 lastSeenPos{0.0f};
    float lastSeenTime{-1.0f};
    bool hasLineOfSight{false};
};

#endif // AI_BLACKBOARD_HPP
