#pragma once

#include <glm/vec2.hpp>

namespace ECS {

class Registry;

// Positions every entity that owns a Transform2D and a ParallaxLayer2D so the
// layer scrolls relative to the camera. This is a presentation pass: run it once
// per rendered frame (not per fixed simulation step) just before render
// extraction, using the camera's current center and world-space view bounds.
class ParallaxSystem2D {
public:
    // `cameraCenter` is the camera's world-space focus point. `viewMinX`/
    // `viewMaxX` are the world-space horizontal extents of the visible area,
    // used to wrap tiled layers so they always cover the screen.
    static void update(Registry& registry, const glm::vec2& cameraCenter,
                       float viewMinX, float viewMaxX);
};

} // namespace ECS
