#pragma once

#include "GameObjects/Texture.hpp"
#include "RenderingSystem/ParticleBlendMode.hpp"

#include <glm/vec4.hpp>

#include <memory>

namespace ECS {

// Presentation is separate from simulation so headless emitters do not carry
// render-only state and effects can be hidden without pausing their lifetime.
struct ParticleRender2D {
    std::shared_ptr<GameObjects::Texture> texture{};
    glm::vec4 tint{1.0f};
    Rendering::ParticleBlendMode blendMode{
        Rendering::ParticleBlendMode::Alpha};
    int order{0};
    bool visible{true};
};

} // namespace ECS
