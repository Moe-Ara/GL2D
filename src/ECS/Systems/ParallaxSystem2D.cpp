#include "ECS/Systems/ParallaxSystem2D.hpp"

#include "ECS/Components/ParallaxLayer2D.hpp"
#include "ECS/Components/Transform2D.hpp"
#include "ECS/Registry.hpp"

#include <cmath>

namespace ECS {

void ParallaxSystem2D::update(Registry& registry, const glm::vec2& cameraCenter,
                              float viewMinX, float viewMaxX) {
    (void)viewMaxX; // reserved for future vertical/columnar tiling
    registry.each<Transform2D, ParallaxLayer2D>(
        [&](Entity, Transform2D& transform, const ParallaxLayer2D& layer) {
            const glm::vec2 delta = cameraCenter - layer.baseCameraCenter;
            const glm::vec2 anchor =
                layer.basePosition + delta * (glm::vec2{1.0f} - layer.factor);

            float x = anchor.x + layer.tileWidth * static_cast<float>(layer.tileIndex);
            if (layer.tileWidth > 0.0f) {
                // Wrap the strip so tileIndex 0 sits just left of the view and the
                // whole strip covers [viewMinX, viewMaxX] regardless of travel.
                float shift = std::fmod(anchor.x - viewMinX, layer.tileWidth);
                if (shift < 0.0f) {
                    shift += layer.tileWidth;
                }
                const float startX = viewMinX + shift - layer.tileWidth;
                x = startX + layer.tileWidth * static_cast<float>(layer.tileIndex);
            }
            transform.position = glm::vec2{x, anchor.y};
        });
}

} // namespace ECS
