//
// Created by Codex on 22/11/2025.
//

#include "LevelBuilder.hpp"
#include "GameObjects/Entity.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Prefabs/PrefabFactory.hpp"
#include "GameObjects/Prefabs/PrefabCatalouge.hpp"

std::unique_ptr<Entity> LevelBuilder::instantiate(const PlacedEntitySpec &placement) {
    const Prefab& prefab = PrefabCatalouge::get(placement.prefabId);
    return PrefabFactory::instantiate(prefab, placement.position, placement.scale, placement.rotation, placement.componentOverrides);
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
