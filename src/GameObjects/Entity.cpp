//
// Created by Mohamad on 19/11/2025.
//

#include "Entity.hpp"
#include "Engine/IController.hpp"
#include "Utils/EntityAttributes.hpp"
#include "Utils/Transform.hpp"

std::atomic<uint64_t> Entity::s_nextId = 1;

Entity::Entity() : m_id{s_nextId.fetch_add(1, std::memory_order_relaxed)} {}


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
