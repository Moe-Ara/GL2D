#ifndef GL2D_CHARACTERCONTROLLER_HPP
#define GL2D_CHARACTERCONTROLLER_HPP

#include <glm/vec2.hpp>
#include <memory>
#include <vector>
#include "Engine/IController.hpp"
#include "Physics/PhysicsUnits.hpp"
#include "FeelingsSystem/FeelingSnapshot.hpp"

class Entity;
class TransformComponent;
class LedgeSensorComponent;
class RigidBody;
namespace GameObjects {
    class Sprite;
}

// Base movement controller that applies platformer-like motion using an intent
// source (input, AI, scripted, etc.). Override gatherIntent in derived classes.
class CharacterController : public IController {
public:
    struct Intent {
        float moveAxis{0.0f}; // -1 = left, +1 = right
        bool jumpPressed{false};
        float climbAxis{0.0f}; // +1 = up, -1 = down
    };

    struct MovementConfig {
        float moveSpeed{PhysicsUnits::toUnits(1.5f)};
        float acceleration{PhysicsUnits::toUnits(20.0f)};
        float deceleration{PhysicsUnits::toUnits(20.0f)};
        float jumpImpulse{PhysicsUnits::toUnits(4.0f)};
        float gravity{PhysicsUnits::toUnits(9.81f)};
        float walkSpeedMultiplier{0.5f};
        float walkAxisThreshold{0.6f};
        float runAxisThreshold{0.85f};
        float climbSpeed{PhysicsUnits::toUnits(1.75f)};
        float climbAcceleration{PhysicsUnits::toUnits(18.0f)};
        float ledgeProbeDistance{PhysicsUnits::toUnits(0.6f)};
        float ledgeHorizontalOffset{PhysicsUnits::toUnits(0.25f)};
        float ledgeVerticalOffset{PhysicsUnits::toUnits(0.65f)};
        float hangVerticalOffset{PhysicsUnits::toUnits(0.1f)};
        float hangClimbOffset{PhysicsUnits::toUnits(0.4f)};
    };

    enum class MoveMode { Idle, Walk, Run };

    CharacterController() = default;
    ~CharacterController() override = default;

    void update(Entity& entity, double dt) override;
    void applyFeeling(const FeelingsSystem::FeelingSnapshot& snapshot);
    void resetFeelingOverrides();
    void setWorldEntities(std::vector<std::unique_ptr<Entity>>* world);
    void resetVelocity();
    void setVelocity(const glm::vec2& velocity);
    MoveMode currentMoveMode() const { return m_lastMoveMode; }
    void setClimbSpeed(float speed) { m_climbSpeed = speed; }
    void setClimbAcceleration(float accel) { m_climbAcceleration = accel; }
    void configureMovement(const MovementConfig &config);
    void setMaxMoveSpeed(float speed);
    float facingDirection() const { return static_cast<float>(m_faceDirection); }
    bool isHanging() const { return m_isHanging; }

protected:
    virtual Intent gatherIntent(Entity& entity, double dt) = 0;
    virtual void onLanded(Entity& /*entity*/) {}
    virtual void onLeftGround(Entity& /*entity*/) {}
    void ensureLedgeSensor(Entity& owner);
    void startHangState(const glm::vec2& point,
                        TransformComponent* transform,
                        RigidBody* body,
                        GameObjects::Sprite* sprite,
                        float ledgeHangOffset);
    bool handleHangInput(const Intent& intent,
                         TransformComponent* transform,
                         RigidBody* body,
                         GameObjects::Sprite* sprite);
    void releaseHang(bool jump,
                     bool climb,
                     TransformComponent* transform,
                     RigidBody* body,
                     GameObjects::Sprite* sprite);
    glm::vec2 hangTransformPosition() const;
    glm::vec2 climbTransformPosition() const;

    std::vector<std::unique_ptr<Entity>>* m_worldEntities{nullptr};
    glm::vec2 m_velocity{0.0f};
    float m_damage{1.0f};
    float m_armor{100.0f};
    float m_groundLevel{0.0f};
    bool m_groundInitialized{false};
    bool m_isGrounded{true};
    bool m_wallContact{false};
    glm::vec2 m_wallNormal{0.0f};
    float m_moveSpeed{PhysicsUnits::toUnits(1.5f)};
    float m_acceleration{PhysicsUnits::toUnits(20.0f)};
    float m_deceleration{PhysicsUnits::toUnits(20.0f)};
    float m_jumpImpulse{PhysicsUnits::toUnits(4.0f)};
    float m_gravity{PhysicsUnits::toUnits(9.81f)};
    float m_walkSpeedMultiplier{0.5f}; // fraction of run speed when stick is slightly tilted
    float m_walkAxisThreshold{0.6f};   // analog magnitude at/under this stays in walk
    float m_runAxisThreshold{0.85f};   // analog magnitude at/over this enters run (hysteresis to prevent flicker)
    float m_climbSpeed{PhysicsUnits::toUnits(1.75f)};
    float m_climbAcceleration{PhysicsUnits::toUnits(18.0f)};
    bool m_isClimbing{false};
    MoveMode m_lastMoveMode{MoveMode::Idle};
    int m_faceDirection{1};

    // Baseline values for feelings overrides.
    float m_baseMoveSpeed{PhysicsUnits::toUnits(1.5f)};
    float m_baseAcceleration{PhysicsUnits::toUnits(20.0f)};
    float m_baseDeceleration{PhysicsUnits::toUnits(20.0f)};
    float m_baseJumpImpulse{PhysicsUnits::toUnits(4.0f)};
    float m_baseGravity{PhysicsUnits::toUnits(9.81f)};

    bool m_sensorCallbacksBound{false};
    MovementConfig m_movementConfig{};
    bool m_isHanging{false};
    glm::vec2 m_hangPoint{0.0f};
    Entity* m_hangEntity{nullptr};
    float m_hangEntityOffset{0.0f};
    LedgeSensorComponent* m_ledgeSensor{nullptr};
};

#endif // GL2D_CHARACTERCONTROLLER_HPP
