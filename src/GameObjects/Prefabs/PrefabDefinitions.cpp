//
// Created by Codex on 27/11/2025.
//

#include "PrefabDefinitions.hpp"

#include "GameObjects/Prefabs/PrefabCatalouge.hpp"
#include "GameObjects/Prefabs/Prefab.hpp"
#include "GameObjects/Sprite.hpp"
#include "Managers/SpriteManager.hpp"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <memory>
#include <string>

namespace {
std::shared_ptr<GameObjects::Sprite> g_ropeSegmentSprite{};

std::shared_ptr<GameObjects::Sprite> ensurePrefabSprite(
        const std::string& id, const glm::vec2& size, const glm::vec3& color) {
    if (auto sprite = SpriteManager::get(id)) {
        return sprite;
    }
    auto sprite = std::make_shared<GameObjects::Sprite>(
        glm::vec2{0.0f}, size, color);
    SpriteManager::registerSprite(id, sprite);
    return sprite;
}

void registerRopeSegmentPrefab() {
    const std::string prefabId = "rope_segment";
    g_ropeSegmentSprite = ensurePrefabSprite(
        "rope_segment_sprite", glm::vec2{1.0f, 1.0f},
        glm::vec3{0.45f, 0.25f, 0.10f});
    if (PrefabCatalouge::contains(prefabId)) {
        return;
    }

    Prefab ropeSegment{};
    ropeSegment.id = prefabId;
    ropeSegment.defaultSize = glm::vec2{1.0f, 1.0f};
    ropeSegment.hasCollider = true;

    ComponentSpec spriteSpec{};
    spriteSpec.type = "Sprite";
    spriteSpec.strings["spriteId"] = "rope_segment_sprite";
    ropeSegment.components["Sprite"] = std::move(spriteSpec);

    ComponentSpec colliderSpec{};
    colliderSpec.type = "Collider";
    colliderSpec.strings["shape"] = "AABB";
    colliderSpec.numbers["minX"] = -0.5f;
    colliderSpec.numbers["minY"] = -0.5f;
    colliderSpec.numbers["maxX"] = 0.5f;
    colliderSpec.numbers["maxY"] = 0.5f;
    colliderSpec.numbers["layer"] = 0.0f;
    ropeSegment.components["Collider"] = std::move(colliderSpec);

    ComponentSpec rigidSpec{};
    rigidSpec.type = "RigidBody";
    rigidSpec.numbers["mass"] = 0.35f;
    rigidSpec.numbers["linearDamping"] = 3.0f;
    rigidSpec.numbers["angularDamping"] = 4.0f;
    ropeSegment.components["RigidBody"] = std::move(rigidSpec);

    PrefabCatalouge::registerPrefab(ropeSegment);
}
} // namespace

namespace Prefabs {
void registerGamePrefabs() {
    registerRopeSegmentPrefab();
}
} // namespace Prefabs
