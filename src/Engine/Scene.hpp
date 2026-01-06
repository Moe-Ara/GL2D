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
#include "FeelingsSystem/FeelingsSystem.hpp"

class Scene {
public:
    Scene();

    virtual ~Scene();

    Scene(const Scene &other) = delete;

    Scene &operator=(const Scene &other) = delete;

    Scene(Scene &&other) = delete;

    Scene &operator=(Scene &&other) = delete;
    Entity& createEntity();
    Entity& addEntity(std::unique_ptr<Entity> entity);
    void destroyEntity(Entity& entity);
    void clear();
    void update(float deltaTime);
    // Full world step: components -> physics -> triggers -> camera -> render.
    void updateWorld(float deltaTime, Camera& camera, Rendering::Renderer& renderer);
    std::vector<std::unique_ptr<Entity>>& getEntities();
    const std::vector<std::unique_ptr<Entity>>& getEntities() const;
    void setPaused(bool paused) { m_paused = paused; }
    [[nodiscard]] bool isPaused() const { return m_paused; }
    FeelingsSystem::FeelingsSystem& feelings() { return m_feelingsSystem; }
    const FeelingsSystem::FeelingsSystem& feelings() const { return m_feelingsSystem; }
private:
    std::vector<std::unique_ptr<Entity>> m_entities;
    PhysicsEngine m_physicsEngine{};
    TriggerSystem m_triggerSystem{};
    WaterSystem m_waterSystem{};
    FeelingsSystem::FeelingsSystem m_feelingsSystem{};
    bool m_paused{false};
};


#endif //GL2D_SCENE_HPP
