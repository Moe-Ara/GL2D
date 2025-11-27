#ifndef AI_ENTITY_HPP
#define AI_ENTITY_HPP

#include <memory>

#include "Engine/IController.hpp"
#include "GameObjects/Entity.hpp"

class AIEntity {
public:
    AIEntity(std::unique_ptr<Entity> entity, std::unique_ptr<IController> controller)
        : m_entity(std::move(entity)), m_controller(std::move(controller)) {}

    Entity& entity() { return *m_entity; }
    const Entity& entity() const { return *m_entity; }

    IController* controller() { return m_controller.get(); }
    const IController* controller() const { return m_controller.get(); }

    template<typename T, typename... Args>
    T& addComponent(Args&&... args) {
        return m_entity->addComponent<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    T* getComponent() {
        return m_entity->getComponent<T>();
    }

    void update(double dt) {
        if (m_controller && m_entity) {
            m_controller->update(*m_entity, dt);
        }
        if (m_entity) {
            m_entity->update(dt);
        }
    }

private:
    std::unique_ptr<Entity> m_entity;
    std::unique_ptr<IController> m_controller;
};

#endif // AI_ENTITY_HPP
