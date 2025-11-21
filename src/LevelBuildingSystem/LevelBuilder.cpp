//
// Created by Codex on 22/11/2025.
//

#include "LevelBuilder.hpp"
#include "GameObjects/Entity.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "Managers/ComponentFactory.hpp"
#include "GameObjects/Prefabs/PrefabCatalouge.hpp"

std::unique_ptr<Entity> LevelBuilder::instantiate(const PlacedEntitySpec &placement) {
    const Prefab& prefab = PrefabCatalouge::get(placement.prefabId);

    auto entity = std::make_unique<Entity>();

    // Always ensure a transform exists and apply the placement data.
    auto &transform = entity->addComponent<TransformComponent>();
    transform.setPosition(placement.position);
    transform.setScale(placement.scale);
    transform.setRotation(placement.rotation);

    // Merge prefab components with per-instance overrides.
    std::unordered_map<std::string, ComponentSpec> merged = prefab.components;
    for (const auto &overrideEntry : placement.componentOverrides) {
        merged[overrideEntry.first] = overrideEntry.second;
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

std::vector<std::unique_ptr<Entity>>
LevelBuilder::build(const std::vector<PlacedEntitySpec> &placements) {
    std::vector<std::unique_ptr<Entity>> entities;
    entities.reserve(placements.size());
    for (const auto &placement : placements) {
        entities.push_back(instantiate(placement));
    }
    return entities;
}
