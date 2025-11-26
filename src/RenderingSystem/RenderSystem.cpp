//
// Created by Codex on 23/11/2025.
//

#include "RenderSystem.hpp"
#include "Renderer.hpp"
#include "Engine/Scene.hpp"
#include "GameObjects/Components/SpriteComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "Graphics/Camera/Camera.hpp"
#include "GameObjects/Components/ColliderComponent.hpp"
#include "Debug/DebugOverlay.hpp"
#include "GameObjects/Sprite.hpp"
#include "RenderingSystem/TilemapRenderer.hpp"
namespace {
inline bool overlaps(const glm::vec4 &aabb, const glm::vec4 &view) {
    return !(aabb.z < view.x || aabb.x > view.z || aabb.w < view.y ||
             aabb.y > view.w);
}
}

void RenderSystem::renderScene(Scene &scene, Camera &camera,
                               Rendering::Renderer &renderer) {
    const glm::mat4 &viewProj = camera.getViewProjection();
    const glm::vec4 viewBounds =
        camera.getViewBounds(/*paddingFactor=*/1.0f); // expand by half the view size

    renderer.beginFrame(viewProj, {0.05f, 0.05f, 0.08f, 1.0f}, true);

    Rendering::TilemapRenderer::render(scene, camera, viewProj);

    for (auto &entityPtr : scene.getEntities()) {
        if (!entityPtr) continue;
        auto *spriteComp = entityPtr->getComponent<SpriteComponent>();
        auto *transformComp = entityPtr->getComponent<TransformComponent>();
        if (!spriteComp || !transformComp) continue;

        auto &transform = transformComp->getTransform();
        const auto baseSize = spriteComp->sprite() ? spriteComp->sprite()->getSize() : glm::vec2(0.0f);
        const glm::vec2 scaledSize = baseSize * transform.Scale;
        const glm::vec2 minPt = transform.Position + glm::min(scaledSize, glm::vec2(0.0f));
        const glm::vec2 maxPt = transform.Position + glm::max(scaledSize, glm::vec2(0.0f));
        const glm::vec4 spriteAabb(minPt.x, minPt.y, maxPt.x, maxPt.y);

        if (!overlaps(spriteAabb, viewBounds)) {
            continue;
        }

        if (auto *sprite = spriteComp->sprite()) {
            renderer.submitSprite(*sprite, transformComp->modelMatrix(),
                                  spriteComp->zIndex());
        }
    }

    renderer.endFrame();

    // Debug: draw collider wireframes when overlay is enabled.
    if (DebugOverlay::enabled()) {
        for (auto &entityPtr : scene.getEntities()) {
            if (!entityPtr) continue;
            auto *colliderComp = entityPtr->getComponent<ColliderComponent>();
            if (!colliderComp) continue;
            colliderComp->debugDraw(viewProj, {1.0f, 0.0f, 0.0f});
        }
    }
}
