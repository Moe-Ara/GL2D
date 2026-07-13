#pragma once

#include "ECS/Components/Light2D.hpp"
#include "ECS/Components/Transform2D.hpp"
#include "Graphics/LightingSystem/Light.hpp"

#include <optional>

namespace ECS {

// Converts authored ECS data into the renderer's immutable per-frame light.
// Invalid or non-finite authoring data is rejected before it reaches OpenGL.
[[nodiscard]] std::optional<Light> extractLight(
    const Transform2D& transform, const Light2D& light,
    const LightAnimation2D* animation, double timeSeconds) noexcept;

void applyLightEffector(Light& light, const LightEffector& effector,
                        double timeSeconds) noexcept;

} // namespace ECS
