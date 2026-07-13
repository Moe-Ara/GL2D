//
// Created by Mohamad on 06/01/2026.
//

#include "SceneBuilder.hpp"

#include "Player.hpp"
#include "Engine/Scene.hpp"
#include "GameObjects/Sprite.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Components/SpriteComponent.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Components/LedgeComponent.hpp"
#include "InputSystem/InputService.hpp"
#include "Physics/RigidBody.hpp"
#include "RenderingSystem/RenderLayers.hpp"
#include "Managers/TextureManager.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>

#ifndef DEMO_ASSETS_DIR
#define DEMO_ASSETS_DIR "assets"
#endif

namespace {
    glm::vec2 calculateCenteredPosition(const glm::vec2& worldSize,
                                        const glm::vec2& spriteSize) {
        return {(worldSize.x - spriteSize.x) * 0.5f,
                (worldSize.y - spriteSize.y) * 0.5f};
    }
}

SceneBuilder::BuildResult SceneBuilder::build(Scene& scene,
                                              InputService* inputService,
                                              const glm::vec2& worldSize,
                                              const glm::vec2& playerSize) const {
    BuildResult result{};
    const std::filesystem::path demoAssets = DEMO_ASSETS_DIR;
    const std::filesystem::path metadataPath =
        demoAssets / "women/Enchantress/animations.json";
    if (!std::filesystem::exists(metadataPath)) {
        throw std::runtime_error(
            "The Lost Heroin animation metadata is missing: " +
            metadataPath.string());
    }
    auto player = std::make_unique<Player>();
    if (!player->loadFromMetadata(metadataPath)) {
        throw std::runtime_error(
            "Failed to load The Lost Heroin player metadata: " +
            metadataPath.string());
    }
    if (inputService) {
        player->attachController(*inputService);
    }
    // The scene's emotional state is owned by the chapter mood system in Game
    // (design/00-Overview.md); the player carries no hardcoded feeling.
    result.player = player.get();
    scene.addEntity(std::move(player));
    if (result.player) {
        result.player->bindWorldEntities(&scene.getEntities());
        applyPlayerTransform(*result.player, worldSize, playerSize);
    }

    const glm::vec2 groundSize{2000.0f, 80.0f};
    std::shared_ptr<GameObjects::Texture> groundTexture;
    const std::filesystem::path texturePath =
        demoAssets / "PNG/BG_01/Layers/Ground_01.png";
    if (std::filesystem::exists(texturePath)) {
        groundTexture = Managers::TextureManager::loadTexture(texturePath.string());
    } else {
        std::cerr << "Missing ground texture: " << texturePath << std::endl;
    }
    if (groundTexture) {
        result.groundSprite = std::make_shared<GameObjects::Sprite>(
                groundTexture, glm::vec2{0.0f}, groundSize);
    } else {
        result.groundSprite = std::make_shared<GameObjects::Sprite>(
                glm::vec2{0.0f}, groundSize, glm::vec3{0.30f, 0.55f, 0.32f});
    }
    // Tile static ground segments so the player can traverse far enough to see
    // the parallax layers cycle instead of walking off a single short strip.
    constexpr float kGroundExtent = 10000.0f;
    for (float x = -kGroundExtent; x < kGroundExtent; x += groundSize.x) {
        Entity &ground = scene.createEntity();
        auto &groundTransform = ground.addComponent<TransformComponent>();
        groundTransform.setPosition(glm::vec2{x, 0.0f});
        ground.addComponent<SpriteComponent>(result.groundSprite,
                                             -1,
                                             static_cast<int>(Rendering::RenderLayer::Gameplay));
        ground.addComponent<ColliderComponent>(nullptr, ColliderType::AABB, 0.0f);
        auto groundBody = std::make_unique<RigidBody>(0.0f, RigidBodyType::STATIC);
        groundBody->setTransform(&groundTransform.getTransform());
        ground.addComponent<RigidBodyComponent>(std::move(groundBody));
    }

    return result;
}

void SceneBuilder::applyPlayerTransform(Player& player,
                                        const glm::vec2& worldSize,
                                        const glm::vec2& playerSize) {
    const glm::vec2 centered = calculateCenteredPosition(worldSize, playerSize);
    player.setSize(playerSize);
    player.setPosition(centered);
}
