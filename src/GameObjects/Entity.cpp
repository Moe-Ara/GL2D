//
// Created by Mohamad on 19/11/2025.
//

#include "Entity.hpp"
#include "Engine/IController.hpp"
#include "Utils/EntityAttributes.hpp"
#include "Utils/Transform.hpp"


void Entity::update(double deltaTime) {
    for (auto &c: m_components) {
        if (auto *updatable = dynamic_cast<IUpdatableComponent *>(c.get())) {
            updatable->update(*this, deltaTime);
        }
    }
}

const std::vector<std::unique_ptr<IComponent>> &Entity::components() const {
    return m_components;

}

void Entity::render() {
    for (auto &c: m_components) {
        if (auto *renderable = dynamic_cast<IRenderableComponent *>(c.get())) {
            renderable->render(*this);
        }
    }
}
