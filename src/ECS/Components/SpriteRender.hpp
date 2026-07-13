#pragma once

#include "RenderingSystem/RenderLayers.hpp"

#include <glm/vec4.hpp>

#include <memory>
#include <utility>

namespace GameObjects {
class Sprite;
class Texture;
}

namespace ECS {

// Render extraction data. The shared sprite is an immutable resource by
// convention; per-instance placement and ordering live in ECS components.
struct SpriteRender {
    SpriteRender() = default;
    explicit SpriteRender(
        std::shared_ptr<GameObjects::Sprite> spriteResource,
        int renderLayer = static_cast<int>(Rendering::RenderLayer::Gameplay),
        int renderZIndex = 0,
        bool isVisible = true)
        : sprite(std::move(spriteResource)), layer(renderLayer),
          zIndex(renderZIndex), visible(isVisible) {}

    std::shared_ptr<GameObjects::Sprite> sprite;
    int layer{static_cast<int>(Rendering::RenderLayer::Gameplay)};
    int zIndex{0};
    bool visible{true};
    glm::vec4 tint{1.0f};
    glm::vec4 animationTint{1.0f};
    bool useCustomUV{false};
    glm::vec4 uvRect{0.0f, 0.0f, 1.0f, 1.0f};
    bool flipX{false};
    std::shared_ptr<GameObjects::Texture> textureOverride;
    std::shared_ptr<GameObjects::Texture> normalTextureOverride;
};

} // namespace ECS
