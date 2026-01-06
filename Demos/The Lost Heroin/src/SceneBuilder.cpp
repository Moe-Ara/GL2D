//
// Created by Mohamad on 06/01/2026.
//

#include "SceneBuilder.hpp"

#include "Player.hpp"
#include "Engine/Scene.hpp"
#include "GameObjects/Sprite.hpp"
#include "GameObjects/Components/FeelingsComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Components/SpriteComponent.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Components/LedgeComponent.hpp"
#include "InputSystem/InputService.hpp"
#include "Physics/RigidBody.hpp"
#include "FeelingsSystem/FeelingSnapshot.hpp"
#include "RenderingSystem/RenderLayers.hpp"
#include "Managers/TextureManager.hpp"

#include <filesystem>
#include <iostream>

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
    const std::filesystem::path metadataPath = "assets/women/Enchantress/animations.json";
    if (!std::filesystem::exists(metadataPath)) {
        std::cerr << "Missing animation metadata: " << metadataPath << std::endl;
        return result;
    }
    auto player = std::make_unique<Player>();
    if (!player->loadFromMetadata(metadataPath)) {
        std::cerr << "Failed to load player metadata" << std::endl;
        return result;
    }
    if (inputService) {
        player->attachController(*inputService);
    }
    FeelingsSystem::FeelingSnapshot playerFeeling{};
    playerFeeling.id = "Enchantress";
    playerFeeling.colorGrade = glm::vec4{1.0f, 0.96f, 0.92f, 1.0f};
    playerFeeling.lightIntensityMul = 1.1f;
    playerFeeling.ambientLightMul = glm::vec3{1.05f};
    playerFeeling.animationSpeedMul = 1.05f;
    playerFeeling.timeScale = 1.0f;
    playerFeeling.uiTint = glm::vec4{1.0f, 0.9f, 0.9f, 1.0f};
    player->addComponent<FeelingsComponent>(scene.feelings(), playerFeeling);
    result.player = player.get();
    scene.addEntity(std::move(player));
    if (result.player) {
        result.player->bindWorldEntities(&scene.getEntities());
        applyPlayerTransform(*result.player, worldSize, playerSize);
    }

    Entity &ground = scene.createEntity();
    auto &groundTransform = ground.addComponent<TransformComponent>();
    groundTransform.setPosition(glm::vec2{-800.0f, 0.0f});
    const glm::vec2 groundSize{2000.0f, 80.0f};
    std::shared_ptr<GameObjects::Texture> groundTexture;
    const std::filesystem::path relPath = "assets/PNG/BG_01/Layers/Ground_01.png";
    std::filesystem::path texturePath = relPath;
    if (!std::filesystem::exists(texturePath)) {
        texturePath = std::filesystem::path("Demos/The Lost Heroin") / relPath;
    }
    if (std::filesystem::exists(texturePath)) {
        groundTexture = Managers::TextureManager::loadTexture(texturePath.string());
    } else {
        std::cerr << "Missing ground texture: " << texturePath << std::endl;
    }
    if (groundTexture) {
        result.groundSprite = std::make_shared<GameObjects::Sprite>(
                groundTexture,
                groundTransform.getTransform().Position,
                groundSize);
    } else {
        result.groundSprite = std::make_shared<GameObjects::Sprite>(
                groundTransform.getTransform().Position,
                groundSize,
                glm::vec3{0.30f, 0.55f, 0.32f});
    }
    ground.addComponent<SpriteComponent>(result.groundSprite.get(),
                                         -1,
                                         static_cast<int>(Rendering::RenderLayer::Gameplay));
    ground.addComponent<ColliderComponent>(nullptr, ColliderType::AABB, 0.0f);
    auto groundBody = std::make_unique<RigidBody>(0.0f, RigidBodyType::STATIC);
    groundBody->setTransform(&groundTransform.getTransform());
    ground.addComponent<RigidBodyComponent>(std::move(groundBody));
//    ground.addComponent<LedgeComponent>();

    return result;
}

void SceneBuilder::applyPlayerTransform(Player& player,
                                        const glm::vec2& worldSize,
                                        const glm::vec2& playerSize) {
    const glm::vec2 centered = calculateCenteredPosition(worldSize, playerSize);
    player.setSize(playerSize);
    player.setPosition(centered);
}
