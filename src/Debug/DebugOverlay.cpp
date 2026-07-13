//
// DebugOverlay.cpp
//

#include "DebugOverlay.hpp"

#include "Engine/Scene.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "Graphics/Camera/Camera.hpp"
#include "Physics/RigidBody.hpp"
#include "RenderingSystem/DebugDraw2D.hpp"
#include "RenderingSystem/Renderer.hpp"
#include <cmath>

bool DebugOverlay::s_enabled = false;

void DebugOverlay::render(const Scene& scene,
                          const Camera& camera,
                          Rendering::Renderer& renderer) {
    if (!s_enabled) {
        return;
    }

    const glm::vec4 viewBounds = camera.getViewBounds(/*paddingFactor=*/1.0f);
    const float spacing = 120.0f;
    const glm::vec2 minPoint(std::floor(viewBounds.x / spacing) * spacing, std::floor(viewBounds.y / spacing) * spacing);
    const glm::vec2 maxPoint(std::ceil(viewBounds.z / spacing) * spacing, std::ceil(viewBounds.w / spacing) * spacing);

    const int zIndex = 10000;
    const float lineThickness = 2.0f;
    const glm::vec4 gridColor(0.18f, 0.18f, 0.26f, 0.6f);
    for (float x = minPoint.x; x <= maxPoint.x; x += spacing) {
        Rendering::DebugDraw2D::line(renderer, {x, minPoint.y}, {x, maxPoint.y},
                                     lineThickness, gridColor, zIndex);
    }
    for (float y = minPoint.y; y <= maxPoint.y; y += spacing) {
        Rendering::DebugDraw2D::line(renderer, {minPoint.x, y}, {maxPoint.x, y},
                                     lineThickness, gridColor, zIndex);
    }

    const glm::vec4 axisColor(0.4f, 0.6f, 0.9f, 0.9f);
    const glm::vec4 frustumColor(0.95f, 0.15f, 0.15f, 0.95f);
    Rendering::DebugDraw2D::line(renderer, {viewBounds.x, 0.0f}, {viewBounds.z, 0.0f},
                                 lineThickness, axisColor, zIndex);
    Rendering::DebugDraw2D::line(renderer, {0.0f, viewBounds.y}, {0.0f, viewBounds.w},
                                 lineThickness, axisColor, zIndex);
    Rendering::DebugDraw2D::point(renderer, {0.0f, 0.0f}, 6.0f, axisColor, zIndex);

    Rendering::DebugDraw2D::rectangle(renderer,
        {viewBounds.x, viewBounds.y}, {viewBounds.z, viewBounds.w},
        lineThickness, frustumColor, zIndex);

    const glm::vec2 overlayTopLeft(viewBounds.x + 20.0f, viewBounds.w - 20.0f);
    const float overlayWidth = 220.0f;
    const float overlayHeight = 60.0f;
    const glm::vec2 overlayBottomRight(overlayTopLeft.x + overlayWidth,
                                       overlayTopLeft.y - overlayHeight);
    const glm::vec4 overlayColor(0.95f, 0.75f, 0.2f, 0.9f);
    Rendering::DebugDraw2D::rectangle(renderer, overlayTopLeft, overlayBottomRight,
                                      lineThickness, overlayColor, zIndex);
    Rendering::DebugDraw2D::line(renderer,
        {overlayTopLeft.x + 12.0f, overlayTopLeft.y - 12.0f},
        {overlayTopLeft.x + 24.0f, overlayTopLeft.y - 12.0f},
        lineThickness, overlayColor, zIndex);
    Rendering::DebugDraw2D::line(renderer,
        {overlayTopLeft.x + 18.0f, overlayTopLeft.y - 18.0f},
        {overlayTopLeft.x + 18.0f, overlayTopLeft.y - 6.0f},
        lineThickness, overlayColor, zIndex);

    const glm::vec4 velocityColor(0.3f, 0.9f, 0.4f, 0.9f);
    for (const auto &entityPtr : scene.getEntities()) {
        if (!entityPtr) continue;
        if (auto *rigidBody = entityPtr->getComponent<RigidBodyComponent>()) {
            if (auto *body = rigidBody->body()) {
                const glm::vec2 pos = body->getPosition();
                const glm::vec2 vel = body->getVelocity();
                if (glm::dot(vel, vel) < 0.0001f) {
                    continue;
                }
                const glm::vec2 tip = pos + vel;
                Rendering::DebugDraw2D::arrow(renderer, pos, tip, lineThickness,
                                              velocityColor, zIndex);
            }
        }
    }
}
