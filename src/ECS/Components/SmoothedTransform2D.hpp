#pragma once

#include "ECS/Components/Transform2D.hpp"

#include <glm/glm.hpp>

namespace ECS {

// Opt-in render smoothing. Entities carrying this tag have their Transform2D
// snapshotted at the start of every fixed simulation step, and render
// extraction draws the pose interpolated by the fixed-step clock's alpha —
// removing sim-rate stepping from slow cinematic movement. Do not add it to
// entities whose transform is written per render frame (e.g. parallax
// layers); they are already frame-smooth.
struct SmoothedTransform2D {};

// Maintained by the Scene for entities tagged SmoothedTransform2D.
struct PreviousTransform2D {
    Transform2D value{};
};

// Pure interpolation used by render extraction (rotation is lerped naively;
// smoothed entities are expected to rotate less than a half-turn per step).
[[nodiscard]] inline Transform2D interpolatedTransform2D(
    const Transform2D& previous, const Transform2D& current, float alpha) {
    Transform2D result{};
    result.position = glm::mix(previous.position, current.position, alpha);
    result.scale = glm::mix(previous.scale, current.scale, alpha);
    result.rotationDegrees = previous.rotationDegrees +
        (current.rotationDegrees - previous.rotationDegrees) * alpha;
    return result;
}

} // namespace ECS
