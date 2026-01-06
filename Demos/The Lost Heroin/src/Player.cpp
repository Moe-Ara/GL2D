#include "Player.hpp"

#include "GameObjects/Sprite.hpp"
#include "GameObjects/Texture.hpp"
#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "GameObjects/Components/SpriteComponent.hpp"
#include "GameObjects/Components/AnimatorComponent.hpp"
#include "GameObjects/Components/ControllerComponent.hpp"
#include "GameObjects/Components/CombatComponent.hpp"
#include "GameObjects/Components/HealthComponent.hpp"
#include "GameObjects/Components/GroundSensorComponent.hpp"
#include "GameObjects/Components/LedgeSensorComponent.hpp"
#include "Graphics/Animation/Animation.hpp"
#include "Graphics/Animation/Loaders/AnimationMetadataLoader.hpp"
#include "Managers/TextureManager.hpp"
#include "Utils/SimpleJson.hpp"
#include "Config/GameConfig.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include "Physics/RigidBody.hpp"
#include "Physics/PhysicsUnits.hpp"
#include "Engine/PlayerController.hpp"
#include "InputSystem/InputService.hpp"
#include "Engine/CharacterController.hpp"

#include <optional>
#include <filesystem>
#include <unordered_map>
#include <cmath>
#include <iostream>

namespace {

std::filesystem::path resolveMetadataDirectory(const std::filesystem::path &metadataPath) {
    auto dir = metadataPath.parent_path();
    if (dir.empty()) {
        dir = std::filesystem::current_path();
    }
    if (!dir.is_absolute()) {
        dir = std::filesystem::absolute(dir);
    }
    return dir;
}

std::string resolveAssetPath(const std::filesystem::path &metadataDir, const std::string &assetPath) {
    if (assetPath.empty()) {
        return {};
    }
    std::filesystem::path candidate(assetPath);
    if (candidate.is_absolute()) {
        return candidate.string();
    }
    if (assetPath.rfind("assets/", 0) == 0) {
        return std::filesystem::absolute(candidate).lexically_normal().string();
    }
    return (metadataDir / candidate).lexically_normal().string();
}

std::shared_ptr<GameObjects::Texture> cacheTexture(
        std::unordered_map<std::string, std::shared_ptr<GameObjects::Texture>> &cache,
        const std::string &path,
        const std::filesystem::path &metadataDir) {
    if (path.empty()) {
        return nullptr;
    }
    const auto resolvedPath = resolveAssetPath(metadataDir, path);
    auto it = cache.find(resolvedPath);
    if (it != cache.end()) {
        return it->second;
    }
    auto texture = Managers::TextureManager::loadTexture(resolvedPath);
    cache[resolvedPath] = texture;
    return texture;
}

std::optional<CharacterController::MovementConfig> loadPlayerMovementConfig() {
    return Config::GameConfig::player().movement;
}

} // namespace

Player::Player() = default;

bool Player::loadFromMetadata(const std::filesystem::path &metadataPath) {
    auto result = loadAnimationsFromMetadata(metadataPath);
    if (result.animations.empty()) {
        return false;
    }

    m_animations = std::move(result.animations);
    std::string initialState = !result.initialState.empty() ? result.initialState
                                                            : m_animations.begin()->first;
    if (m_animations.find(initialState) == m_animations.end()) {
        initialState = m_animations.begin()->first;
    }

    if (!m_sprite) {
        const glm::vec2 defaultSize{512.0f, 512.0f};
        m_sprite = std::make_shared<GameObjects::Sprite>(glm::vec2(0.0f), defaultSize, glm::vec3(1.0f));
    }

    if (!m_spriteComponent) {
        m_spriteComponent = &addComponent<SpriteComponent>(m_sprite.get(), 0);
    } else {
        m_spriteComponent->setSprite(m_sprite.get());
    }

    if (!m_transformComponent) {
        m_transformComponent = &addComponent<TransformComponent>();
    }

    if (!m_colliderComponent) {
        m_colliderComponent = &addComponent<ColliderComponent>();
        m_colliderComponent->setRequestedType(ColliderType::CAPSULE);
        m_colliderComponent->setCapsuleSizeOverride(glm::vec2{150.0f, 70.0f});
        m_colliderComponent->setCapsuleOffsetOverride(glm::vec2{0.f, -90.0f});
    }
    if (m_colliderComponent) {
        m_colliderComponent->ensureCollider(*this);
    }

    if (!m_rigidBodyComponent) {
        auto body = std::make_unique<RigidBody>(1.0f, RigidBodyType::DYNAMIC);
        body->setLinearDamping(6.0f);
        m_rigidBodyComponent = &addComponent<RigidBodyComponent>(std::move(body));
    }
    if (m_rigidBodyComponent) {
        m_rigidBodyComponent->ensureBound(*this);
    }
    if (!m_groundSensor) {
        m_groundSensor = &addComponent<GroundSensorComponent>();
    }
    if (!m_ledgeSensor) {
        m_ledgeSensor = &addComponent<LedgeSensorComponent>();
    }
    if(!m_combatComponent){
        m_combatComponent=&addComponent<CombatComponent>();
    }
    if(!m_healthComponent){
        m_healthComponent=&addComponent<HealthComponent>();
    }
    if (!m_animatorComponent) {
        m_animatorComponent = &addComponent<AnimatorComponent>();
    }
    m_animatorComponent->setSprite(m_sprite);

    const auto &playerCfg = Config::GameConfig::player();
    if (m_combatComponent) {
        m_combatComponent->setRange(playerCfg.combat.attackRange);
        m_combatComponent->setCooldown(playerCfg.combat.attackCooldown);
        m_combatComponent->setDamage(playerCfg.combat.attackDamage);
    }
    if (m_healthComponent) {
        m_healthComponent->setMaxHp(playerCfg.health.maxHp);
        m_healthComponent->setArmor(playerCfg.health.armor);
    }

    applyAnimation(initialState);
    m_currentMovementAnimation = initialState;
    return true;
}

void Player::update(double deltaTime) {
    Entity::update(deltaTime);
    updateMovementAnimation();

    if (!m_sprite) {
        return;
    }

    constexpr float kFacingVelocityThreshold = 0.01f;
    float desiredFacing = m_lastFacingDirection;

    if (m_rigidBodyComponent && m_rigidBodyComponent->body()) {
        const float velocityX = m_rigidBodyComponent->body()->getVelocity().x;
        if (std::abs(velocityX) > kFacingVelocityThreshold) {
            desiredFacing = velocityX < 0.0f ? -1.0f : 1.0f;
        }
    }

    if (desiredFacing == m_lastFacingDirection && m_controllerComponent) {
        if (auto *controller = dynamic_cast<CharacterController *>(m_controllerComponent->controller())) {
            desiredFacing = controller->facingDirection();
        }
    }

    if (desiredFacing != 0.0f) {
        m_lastFacingDirection = desiredFacing;
    }

    if (m_sprite) {
        m_sprite->setFlipX(m_lastFacingDirection < 0.0f);
    }
}

void Player::setSize(const glm::vec2 &size) {
    if (m_sprite) {
        m_sprite->setSize(size);
    }
    if (m_transformComponent) {
        m_transformComponent->setScale(glm::vec2(1.0f));
    }
    if (m_colliderComponent) {
        m_colliderComponent->fitToSprite(*this);
    }
}

void Player::setPosition(const glm::vec2 &position) {
    if (m_transformComponent) {
        m_transformComponent->setPosition(position);
    }
    if (m_sprite) {
        m_sprite->setPosition(position);
    }
    if (m_rigidBodyComponent && m_rigidBodyComponent->body()) {
        m_rigidBodyComponent->body()->setPosition(position);
    }
}

std::shared_ptr<GameObjects::Sprite> Player::getSprite() const {
    return m_sprite;
}


const glm::mat4 &Player::getModelMatrix() const {
    static const glm::mat4 identity(1.0f);
    if (m_transformComponent) {
        return m_transformComponent->modelMatrix();
    }
    return identity;
}

TransformComponent *Player::getTransformComponent() {
    return m_transformComponent;
}

void Player::attachController(InputService &inputService) {
    if (!m_controllerComponent) {
        auto controller = std::make_unique<PlayerController>(inputService);
        if (auto config = loadPlayerMovementConfig()) {
            controller->configureMovement(*config);
        }
        m_controllerComponent = &addComponent<ControllerComponent>(std::move(controller));
    }
}

void Player::bindWorldEntities(std::vector<std::unique_ptr<Entity>>* world) {
    if (m_groundSensor) {
        m_groundSensor->setWorldEntities(world);
    }
    if (m_ledgeSensor) {
        m_ledgeSensor->setWorldEntities(world);
    }
    if (m_controllerComponent) {
        if (auto controller = dynamic_cast<CharacterController *>(m_controllerComponent->controller())) {
            controller->setWorldEntities(world);
        }
    }
}

Player::AnimationLoadResult Player::loadAnimationsFromMetadata(
        const std::filesystem::path &metadataPath) {
    AnimationLoadResult result{};
    try {
        auto metadata = Loaders::AnimationMetadataLoader::loadFromFile(metadataPath.string());
        result.initialState = metadata.initialState;

        std::unordered_map<std::string, std::shared_ptr<GameObjects::Texture>> textureCache;
        const auto metadataDir = resolveMetadataDirectory(metadataPath);
        auto atlasTexture = cacheTexture(textureCache, metadata.atlas.texturePath, metadataDir);

        for (const auto &entry : metadata.animations) {
            if (entry.frames.empty()) {
                continue;
            }

            const float animationFrameDuration = entry.defaultFrameDuration > 0.0f
                                                         ? entry.defaultFrameDuration
                                                         : metadata.defaultFrameDuration;

            auto animation = std::make_shared<Graphics::Animation>(
                    metadata.atlas.rows, metadata.atlas.cols, animationFrameDuration,
                    entry.loop, entry.playbackMode);

            animation->setName(entry.name);
            animation->setSharedTexture(atlasTexture);
            animation->setFrameDuration(animationFrameDuration);

        for (const auto &transition : entry.transitions) {
            animation->addTransition(Graphics::AnimationTransition{
                    transition.target, transition.condition});
        }

            for (const auto &frameMeta : entry.frames) {
                Graphics::Frame frame{};
                frame.row = frameMeta.row;
                frame.column = frameMeta.column;
                frame.useCustomUV = frameMeta.useCustomUV;
                frame.uvRect = frameMeta.uvRect;
                frame.duration = frameMeta.duration > 0.0f ? frameMeta.duration
                                                           : animationFrameDuration;
                frame.eventName = frameMeta.eventName;
                if (!frameMeta.texturePath.empty()) {
                    frame.texture = cacheTexture(textureCache, frameMeta.texturePath, metadataDir);
                } else if (atlasTexture) {
                    frame.texture = atlasTexture;
                }
                animation->addFrame(frame);
            }

            result.animations[entry.name] = animation;
        }
    } catch (const Utils::JsonParseException &ex) {
        std::cerr << "Failed to parse animation metadata (" << metadataPath << "): "
                  << ex.what() << std::endl;
    } catch (const std::exception &ex) {
        std::cerr << "Failed to load animation metadata (" << metadataPath << "): "
                  << ex.what() << std::endl;
    }

    return result;
}

void Player::applyAnimation(const std::string &state) {
    if (m_animatorComponent == nullptr) {
        return;
    }
    const auto it = m_animations.find(state);
    if (it == m_animations.end()) {
        return;
    }

    constexpr float kAnimationCrossfade = 0.15f;
    m_animatorComponent->play(it->second, kAnimationCrossfade);
}

void Player::updateMovementAnimation() {
    if (!m_controllerComponent) {
        return;
    }
    auto *controller = dynamic_cast<CharacterController *>(m_controllerComponent->controller());
    if (!controller) {
        return;
    }
    std::string desired;
    if (controller->isHanging()) {
        desired = "Hang";
    } else {
        desired = animationForMoveMode(controller->currentMoveMode());
    }
    if (desired.empty() || desired == m_currentMovementAnimation) {
        return;
    }
    const auto it = m_animations.find(desired);
    if (it == m_animations.end()) {
        return;
    }
    applyAnimation(desired);
    m_currentMovementAnimation = desired;
}

std::string Player::animationForMoveMode(CharacterController::MoveMode mode) const {
    switch (mode) {
        case CharacterController::MoveMode::Idle:
            return "Idle";
        case CharacterController::MoveMode::Walk:
            return "Walk";
        case CharacterController::MoveMode::Run:
            return "Run";
    }
    return {};
}
