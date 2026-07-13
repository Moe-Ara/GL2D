#pragma once

#include <glm/vec2.hpp>

#include <cstdint>

namespace ECS {

// Axis-aligned local bounds. Transform rotation is intentionally ignored; use
// purpose-built circle/capsule data when those shape paths are migrated.
struct AabbCollider2D {
    glm::vec2 halfExtents{0.5f};
    glm::vec2 offset{0.0f};
    std::uint32_t categoryBits{1u};
    std::uint32_t maskBits{0xFFFFFFFFu};
};

struct StaticCollider2D {};

struct SurfaceVelocity2D {
    glm::vec2 velocity{0.0f};
};

struct CharacterCollisionState2D {
    bool hitWallLeft{false};
    bool hitWallRight{false};
    bool hitCeiling{false};
    glm::vec2 wallNormal{0.0f};
};

} // namespace ECS
