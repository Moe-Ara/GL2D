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
#include "GameObjects/Components/GroundSensorComponent.hpp"
#include "Debug/DebugOverlay.hpp"
#include "GameObjects/Sprite.hpp"
#include "RenderingSystem/TilemapRenderer.hpp"
#include "GameObjects/Components/LightingComponent.hpp"
#include "RenderingSystem/RenderTarget.hpp"
#include "RenderingSystem/LightingPass.hpp"
#include "Graphics/LightingSystem/Light.hpp"
#include "Graphics/LightingSystem/LightEffector.hpp"
#include "Managers/TextureManager.hpp"
#include "FeelingsSystem/FeelingSnapshot.hpp"
#include <algorithm>
#include <GL/glew.h>
#include <vector>
#include <glm/geometric.hpp>
#include <chrono>
#include <cmath>
namespace {
inline bool overlaps(const glm::vec4 &aabb, const glm::vec4 &view) {
    return !(aabb.z < view.x || aabb.x > view.z || aabb.w < view.y ||
             aabb.y > view.w);
}

Rendering::RenderTarget& sharedRenderTarget() {
    static Rendering::RenderTarget target;
    return target;
}

struct LightingFeeling {
    float intensityMul{1.0f};
    float radiusMul{1.0f};
    glm::vec3 colorMul{1.0f, 1.0f, 1.0f};
    glm::vec3 ambientAdd{0.0f, 0.0f, 0.0f};
    glm::vec3 ambientMul{1.0f, 1.0f, 1.0f};
};

LightingFeeling& feelingState() {
    static LightingFeeling s_state;
    return s_state;
}

double nowSeconds() {
    using clock = std::chrono::steady_clock;
    static const auto start = clock::now();
    const auto t = clock::now() - start;
    return std::chrono::duration<double>(t).count();
}

void applyEffectorToLight(const LightEffector& eff, double t, Light& out) {
    const float time = static_cast<float>(t);
    switch (eff.type) {
        case LightEffector::Type::Flicker: {
            const float wave = 0.5f * (std::sin(time * eff.speed + eff.phase) + 1.0f); // 0..1
            const float factor = 1.0f - eff.strength * 0.5f + eff.strength * wave;
            out.intensityMul *= std::max(0.1f, factor);
            break;
        }
        case LightEffector::Type::Pulse: {
            const float wave = 0.5f * (std::sin(time * eff.speed + eff.phase) + 1.0f); // 0..1
            const float factor = 1.0f - eff.strength * 0.5f + eff.strength * wave;
            out.intensityMul *= std::max(0.1f, factor);
            out.radiusMul *= std::max(0.5f, factor);
            break;
        }
        case LightEffector::Type::Sweep: {
            glm::vec2 dir = eff.sweepDir;
            const float len = glm::length(dir);
            if (len > 0.0001f) dir /= len;
            const float wave = std::sin(time * eff.speed + eff.phase); // -1..1
            out.posOffset += dir * eff.sweepSpan * wave;
            break;
        }
    }
}
}

void RenderSystem::renderScene(Scene &scene, Camera &camera,
                               Rendering::Renderer &renderer) {
    const glm::mat4 &viewProj = camera.getViewProjection();
    const glm::vec4 viewBounds =
        camera.getViewBounds(/*paddingFactor=*/1.0f); // expand by half the view size

    // Ensure the off-screen target matches the window size.
    auto &rt = sharedRenderTarget();
    const glm::vec2 viewportSize = camera.getViewportSize();
    const int fbWidth = std::max(1, static_cast<int>(viewportSize.x));
    const int fbHeight = std::max(1, static_cast<int>(viewportSize.y));
    if (!rt.isInitialized()) {
        rt.initialize(fbWidth, fbHeight);
    } else {
        rt.resize(fbWidth, fbHeight);
    }

    rt.bind();
    glViewport(0, 0, fbWidth, fbHeight);

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
    rt.unbind();

    // Gather lights (culled to the camera view with padding).
    std::vector<Light> lights;
    std::vector<GLuint> cookieTextures;
    constexpr int kMaxCookieSlots = 8;
    const auto feeling = feelingState();
    glm::vec3 ambientColor = glm::vec3(0.16f, 0.16f, 0.18f); // slightly brighter ambient
    ambientColor = ambientColor * feeling.ambientMul + feeling.ambientAdd;
    const glm::vec4 lightCullBounds = camera.getViewBounds(/*paddingFactor=*/0.25f);
    for (auto &entityPtr : scene.getEntities()) {
        if (!entityPtr) continue;
        auto *lightComp = entityPtr->getComponent<LightingComponent>();
        auto *transformComp = entityPtr->getComponent<TransformComponent>();
        if (!lightComp || !transformComp) continue;

        const glm::vec2 worldPos = transformComp->getTransform().Position + lightComp->localPos();
        if (!overlaps(glm::vec4(worldPos.x, worldPos.y, worldPos.x, worldPos.y), lightCullBounds)) {
            continue;
        }

        Light gpuLight{};
        gpuLight.type = lightComp->type();
        gpuLight.pos = worldPos;
        gpuLight.radius = lightComp->radius() * feeling.radiusMul;
        gpuLight.color = lightComp->color() * feeling.colorMul;
        gpuLight.intensity = lightComp->intensity() * feeling.intensityMul;
        gpuLight.falloff = lightComp->falloff();
        gpuLight.emissiveBoost = lightComp->emissiveBoost();
        glm::vec2 dir = lightComp->direction();
        const float len = glm::length(dir);
        gpuLight.dir = len > 0.0001f ? dir / len : glm::vec2(0.0f, -1.0f);
        gpuLight.innerCutoff = lightComp->innerCutoff();
        gpuLight.outerCutoff = lightComp->outerCutoff();
        gpuLight.cookieStrength = lightComp->cookieStrength();
        if (!lightComp->cookiePath().empty() && lightComp->cookieStrength() > 0.0f && static_cast<int>(cookieTextures.size()) < kMaxCookieSlots) {
            auto tex = Managers::TextureManager::loadTexture(lightComp->cookiePath(), false);
            if (tex) {
                gpuLight.cookieSlot = static_cast<int>(cookieTextures.size());
                cookieTextures.push_back(tex->getID());
            }
        }
        if (const auto& eff = lightComp->effector()) {
            applyEffectorToLight(*eff, nowSeconds(), gpuLight);
            gpuLight.pos += gpuLight.posOffset;
            gpuLight.radius *= gpuLight.radiusMul;
            gpuLight.intensity *= gpuLight.intensityMul;
        }
        lights.push_back(gpuLight);
    }

    // Always inject a soft directional "moon" light.
    Light moon{};
    moon.type = LightType::DIRECTIONAL;
    moon.dir = glm::normalize(glm::vec2{-0.3f, -1.0f});
    moon.color = glm::vec3(0.8f, 0.85f, 0.95f);
    moon.intensity = 0.55f;
    moon.falloff = 1.0f;
    moon.innerCutoff = 1.0f;
    moon.outerCutoff = 1.0f;
    lights.push_back(moon);

    // No extra injected point lights; rely on authored lights and fallback moon.

    // Camera-follow fill so the whole view is lit, even with few world lights.
    // Add a soft directional fill from above to avoid circular falloff artifacts.
    {
        Light overhead{};
        overhead.type = LightType::DIRECTIONAL;
        overhead.dir = glm::normalize(glm::vec2{0.0f, -1.0f});
        overhead.color = glm::vec3(0.95f, 1.0f, 1.05f);
        overhead.intensity = 0.8f;
        overhead.falloff = 1.0f;
        overhead.innerCutoff = 1.0f;
        overhead.outerCutoff = 1.0f;
        lights.push_back(overhead);
    }

    // Composite the off-screen color with the lighting pass.
    glViewport(0, 0, fbWidth, fbHeight);
    Rendering::LightingPass::draw(rt, lights, camera.getViewBounds(/*paddingFactor=*/0.0f), cookieTextures, ambientColor);

    // Debug: draw collider wireframes and sensors on top of lighting when overlay is enabled.
    if (DebugOverlay::enabled()) {
        glDisable(GL_BLEND); // simple lines over lit scene
        for (auto &entityPtr : scene.getEntities()) {
            if (!entityPtr) continue;
            if (auto *colliderComp = entityPtr->getComponent<ColliderComponent>()) {
                colliderComp->debugDraw(viewProj, {1.0f, 0.0f, 0.0f});
            }
            if (auto* sensor = entityPtr->getComponent<GroundSensorComponent>()) {
                sensor->debugDraw(viewProj, *entityPtr, {0.2f, 0.9f, 0.2f}, {0.2f, 0.6f, 1.0f});
            }
        }
        glEnable(GL_BLEND);
    }
}

void RenderSystem::applyFeeling(const FeelingsSystem::FeelingSnapshot &snapshot) {
    auto &state = feelingState();
    state.intensityMul = snapshot.lightIntensityMul.value_or(1.0f);
    state.radiusMul = snapshot.lightRadiusMul.value_or(1.0f);
    state.colorMul = snapshot.lightColorMul.value_or(glm::vec3(1.0f));
    state.ambientMul = snapshot.ambientLightMul.value_or(glm::vec3(1.0f));
    state.ambientAdd = snapshot.ambientLightAdd.value_or(glm::vec3(0.0f));
}
