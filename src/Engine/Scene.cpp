//
// Created by Mohamad on 21/11/2025.
//

#include "Scene.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "RenderingSystem/RenderSystem.hpp"
#include <algorithm>

Scene::Scene() {

}

Scene::~Scene() {

}

Entity &Scene::createEntity() {
    m_entities.push_back(std::make_unique<Entity>());
    return *m_entities.back();
}

void Scene::destroyEntity(Entity &entity) {
    m_triggerSystem.unregisterEntity(entity.getId());
    auto it= std::find_if(m_entities.begin(),m_entities.end(),[&entity](const std::unique_ptr<Entity>& ptr){
        return ptr.get()==&entity;
    });
    if(it!=m_entities.end()){
        m_entities.erase(it);
    }
}

void Scene::clear() {
    m_triggerSystem.clear();
    m_entities.clear();
}

void Scene::update(float deltaTime) {
    if (m_paused) {
        return;
    }
    // Feelings drive cross-system parameters; update blend first.
    m_feelingsSystem.update(deltaTime * 1000.0f); // deltaTime is seconds; convert to ms.
    for(auto& e: m_entities){
        e->update(deltaTime);
    }
    m_waterSystem.update(deltaTime, m_entities, m_physicsEngine.getGravity());
    m_physicsEngine.step(deltaTime, m_entities);
    m_triggerSystem.update(m_entities);
}

void Scene::updateWorld(float deltaTime, Camera &camera, Rendering::Renderer &renderer) {
    update(deltaTime);
    camera.update(deltaTime);
    RenderSystem::renderScene(*this, camera, renderer);
}

std::vector<std::unique_ptr<Entity>> &Scene::getEntities() {
    return m_entities;
}

const std::vector<std::unique_ptr<Entity>> &Scene::getEntities() const {
    return m_entities;
}
