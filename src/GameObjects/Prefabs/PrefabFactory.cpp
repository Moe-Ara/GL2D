//
// PrefabFactory.cpp
//

#include "PrefabFactory.hpp"
#include "GameObjects/Entity.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "Managers/ComponentFactory.hpp"
#include "PrefabCatalouge.hpp"

std::unique_ptr<Entity> PrefabFactory::instantiate(const Prefab &prefab,
                                                   const glm::vec2 &position,
                                                   const glm::vec2 &scale,
                                                   float rotation,
                                                   const std::unordered_map<std::string, ComponentSpec> &overrides) {
    auto entity = std::make_unique<Entity>();

    // Transform first
    auto &transform = entity->addComponent<TransformComponent>();
    transform.setPosition(position);
    transform.setScale(scale);
    transform.setRotation(rotation);

    // Merge prefab components with overrides
    std::unordered_map<std::string, ComponentSpec> merged = prefab.components;
    for (const auto &ov : overrides) {
        merged[ov.first] = ov.second;
    }

    for (const auto &entry : merged) {
        const ComponentSpec &spec = entry.second;
        auto component = ComponentFactory::create(spec);
        if (component) {
            entity->addComponent(std::move(component));
        }
    }

    return entity;
}

std::unique_ptr<Entity> PrefabFactory::instantiate(const std::string &prefabId,
                                                   const glm::vec2 &position,
                                                   const glm::vec2 &scale,
                                                   float rotation,
                                                   const std::unordered_map<std::string, ComponentSpec> &overrides) {
    if (!PrefabCatalouge::contains(prefabId)) {
        return nullptr;
    }
    const Prefab& prefab = PrefabCatalouge::get(prefabId);
    return instantiate(prefab, position, scale, rotation, overrides);
}
