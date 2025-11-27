#ifndef GL2D_GROUNDSENSORCOMPONENT_HPP
#define GL2D_GROUNDSENSORCOMPONENT_HPP

#include "GameObjects/IComponent.hpp"
#include "Physics/PhysicsUnits.hpp"

#include <functional>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <memory>
#include <vector>

class Entity;

// Performs ground and wall sensing using ray casts with layer masks. Emits
// landed/left-ground callbacks when state changes.
class GroundSensorComponent : public IUpdatableComponent {
public:
    struct HitInfo {
        bool hit{false};
        glm::vec2 normal{0.0f};
        glm::vec2 point{0.0f};
        float distance{0.0f};
    };

    void update(Entity& owner, double /*dt*/) override;
    void refresh(Entity& owner); // Force a sense pass (useful if update order is uncertain).

    void setWorldEntities(std::vector<std::unique_ptr<Entity>>* world) { m_worldEntities = world; }
    void setLayerMasks(uint32_t groundMask, uint32_t wallMask) { m_groundLayerMask = groundMask; m_wallLayerMask = wallMask; }
    void setProbeDistances(float groundDistance, float wallDistance) { m_groundProbeDistance = groundDistance; m_wallProbeDistance = wallDistance; }
    void setMinGroundNormalDot(float dot) { m_minGroundNormalDot = dot; }
    void setOffsets(float groundOffset, float wallOffset) { m_groundOffset = groundOffset; m_wallOffset = wallOffset; }
    void setCallbacks(std::function<void(Entity&)> landed, std::function<void(Entity&)> leftGround) {
        m_onLanded = std::move(landed);
        m_onLeftGround = std::move(leftGround);
    }

    bool isGrounded() const { return m_grounded; }
    bool justLanded() const { return m_justLanded; }
    bool justLeftGround() const { return m_justLeftGround; }
    bool hasWallContact() const { return m_wallContact; }
    glm::vec2 wallNormal() const { return m_wallNormal; }
    const HitInfo& groundHit() const { return m_groundHit; }
    const HitInfo& leftWallHit() const { return m_leftWallHit; }
    const HitInfo& rightWallHit() const { return m_rightWallHit; }
    void debugDraw(const glm::mat4& viewProj,
                   Entity& owner,
                   const glm::vec3& groundColor = {0.2f, 0.9f, 0.2f},
                   const glm::vec3& wallColor = {0.2f, 0.6f, 1.0f}) const;

private:
    HitInfo castSensor(const glm::vec2& origin,
                       const glm::vec2& dir,
                       float maxDistance,
                       uint32_t mask,
                       Entity& owner) const;
    void updateGroundState(Entity& owner, bool groundedNow);

    std::vector<std::unique_ptr<Entity>>* m_worldEntities{nullptr};

    HitInfo m_groundHit{};
    HitInfo m_leftWallHit{};
    HitInfo m_rightWallHit{};

    bool m_grounded{false};
    bool m_justLanded{false};
    bool m_justLeftGround{false};
    bool m_wallContact{false};
    glm::vec2 m_wallNormal{0.0f};
    bool m_wasGrounded{false};

    float m_groundProbeDistance{PhysicsUnits::toUnits(0.25f)};
    float m_wallProbeDistance{PhysicsUnits::toUnits(0.15f)};
    float m_minGroundNormalDot{0.2f};
    float m_groundOffset{PhysicsUnits::toUnits(0.02f)};
    float m_wallOffset{PhysicsUnits::toUnits(0.02f)};
    uint32_t m_groundLayerMask{0xFFFFFFFFu};
    uint32_t m_wallLayerMask{0xFFFFFFFFu};
    bool m_includeTriggers{false};

    std::function<void(Entity&)> m_onLanded{};
    std::function<void(Entity&)> m_onLeftGround{};
};

#endif // GL2D_GROUNDSENSORCOMPONENT_HPP
