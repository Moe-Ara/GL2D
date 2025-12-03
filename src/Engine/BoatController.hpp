#ifndef GL2D_BOATCONTROLLER_HPP
#define GL2D_BOATCONTROLLER_HPP

#include "Engine/IController.hpp"

#include <glm/vec2.hpp>

#include "InputSystem/InputService.hpp"
#include "Physics/PhysicsUnits.hpp"

// Simple boat controller that steers a rigid body while respecting water flow.
// Requires WaterStateComponent to get submersion/flow info.
class BoatController : public IController {
public:
    explicit BoatController(InputService& inputService)
        : m_inputService(inputService) {}
    ~BoatController() override = default;

    void update(Entity& entity, double dt) override;

    void setMaxSpeed(float speed) { m_maxSpeed = speed; }
    void setAcceleration(float accel) { m_acceleration = accel; }
    void setTurnDrag(float drag) { m_turnDrag = drag; }
    void setBoostImpulse(float impulse) { m_boostImpulse = impulse; }
    void setFlowMatch(float flowMatch) { m_flowMatch = flowMatch; }

private:
    void consumeActionEvent(const ActionEvent& event);

    InputService& m_inputService;
    bool m_moveLeft{false};
    bool m_moveRight{false};
    bool m_boostQueued{false};
    float m_axisX{0.0f};
    bool m_axisUpdated{false};

    float m_maxSpeed{PhysicsUnits::toUnits(8.0f)};
    float m_acceleration{PhysicsUnits::toUnits(36.0f)};
    float m_turnDrag{3.5f};
    float m_boostImpulse{PhysicsUnits::toUnits(4.5f)};
    float m_flowMatch{0.65f};
};

#endif // GL2D_BOATCONTROLLER_HPP
