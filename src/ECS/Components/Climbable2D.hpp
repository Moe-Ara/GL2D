#pragma once

#include "ECS/Entity.hpp"

#include <glm/vec2.hpp>

namespace ECS {

// Marks a non-solid AABB volume as climbable. Axis is expressed in world space
// and allows ladders, vines, and angled climb paths without separate systems.
struct Climbable2D {
    glm::vec2 axis{0.0f, 1.0f};
    bool snapToAxis{true};
};

struct ClimbingState2D {
    Entity climbable{};
    glm::vec2 axis{0.0f, 1.0f};
    float lateralError{0.0f};
    bool active{false};
    bool startedThisStep{false};
    bool endedThisStep{false};
    bool jumpedOffThisStep{false};
    bool requireInputRelease{false};
};

} // namespace ECS
