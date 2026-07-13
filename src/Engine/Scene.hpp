//
// Created by Mohamad on 21/11/2025.
//

#ifndef GL2D_SCENE_HPP
#define GL2D_SCENE_HPP


#include "GameObjects/Entity.hpp"
#include "Physics/PhysicsEngine.hpp"
#include "Physics/TriggerSystem.hpp"
#include "Physics/WaterSystem.hpp"
#include "Graphics/Camera/Camera.hpp"
#include "RenderingSystem/Renderer.hpp"
#include "RenderingSystem/PostProcessSettings.hpp"
#include "FeelingsSystem/FeelingsSystem.hpp"
#include "ECS/Registry.hpp"
#include "Engine/FixedStepClock.hpp"

#include <unordered_map>
#include <unordered_set>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

class Scene {
public:
    Scene() = default;

    virtual ~Scene() = default;

    Scene(const Scene &other) = delete;

    Scene &operator=(const Scene &other) = delete;

    Scene(Scene &&other) = delete;

    Scene &operator=(Scene &&other) = delete;
    Entity& createEntity();
    Entity& addEntity(std::unique_ptr<Entity> entity);
    void destroyEntity(Entity& entity);
    void clear();
    // Advances real frame time through deterministic fixed simulation steps.
    Engine::FixedStepClock::Result advance(float frameDeltaTime);
    // Runs exactly one simulation step. Prefer advance() in game loops.
    void update(float deltaTime);
    // Full world step: components -> physics -> triggers -> camera -> render.
    void updateWorld(float deltaTime, Camera& camera, Rendering::Renderer& renderer);
    std::vector<std::unique_ptr<Entity>>& getEntities();
    const std::vector<std::unique_ptr<Entity>>& getEntities() const;
    void setPaused(bool paused) { m_paused = paused; }
    [[nodiscard]] bool isPaused() const { return m_paused; }
    FeelingsSystem::FeelingsSystem& feelings() { return m_feelingsSystem; }
    const FeelingsSystem::FeelingsSystem& feelings() const { return m_feelingsSystem; }
    // ECS-native entities live here while legacy Entity components are migrated
    // subsystem by subsystem. New data-oriented systems should use this registry.
    ECS::Registry& registry() noexcept { return m_ecsRegistry; }
    const ECS::Registry& registry() const noexcept { return m_ecsRegistry; }
    // Authored presentation settings for this scene. Feelings may temporarily
    // override a subset of these values during rendering.
    Rendering::PostProcessSettings& postProcess() noexcept { return m_postProcessSettings; }
    const Rendering::PostProcessSettings& postProcess() const noexcept { return m_postProcessSettings; }
    void setAmbientLight(glm::vec3 color);
    [[nodiscard]] const glm::vec3& ambientLight() const noexcept { return m_ambientLight; }
    // Background clear color for the scene's HDR target, in linear RGBA. Authors
    // set the world's void/backdrop tone here instead of relying on a renderer
    // default.
    void setClearColor(const glm::vec4& color);
    [[nodiscard]] const glm::vec4& clearColor() const noexcept { return m_clearColor; }
    void configureFixedStep(Engine::FixedStepClock::Config config) { m_fixedClock.configure(config); }
    [[nodiscard]] double fixedStepSeconds() const noexcept { return m_fixedClock.stepSeconds(); }
    [[nodiscard]] double interpolationAlpha() const noexcept { return m_fixedClock.interpolationAlpha(); }
    // Position a legacy entity held at the start of the last simulation step,
    // for render interpolation. Returns nullptr for unknown entities.
    [[nodiscard]] const glm::vec2* previousPosition(uint64_t entityId) const {
        const auto it = m_previousPositions.find(entityId);
        return it != m_previousPositions.end() ? &it->second : nullptr;
    }
private:
    void snapshotTransformsForInterpolation();
    void detachLegacyEntityReferences(const Entity* target);
    void flushPendingMutations();

    std::vector<std::unique_ptr<Entity>> m_entities;
    std::vector<std::unique_ptr<Entity>> m_pendingAdditions;
    std::unordered_set<uint64_t> m_pendingDestructions;
    PhysicsEngine m_physicsEngine{};
    TriggerSystem m_triggerSystem{};
    WaterSystem m_waterSystem{};
    FeelingsSystem::FeelingsSystem m_feelingsSystem{};
    ECS::Registry m_ecsRegistry{};
    Rendering::PostProcessSettings m_postProcessSettings{};
    glm::vec3 m_ambientLight{0.16f, 0.16f, 0.18f};
    glm::vec4 m_clearColor{0.05f, 0.05f, 0.08f, 1.0f};
    Engine::FixedStepClock m_fixedClock{};
    std::unordered_map<uint64_t, glm::vec2> m_previousPositions;
    std::vector<ECS::Entity> m_smoothedNeedingHistory;
    bool m_paused{false};
    bool m_updating{false};
    bool m_clearPending{false};
};


#endif //GL2D_SCENE_HPP
