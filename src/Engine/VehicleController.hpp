#ifndef GL2D_VEHICLECONTROLLER_HPP
#define GL2D_VEHICLECONTROLLER_HPP

#include "Engine/IController.hpp"

#include <glm/vec2.hpp>
#include "InputSystem/InputService.hpp"
#include "Physics/PhysicsUnits.hpp"

class ControllerComponent;
class TransformComponent;
class CharacterController;
class Entity;

// Drives a vehicle entity using player input, but only while mounted.
// On mount it disables the rider's controller; on dismount it re-enables it.
class VehicleController : public IController {
public:
    explicit VehicleController(InputService& inputService)
        : m_inputService(inputService) {}
    ~VehicleController() override = default;

    void update(Entity& entity, double dt) override;

    void mount(Entity& rider);
    void dismount();
    bool isMounted() const { return m_rider != nullptr; }

    void setMaxSpeed(float speed) { m_maxSpeed = speed; }
    void setAcceleration(float accel) { m_acceleration = accel; }
    void setBrakeDrag(float drag) { m_brakeDrag = drag; }
    void setSeatOffset(const glm::vec2& offset) { m_seatOffset = offset; }
    void setMinSubmersionForMotion(float value) { m_minSubmersionForMotion = value; }

private:
    void consumeActionEvent(const ActionEvent& event);
    void syncRiderToSeat(Entity& vehicle);

    InputService& m_inputService;
    Entity* m_rider{nullptr};
    ControllerComponent* m_riderController{nullptr};
    glm::vec2 m_seatOffset{0.0f, PhysicsUnits::toUnits(0.7f)};

    bool m_moveLeft{false};
    bool m_moveRight{false};
    bool m_jumpPressed{false};
    float m_axisX{0.0f};
    bool m_axisUpdated{false};

    float m_maxSpeed{PhysicsUnits::toUnits(10.0f)};
    float m_acceleration{PhysicsUnits::toUnits(40.0f)};
    float m_brakeDrag{6.0f};
    float m_minSubmersionForMotion{0.05f};
    CharacterController* m_riderCharacterController{nullptr};
    bool m_resetVelocityOnMount{false};
};

#endif // GL2D_VEHICLECONTROLLER_HPP
