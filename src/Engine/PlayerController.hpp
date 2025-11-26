#ifndef PLAYER_CONTROLLER_HPP
#define PLAYER_CONTROLLER_HPP

#include <glm/vec2.hpp>

#include "GameObjects/Entity.hpp"
#include "IController.hpp"
#include "InputSystem/InputService.hpp"
#include "InputSystem/InputTypes.hpp"
#include "Physics/PhysicsUnits.hpp"
#include "FeelingsSystem/FeelingSnapshot.hpp"

class PlayerController : public IController {
public:
    explicit PlayerController(InputService &inputService)
            : m_inputService(inputService) {}
    ~PlayerController() override = default;

    void update(Entity &entity, double dt) override;
    void applyFeeling(const FeelingsSystem::FeelingSnapshot& snapshot);
    void resetFeelingOverrides();

private:
    void consumeActionEvent(const ActionEvent &event);

    InputService &m_inputService;
    glm::vec2 m_velocity{0.0f};
    float m_damage{1.f};
    float m_armor{100.f};
    float m_groundLevel{0.0f};
    bool m_groundInitialized{false};
    bool m_moveLeft{false};
    bool m_moveRight{false};
    bool m_jumpQueued{false};
    bool m_isGrounded{true};
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
};

#endif
