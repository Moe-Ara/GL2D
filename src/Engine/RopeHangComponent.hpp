#ifndef GL2D_ROPEHANGCOMPONENT_HPP
#define GL2D_ROPEHANGCOMPONENT_HPP

#include "GameObjects/IComponent.hpp"
#include "GameObjects/Components/RopeSegmentComponent.hpp"
#include "InputSystem/InputTypes.hpp"

#include <glm/vec2.hpp>
#include <memory>
#include <string>
#include <vector>

class ControllerComponent;
class Entity;
class InputService;

// Detects nearby ropes, disables the main controller while the player hangs, and
// drives movement along the rope until the player releases.
class RopeHangComponent : public IUpdatableComponent {
public:
    RopeHangComponent(InputService& inputService,
                      ControllerComponent& controller,
                      std::vector<std::unique_ptr<Entity>>* worldEntities);

    void update(Entity& owner, double dt) override;

    void setDetectionRadius(float radius) { m_detectionRadius = radius; }
    void setClimbSpeed(float speed) { m_climbSpeed = speed; }
    void setGrabAction(const std::string& action) { m_grabAction = action; }
    void setReleaseAction(const std::string& action) { m_releaseAction = action; }

private:
    void consumeActionEvent(const ActionEvent& event);
    Entity* findNearbyRope(Entity& owner) const;
    void applyRopePosition(Entity& owner);
    void startHang(Entity& owner, Entity& ropeEntity, RopeSegmentComponent& ropeInfo);
    void stopHang(Entity& owner);
    void updateHangMovement(Entity& owner, double dt);

    InputService& m_inputService;
    ControllerComponent& m_controller;
    std::vector<std::unique_ptr<Entity>>* m_worldEntities{nullptr};

    float m_detectionRadius{0.0f};
    float m_climbSpeed{0.0f};
    std::string m_grabAction{"Interact"};
    std::string m_releaseAction{"Jump"};

    bool m_isHanging{false};
    Entity* m_currentSegment{nullptr};
    float m_ropeLength{0.0f};
    float m_ropeParam{0.0f};
    glm::vec2 m_ropeDirection{0.0f, -1.0f};
    glm::vec2 m_ropeTop{0.0f};
    glm::vec2 m_ropeBottom{0.0f};

    bool m_grabRequested{false};
    bool m_releaseRequested{false};
    float m_axisY{0.0f};
    bool m_axisUpdated{false};
    bool m_moveUp{false};
    bool m_moveDown{false};
};

#endif // GL2D_ROPEHANGCOMPONENT_HPP
