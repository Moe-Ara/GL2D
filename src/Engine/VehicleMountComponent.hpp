#ifndef GL2D_VEHICLEMOUNTCOMPONENT_HPP
#define GL2D_VEHICLEMOUNTCOMPONENT_HPP

#include "GameObjects/IComponent.hpp"

#include <glm/vec2.hpp>
#include <string>

#include "InputSystem/InputService.hpp"
#include "Physics/PhysicsUnits.hpp"

class VehicleController;
class Entity;
class Camera;

// Mount/dismount helper: waits for a Jump press near the vehicle, then mounts
// the provided rider and disables their controller until they dismount.
class VehicleMountComponent : public IUpdatableComponent {
public:
    explicit VehicleMountComponent(InputService& inputService, Entity* rider = nullptr)
        : m_inputService(inputService), m_rider(rider) {}
    ~VehicleMountComponent() override = default;

    void update(Entity& owner, double dt) override;

    void setRider(Entity* rider) { m_rider = rider; }
    void setMountRadius(float radius) { m_mountRadius = radius; }
    void setSeatOffset(const glm::vec2& offset) { m_seatOffset = offset; }
    void setDebugCamera(Camera* cam) { m_debugCamera = cam; }
    void setInteractActionName(const std::string& name) { m_interactAction = name; }

private:
    bool isInteractPressedThisFrame() const;
    bool riderIsClose(Entity& owner) const;
    VehicleController* resolveVehicleController(Entity& owner) const;
    void drawRadius(Entity& owner) const;

    InputService& m_inputService;
    Entity* m_rider{nullptr};
    float m_mountRadius{PhysicsUnits::toUnits(1.2f)};
    glm::vec2 m_seatOffset{0.0f, PhysicsUnits::toUnits(0.7f)};
    bool m_mounted{false};
    Camera* m_debugCamera{nullptr};
    std::string m_interactAction{"Interact"};
};

#endif // GL2D_VEHICLEMOUNTCOMPONENT_HPP
