#ifndef GL2D_ROPEHANGCOMPONENT_HPP
#define GL2D_ROPEHANGCOMPONENT_HPP

#include "GameObjects/IComponent.hpp"
#include "GameObjects/Components/RopeSegmentComponent.hpp"
#include "InputSystem/InputTypes.hpp"
#include "Physics/RigidBody.hpp"

#include <glm/vec2.hpp>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <limits>

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

    void setDetectionRadius(float radius);
    void setClimbSpeed(float speed);
    void setReleaseSpeed(float speed);
    void setGrabAction(const std::string& action) { m_grabAction = action; }
    void setReleaseAction(const std::string& action) { m_releaseAction = action; }
    [[nodiscard]] bool isHanging() const noexcept { return m_isHanging; }

private:
    void consumeActionEvent(const ActionEvent& event);
    Entity* findNearbyRope(Entity& owner) const;
    bool applyRopePosition(Entity& owner);
    void startHang(Entity& owner, Entity& ropeEntity, RopeSegmentComponent& ropeInfo);
    void stopHang(Entity& owner, bool jumpRelease = false);
    void updateHangMovement(Entity& owner, double dt);
    [[nodiscard]] bool containsWorldEntity(const Entity* entity) const;
    [[nodiscard]] RopeSegmentComponent* ropeInfo(Entity* entity) const;

    InputService& m_inputService;
    ControllerComponent& m_controller;
    std::vector<std::unique_ptr<Entity>>* m_worldEntities{nullptr};

    float m_detectionRadius{0.0f};
    float m_climbSpeed{0.0f};
    float m_releaseSpeed{0.0f};
    std::string m_grabAction{"Interact"};
    std::string m_releaseAction{"Jump"};

    bool m_isHanging{false};
    Entity* m_currentSegment{nullptr};
    float m_segmentDistance{0.0f};
    RigidBodyType m_previousBodyType{RigidBodyType::DYNAMIC};
    float m_previousGravityScale{1.0f};
    bool m_controllerWasEnabled{true};

    bool m_grabRequested{false};
    bool m_releaseRequested{false};
    float m_axisY{0.0f};
    bool m_axisUpdated{false};
    bool m_moveUp{false};
    bool m_moveDown{false};
    std::uint64_t m_lastActionFrame{std::numeric_limits<std::uint64_t>::max()};
};

#endif // GL2D_ROPEHANGCOMPONENT_HPP
