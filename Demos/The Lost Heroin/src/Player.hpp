//
// Created by Mohamad on 04/12/2025.
//

#ifndef GL2D_PLAYER_HPP
#define GL2D_PLAYER_HPP

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <memory>
#include <map>
#include <string>
#include <vector>
#include <filesystem>

#include "GameObjects/Entity.hpp"

#include "GameObjects/Components/AnimatorComponent.hpp"
#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "GameObjects/Components/SpriteComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Components/ControllerComponent.hpp"
#include "GameObjects/Components/HealthComponent.hpp"
#include "GameObjects/Components/CombatComponent.hpp"
#include "Engine/CharacterController.hpp"
#include "InputSystem/InputService.hpp"

namespace GameObjects {
    class Sprite;
    class Texture;
}

namespace Graphics {
    class Animation;
}

class GroundSensorComponent;
class LedgeSensorComponent;

class Player : public Entity {
public:
    Player();
    ~Player() override = default;

    bool loadFromMetadata(const std::filesystem::path &metadataPath);
    void update(double deltaTime) override;

    void setSize(const glm::vec2 &size);
    void setPosition(const glm::vec2 &position);
    void attachController(InputService &inputService);
    void bindWorldEntities(std::vector<std::unique_ptr<Entity>>* world);

    std::shared_ptr<GameObjects::Sprite> getSprite() const;
    const glm::mat4 &getModelMatrix() const;
    TransformComponent *getTransformComponent();

private:
    struct AnimationLoadResult {
        std::map<std::string, std::shared_ptr<Graphics::Animation>> animations;
        std::string initialState;
    };

    static AnimationLoadResult loadAnimationsFromMetadata(const std::filesystem::path &metadataPath);
    void applyAnimation(const std::string &state);
    void updateMovementAnimation();
    std::string animationForMoveMode(CharacterController::MoveMode mode) const;

    std::shared_ptr<GameObjects::Sprite> m_sprite;
    std::map<std::string, std::shared_ptr<Graphics::Animation>> m_animations;
    SpriteComponent *m_spriteComponent{nullptr};
    TransformComponent *m_transformComponent{nullptr};
    ColliderComponent *m_colliderComponent{nullptr};
    RigidBodyComponent *m_rigidBodyComponent{nullptr};
    AnimatorComponent *m_animatorComponent{nullptr};
    ControllerComponent* m_controllerComponent{nullptr};
    HealthComponent* m_healthComponent{nullptr};
    CombatComponent* m_combatComponent{nullptr};
    std::string m_currentMovementAnimation;
    float m_lastFacingDirection{1.0f};
    GroundSensorComponent* m_groundSensor{nullptr};
    LedgeSensorComponent* m_ledgeSensor{nullptr};


};

#endif //GL2D_PLAYER_HPP
