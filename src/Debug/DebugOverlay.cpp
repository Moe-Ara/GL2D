//
// DebugOverlay.cpp
//

#include "DebugOverlay.hpp"

#include "Engine/Scene.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "Graphics/Camera/Camera.hpp"
#include "Physics/RigidBody.hpp"
#include "RenderingSystem/Renderer.hpp"
#include "RenderingSystem/RenderLayers.hpp"
#include "GameObjects/Sprite.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace {
    void submitRect(Rendering::Renderer &renderer,
                    const glm::vec2 &pos,
                    const glm::vec2 &size,
                    const glm::vec4 &color,
                    int zIndex) {
        GameObjects::Sprite sprite(glm::vec2{0.0f}, size, glm::vec3(color));
        sprite.setColor(color);
        glm::mat4 model{1.0f};
        model = glm::translate(model, glm::vec3(pos, 0.0f));
        renderer.submitSprite(sprite, model,
                              static_cast<int>(Rendering::RenderLayer::UI),
                              zIndex);
    }

    void submitLine(Rendering::Renderer &renderer,
                    const glm::vec2 &a,
                    const glm::vec2 &b,
                    float thickness,
                    const glm::vec4 &color,
                    int zIndex) {
        const glm::vec2 delta = b - a;
        const float length = glm::length(delta);
        if (length <= 0.001f) {
            return;
        }
        GameObjects::Sprite sprite(glm::vec2{0.0f}, glm::vec2{length, thickness}, glm::vec3(color));
        sprite.setColor(color);
        const float angle = std::atan2(delta.y, delta.x);
        glm::mat4 model{1.0f};
        model = glm::translate(model, glm::vec3(a, 0.0f));
        model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));
        renderer.submitSprite(sprite, model,
                              static_cast<int>(Rendering::RenderLayer::UI),
                              zIndex);
    }

    void submitArrow(Rendering::Renderer &renderer,
                     const glm::vec2 &from,
                     const glm::vec2 &to,
                     float thickness,
                     const glm::vec4 &color,
                     int zIndex) {
        const glm::vec2 delta = to - from;
        const float length = glm::length(delta);
        if (length <= 0.001f) {
            return;
        }
        const glm::vec2 dir = delta / length;
        submitLine(renderer, from, to, thickness, color, zIndex);

        const glm::vec2 perp{-dir.y, dir.x};
        const float headLen = std::min(length * 0.25f, 18.0f);
        const glm::vec2 tip = to;
        submitLine(renderer, tip, tip - dir * headLen + perp * (headLen * 0.4f), thickness, color, zIndex);
        submitLine(renderer, tip, tip - dir * headLen - perp * (headLen * 0.4f), thickness, color, zIndex);
    }
}

bool DebugOverlay::s_enabled = true;

void DebugOverlay::render(const Scene &scene, Camera &camera, Rendering::Renderer &renderer) {
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
        submitLine(renderer, {x, minPoint.y}, {x, maxPoint.y}, lineThickness, gridColor, zIndex);
    }
    for (float y = minPoint.y; y <= maxPoint.y; y += spacing) {
        submitLine(renderer, {minPoint.x, y}, {maxPoint.x, y}, lineThickness, gridColor, zIndex);
    }

    const glm::vec4 axisColor(0.4f, 0.6f, 0.9f, 0.9f);
    const glm::vec4 frustumColor(0.95f, 0.15f, 0.15f, 0.95f);
    submitLine(renderer, {viewBounds.x, 0.0f}, {viewBounds.z, 0.0f}, lineThickness, axisColor, zIndex);
    submitLine(renderer, {0.0f, viewBounds.y}, {0.0f, viewBounds.w}, lineThickness, axisColor, zIndex);
    submitRect(renderer, {-3.0f, -3.0f}, {6.0f, 6.0f}, axisColor, zIndex);

    submitLine(renderer, {viewBounds.x, viewBounds.y}, {viewBounds.z, viewBounds.y}, lineThickness, frustumColor, zIndex);
    submitLine(renderer, {viewBounds.z, viewBounds.y}, {viewBounds.z, viewBounds.w}, lineThickness, frustumColor, zIndex);
    submitLine(renderer, {viewBounds.z, viewBounds.w}, {viewBounds.x, viewBounds.w}, lineThickness, frustumColor, zIndex);
    submitLine(renderer, {viewBounds.x, viewBounds.w}, {viewBounds.x, viewBounds.y}, lineThickness, frustumColor, zIndex);

    const glm::vec2 overlayTopLeft(viewBounds.x + 20.0f, viewBounds.w - 20.0f);
    const float overlayWidth = 220.0f;
    const float overlayHeight = 60.0f;
    const glm::vec2 overlayBottomRight(overlayTopLeft.x + overlayWidth,
                                       overlayTopLeft.y - overlayHeight);
    const glm::vec4 overlayColor(0.95f, 0.75f, 0.2f, 0.9f);
    submitLine(renderer, overlayTopLeft, {overlayBottomRight.x, overlayTopLeft.y}, lineThickness, overlayColor, zIndex);
    submitLine(renderer, {overlayBottomRight.x, overlayTopLeft.y}, overlayBottomRight, lineThickness, overlayColor, zIndex);
    submitLine(renderer, overlayBottomRight, {overlayTopLeft.x, overlayBottomRight.y}, lineThickness, overlayColor, zIndex);
    submitLine(renderer, {overlayTopLeft.x, overlayBottomRight.y}, overlayTopLeft, lineThickness, overlayColor, zIndex);
    submitLine(renderer, {overlayTopLeft.x + 12.0f, overlayTopLeft.y - 12.0f},
               {overlayTopLeft.x + 24.0f, overlayTopLeft.y - 12.0f},
               lineThickness, overlayColor, zIndex);
    submitLine(renderer, {overlayTopLeft.x + 18.0f, overlayTopLeft.y - 18.0f},
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
                submitArrow(renderer, pos, tip, lineThickness, velocityColor, zIndex);
            }
        }
    }
}
