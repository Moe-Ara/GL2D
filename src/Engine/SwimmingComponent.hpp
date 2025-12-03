#ifndef GL2D_SWIMMINGCOMPONENT_HPP
#define GL2D_SWIMMINGCOMPONENT_HPP

#include "GameObjects/IComponent.hpp"

#include <glm/vec2.hpp>

#include "InputSystem/InputService.hpp"
#include "Physics/PhysicsUnits.hpp"

class Entity;

// Adds swim locomotion on top of regular movement when the entity is submerged
// (requires WaterStateComponent + RigidBodyComponent).
class SwimmingComponent : public IUpdatableComponent {
public:
    explicit SwimmingComponent(InputService& inputService)
        : m_inputService(inputService) {}

    void update(Entity& owner, double dt) override;

    void setSwimSpeed(float speed) { m_swimSpeed = speed; }
    void setSwimAcceleration(float accel) { m_swimAcceleration = accel; }
    void setUpStrokeImpulse(float impulse) { m_upStrokeImpulse = impulse; }
    void setFlowAdherence(float factor) { m_flowAdherence = factor; }
    void setActivationDepth(float depth) { m_activationSubmersion = depth; }

private:
    void consumeActionEvent(const ActionEvent& event);

    InputService& m_inputService;
    bool m_moveLeft{false};
    bool m_moveRight{false};
    bool m_swimUpQueued{false};
    float m_axisX{0.0f};
    bool m_axisUpdated{false};

    float m_swimSpeed{PhysicsUnits::toUnits(3.5f)};
    float m_swimAcceleration{PhysicsUnits::toUnits(24.0f)};
    float m_upStrokeImpulse{PhysicsUnits::toUnits(3.0f)};
    float m_flowAdherence{0.35f};
    float m_activationSubmersion{0.15f};
    float m_maxUpwardSpeed{PhysicsUnits::toUnits(6.0f)};
    float m_maxDownwardSpeed{PhysicsUnits::toUnits(10.0f)};
};

#endif // GL2D_SWIMMINGCOMPONENT_HPP
