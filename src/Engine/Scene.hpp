//
// Created by Mohamad on 21/11/2025.
//

#ifndef GL2D_SCENE_HPP
#define GL2D_SCENE_HPP


#include "GameObjects/Entity.hpp"
#include "Physics/TriggerSystem.hpp"

class Scene {
public:
    Scene();

    virtual ~Scene();

    Scene(const Scene &other) = delete;

    Scene &operator=(const Scene &other) = delete;

    Scene(Scene &&other) = delete;

    Scene &operator=(Scene &&other) = delete;
    Entity& createEntity();
    void destroyEntity(Entity& entity);
    void clear();
    void update(float deltaTime);
    std::vector<std::unique_ptr<Entity>>& getEntities();
    const std::vector<std::unique_ptr<Entity>>& getEntities() const;
private:
    std::vector<std::unique_ptr<Entity>> m_entities;
    TriggerSystem m_triggerSystem{};
};


#endif //GL2D_SCENE_HPP
