#pragma once

#include <glm/vec2.hpp>

namespace ECS {

// Drives a Transform2D so a background/foreground layer scrolls relative to the
// camera, and optionally tiles horizontally to always cover the view.
//
// `factor` is the fraction of camera travel the layer shows on screen:
//   0        -> pinned to the camera (infinitely far)
//   {1, 1}   -> world-locked geometry (moves exactly with the world)
//   > 1      -> foreground that parallaxes past the world
// The layer's anchor therefore moves with the camera by (1 - factor).
//
// For a tiled layer, author one entity per tile sharing the same `basePosition`,
// `baseCameraCenter`, `tileWidth`, and `tileCount`, with `tileIndex` running
// 0..tileCount-1. The system wraps the strip around the current view so a tile
// scrolling off one edge reappears on the other.
struct ParallaxLayer2D {
    glm::vec2 factor{1.0f};
    glm::vec2 basePosition{0.0f};
    glm::vec2 baseCameraCenter{0.0f};
    float tileWidth{0.0f}; // 0 disables horizontal tiling
    int tileIndex{0};
    int tileCount{1};
};

} // namespace ECS
