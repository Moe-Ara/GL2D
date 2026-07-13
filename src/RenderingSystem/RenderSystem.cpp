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
#include "RenderingSystem/ColorRenderTarget.hpp"
#include "RenderingSystem/LightingPass.hpp"
#include "RenderingSystem/PostProcessPipeline.hpp"
#include "Graphics/LightingSystem/Light.hpp"
#include "Graphics/LightingSystem/LightEffector.hpp"
#include "Managers/TextureManager.hpp"
#include "FeelingsSystem/FeelingSnapshot.hpp"
#include "ECS/Components/SmoothedTransform2D.hpp"
#include "ECS/Components/SpriteRender.hpp"
#include "ECS/Components/Transform2D.hpp"
#include "ECS/Components/Light2D.hpp"
#include "ECS/Systems/LightExtractionSystem.hpp"
#include "ECS/Systems/ParallaxSystem2D.hpp"
#include "ECS/Components/ParticleEmitter2D.hpp"
#include "ECS/Components/ParticleRender2D.hpp"
#include "RenderingSystem/ParticleRenderer.hpp"
#include <algorithm>
#include <GL/glew.h>
#include <vector>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <chrono>
#include <cmath>
#include <limits>
namespace {
inline bool overlaps(const glm::vec4 &aabb, const glm::vec4 &view) {
    return !(aabb.z < view.x || aabb.x > view.z || aabb.w < view.y ||
             aabb.y > view.w);
}

glm::vec4 transformedBounds(const glm::mat4& model, const glm::vec2& size) {
    const glm::vec2 corners[] = {{0.0f, 0.0f}, {size.x, 0.0f},
                                 {size.x, size.y}, {0.0f, size.y}};
    glm::vec2 minPoint{std::numeric_limits<float>::max()};
    glm::vec2 maxPoint{std::numeric_limits<float>::lowest()};
    for (const glm::vec2& corner : corners) {
        const glm::vec4 world = model * glm::vec4(corner, 0.0f, 1.0f);
        const glm::vec2 point{world.x, world.y};
        minPoint = glm::min(minPoint, point);
        maxPoint = glm::max(maxPoint, point);
    }
    return {minPoint.x, minPoint.y, maxPoint.x, maxPoint.y};
}

struct LightingFeeling {
    float intensityMul{1.0f};
    float radiusMul{1.0f};
    glm::vec3 colorMul{1.0f, 1.0f, 1.0f};
    glm::vec3 ambientAdd{0.0f, 0.0f, 0.0f};
    glm::vec3 ambientMul{1.0f, 1.0f, 1.0f};
    std::optional<glm::vec3> colorTint{};
    std::optional<float> vignetteStrength{};
    std::optional<float> bloomStrength{};
};

LightingFeeling extractLightingFeeling(
    const FeelingsSystem::FeelingSnapshot& snapshot) {
    LightingFeeling feeling;
    feeling.intensityMul = snapshot.lightIntensityMul.value_or(1.0f);
    feeling.radiusMul = snapshot.lightRadiusMul.value_or(1.0f);
    feeling.colorMul = snapshot.lightColorMul.value_or(glm::vec3(1.0f));
    feeling.ambientMul = snapshot.ambientLightMul.value_or(glm::vec3(1.0f));
    feeling.ambientAdd = snapshot.ambientLightAdd.value_or(glm::vec3(0.0f));
    if (snapshot.colorGrade) {
        feeling.colorTint = glm::max(glm::vec3(*snapshot.colorGrade),
                                     glm::vec3(0.0f));
    }
    if (snapshot.vignette) {
        feeling.vignetteStrength = std::clamp(*snapshot.vignette, 0.0f, 1.0f);
    }
    if (snapshot.bloomStrength) {
        feeling.bloomStrength = std::max(*snapshot.bloomStrength, 0.0f);
    }
    return feeling;
}

double nowSeconds() {
    using clock = std::chrono::steady_clock;
    static const auto start = clock::now();
    const auto t = clock::now() - start;
    return std::chrono::duration<double>(t).count();
}

bool lightOverlapsView(const Light& light, const glm::vec4& viewBounds) {
    if (light.type == LightType::DIRECTIONAL) {
        return true;
    }
    return overlaps({light.pos.x - light.radius, light.pos.y - light.radius,
                     light.pos.x + light.radius, light.pos.y + light.radius},
                    viewBounds);
}
}

void RenderSystem::renderScene(Scene &scene, Camera &camera,
                               Rendering::Renderer &renderer) {
    const glm::mat4 &viewProj = camera.getViewProjection();
    const glm::vec4 viewBounds =
        camera.getViewBounds(/*paddingFactor=*/1.0f); // expand by half the view size

    // Presentation pass: scroll parallax layers to the camera before extraction
    // so their transforms are current for both culling and submission.
    const glm::vec4 tightView = camera.getViewBounds(/*paddingFactor=*/0.0f);
    ECS::ParallaxSystem2D::update(scene.registry(), camera.getTransform().Position,
                                  tightView.x, tightView.z);

    // Ensure the off-screen target matches the window size.
    auto &rt = renderer.sceneTarget();
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

    renderer.beginFrame(viewProj, scene.clearColor(), true);

    renderer.tilemapRenderer().render(scene, viewProj);

    // Render interpolation: draw poses blended between the last two fixed
    // steps so movement is smooth at any display rate.
    const float interpolationAlpha =
        static_cast<float>(scene.interpolationAlpha());

    for (auto &entityPtr : scene.getEntities()) {
        if (!entityPtr) continue;
        auto *spriteComp = entityPtr->getComponent<SpriteComponent>();
        auto *transformComp = entityPtr->getComponent<TransformComponent>();
        if (!spriteComp || !transformComp) continue;

        const auto baseSize = spriteComp->sprite() ? spriteComp->sprite()->getSize() : glm::vec2(0.0f);
        glm::mat4 model = transformComp->modelMatrix();
        if (const glm::vec2* previous =
                scene.previousPosition(entityPtr->getId())) {
            const glm::vec2 interpolated = glm::mix(
                *previous, transformComp->getTransform().Position,
                interpolationAlpha);
            model[3].x = interpolated.x;
            model[3].y = interpolated.y;
        }
        const glm::vec4 spriteAabb = transformedBounds(model, baseSize);

        if (!overlaps(spriteAabb, viewBounds)) {
            continue;
        }

        if (auto *sprite = spriteComp->sprite()) {
            renderer.submitSprite(*sprite, model,
                                  spriteComp->layer(),
                                  spriteComp->zIndex());
        }
    }

    // ECS-native render extraction. It intentionally shares the same renderer
    // queue as legacy entities so sorting remains deterministic during migration.
    scene.registry().each<ECS::Transform2D, ECS::SpriteRender>(
        [&](ECS::Entity entity, const ECS::Transform2D& transform, const ECS::SpriteRender& renderable) {
            if (!renderable.visible || !renderable.sprite) {
                return;
            }
            const auto* previous =
                scene.registry().tryGet<ECS::PreviousTransform2D>(entity);
            const glm::mat4 model = previous
                ? ECS::toMatrix(ECS::interpolatedTransform2D(
                      previous->value, transform, interpolationAlpha))
                : ECS::toMatrix(transform);
            if (!overlaps(transformedBounds(model, renderable.sprite->getSize()), viewBounds)) {
                return;
            }
            Rendering::SpriteDrawData drawData{};
            drawData.color = renderable.sprite->getColor() * renderable.tint *
                             renderable.animationTint;
            drawData.uvRect = renderable.useCustomUV
                ? renderable.uvRect : renderable.sprite->getUVCoords();
            drawData.flipX = renderable.flipX;
            drawData.textureOverride = renderable.textureOverride.get();
            drawData.normalTextureOverride = renderable.normalTextureOverride.get();
            renderer.submitSprite(*renderable.sprite, model, renderable.layer,
                                  renderable.zIndex, drawData);
        });

    renderer.endFrame();
    rt.unbind();

    // Gather lights (culled to the camera view with padding).
    std::vector<Light> lights;
    std::vector<GLuint> cookieTextures;
    constexpr int kMaxCookieSlots = 8;
    const auto feeling = extractLightingFeeling(scene.feelings().getSnapshot());
    glm::vec3 ambientColor = scene.ambientLight();
    ambientColor = glm::max(
        ambientColor * feeling.ambientMul + feeling.ambientAdd,
        glm::vec3(0.0f));
    const glm::vec4 lightCullBounds = camera.getViewBounds(/*paddingFactor=*/0.25f);
    for (auto &entityPtr : scene.getEntities()) {
        if (!entityPtr) continue;
        auto *lightComp = entityPtr->getComponent<LightingComponent>();
        auto *transformComp = entityPtr->getComponent<TransformComponent>();
        if (!lightComp || !transformComp) continue;

        const glm::vec2 worldPos = transformComp->getTransform().Position + lightComp->localPos();

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
        if (const auto& eff = lightComp->effector()) {
            ECS::applyLightEffector(gpuLight, *eff, nowSeconds());
        }
        if (!lightOverlapsView(gpuLight, lightCullBounds)) {
            continue;
        }
        if (!lightComp->cookiePath().empty() && lightComp->cookieStrength() > 0.0f &&
            static_cast<int>(cookieTextures.size()) < kMaxCookieSlots) {
            auto tex = Managers::TextureManager::loadTexture(
                lightComp->cookiePath(), false);
            if (tex) {
                gpuLight.cookieSlot = static_cast<int>(cookieTextures.size());
                cookieTextures.push_back(tex->getID());
            }
        }
        lights.push_back(gpuLight);
    }


    // ECS-native lighting uses Transform2D as its world anchor. Light animation
    // is optional composition rather than mutable state embedded in every light.
    scene.registry().each<ECS::Transform2D, ECS::Light2D>(
        [&](ECS::Entity entity, const ECS::Transform2D& transform,
            const ECS::Light2D& light) {
            const auto* animation =
                scene.registry().tryGet<ECS::LightAnimation2D>(entity);
            auto extracted = ECS::extractLight(
                transform, light, animation, nowSeconds());
            if (!extracted || extracted->intensity <= 0.0f) {
                return;
            }

            Light gpuLight = *extracted;
            gpuLight.radius *= feeling.radiusMul;
            gpuLight.color *= feeling.colorMul;
            gpuLight.intensity *= feeling.intensityMul;
            if (!lightOverlapsView(gpuLight, lightCullBounds)) {
                return;
            }
            if (light.cookie && gpuLight.cookieStrength > 0.0f &&
                static_cast<int>(cookieTextures.size()) < kMaxCookieSlots) {
                gpuLight.cookieSlot = static_cast<int>(cookieTextures.size());
                cookieTextures.push_back(light.cookie->getID());
            }
            lights.push_back(gpuLight);
        });

    // Resolve lighting into HDR before tone mapping and presentation effects.
    auto& lightingTarget = renderer.lightingTarget();
    lightingTarget.resize(fbWidth, fbHeight);
    lightingTarget.bind();
    glViewport(0, 0, fbWidth, fbHeight);
    renderer.lightingPass().draw(rt, lights,
                                 glm::inverse(camera.getViewProjection()),
                                 cookieTextures, ambientColor);

    struct ParticleDrawSource {
        ECS::Entity entity;
        const ECS::ParticleEmitter2D* emitter;
        const ECS::ParticleRender2D* presentation;
    };
    std::vector<ParticleDrawSource> particleSources;
    scene.registry().each<ECS::ParticleEmitter2D, ECS::ParticleRender2D>(
        [&particleSources](ECS::Entity entity,
                           const ECS::ParticleEmitter2D& emitter,
                           const ECS::ParticleRender2D& presentation) {
            if (presentation.visible && !emitter.emitter.isFinished()) {
                particleSources.push_back({entity, &emitter, &presentation});
            }
        });
    if (!particleSources.empty()) {
        std::stable_sort(particleSources.begin(), particleSources.end(),
            [](const ParticleDrawSource& left, const ParticleDrawSource& right) {
                if (left.presentation->order != right.presentation->order) {
                    return left.presentation->order < right.presentation->order;
                }
                return left.entity.index() < right.entity.index();
            });
        auto& particleRenderer = renderer.particleRenderer();
        particleRenderer.applyFeeling(scene.feelings().getSnapshot());
        particleRenderer.begin(camera.getViewProjection(),
                               camera.getViewBounds(0.1f));
        for (const ParticleDrawSource& source : particleSources) {
                particleRenderer.setBlendMode(source.presentation->blendMode);
                particleRenderer.setTexture(source.presentation->texture.get());
                for (const Particle& particle : source.emitter->emitter.getParticles()) {
                    if (particle.alive) {
                        particleRenderer.submit({
                            particle.position, particle.size,
                            particle.rotation,
                            particle.color * source.presentation->tint});
                    }
                }
        }
        particleRenderer.end();
    }
    Rendering::ColorRenderTarget::unbind();

    Rendering::PostProcessSettings postProcess = scene.postProcess();
    if (feeling.colorTint) {
        postProcess.colorTint *= *feeling.colorTint;
    }
    if (feeling.vignetteStrength) {
        postProcess.vignetteStrength = std::clamp(
            postProcess.vignetteStrength + *feeling.vignetteStrength,
            0.0f, 1.0f);
    }
    if (feeling.bloomStrength) {
        postProcess.bloomStrength = std::max(
            postProcess.bloomStrength + *feeling.bloomStrength, 0.0f);
    }
    renderer.postProcessor().draw(lightingTarget.texture(), fbWidth, fbHeight,
                                  postProcess);

    if (DebugOverlay::enabled()) {
        renderer.beginFrame(camera.getViewProjection(), {0.0f, 0.0f, 0.0f, 0.0f}, false);
        DebugOverlay::render(scene, camera, renderer);
        for (auto &entityPtr : scene.getEntities()) {
            if (!entityPtr) continue;
            if (auto *colliderComp = entityPtr->getComponent<ColliderComponent>()) {
                colliderComp->debugDraw(renderer, {1.0f, 0.0f, 0.0f, 1.0f});
            }
            if (auto* sensor = entityPtr->getComponent<GroundSensorComponent>()) {
                sensor->debugDraw(renderer, *entityPtr,
                                  {0.2f, 0.9f, 0.2f, 1.0f},
                                  {0.2f, 0.6f, 1.0f, 1.0f});
            }
        }
        renderer.endFrame();
    }
}
