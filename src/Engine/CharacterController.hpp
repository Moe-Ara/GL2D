#ifndef GL2D_CHARACTERCONTROLLER_HPP
#define GL2D_CHARACTERCONTROLLER_HPP

#include <glm/vec2.hpp>
#include <memory>
#include <vector>
#include "Engine/IController.hpp"
#include "Physics/PhysicsUnits.hpp"
#include "FeelingsSystem/FeelingSnapshot.hpp"

// Base movement controller that applies platformer-like motion using an intent
// source (input, AI, scripted, etc.). Override gatherIntent in derived classes.
class CharacterController : public IController {
public:
    struct Intent {
        float moveAxis{0.0f}; // -1 = left, +1 = right
        bool jumpPressed{false};
    };

    CharacterController() = default;
    ~CharacterController() override = default;

    void update(Entity& entity, double dt) override;
    void applyFeeling(const FeelingsSystem::FeelingSnapshot& snapshot);
    void resetFeelingOverrides();
    void setWorldEntities(std::vector<std::unique_ptr<Entity>>* world) { m_worldEntities = world; }

protected:
    virtual Intent gatherIntent(Entity& entity, double dt) = 0;
    virtual void onLanded(Entity& /*entity*/) {}
    virtual void onLeftGround(Entity& /*entity*/) {}

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

    // Baseline values for feelings overrides.
    float m_baseMoveSpeed{PhysicsUnits::toUnits(1.5f)};
    float m_baseAcceleration{PhysicsUnits::toUnits(20.0f)};
    float m_baseDeceleration{PhysicsUnits::toUnits(20.0f)};
    float m_baseJumpImpulse{PhysicsUnits::toUnits(4.0f)};
    float m_baseGravity{PhysicsUnits::toUnits(9.81f)};

    bool m_sensorCallbacksBound{false};
};

#endif // GL2D_CHARACTERCONTROLLER_HPP
