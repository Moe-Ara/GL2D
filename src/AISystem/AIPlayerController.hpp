#ifndef AI_PLAYER_CONTROLLER_HPP
#define AI_PLAYER_CONTROLLER_HPP

#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "Engine/CharacterController.hpp"

class INavMesh;
class Entity;

class AIPlayerController : public CharacterController {
public:
    explicit AIPlayerController(std::shared_ptr<INavMesh> navMesh);

    void setTargetPosition(const glm::vec2& targetPos);
    void setArrivalTolerance(float toleranceUnits) { m_arrivalTolerance = toleranceUnits; }
    void setRepathInterval(float seconds) { m_repathInterval = seconds; }
    void setFollowEntity(Entity* entity) { m_followEntity = entity; }
    Entity* getFollowEntity() const { return m_followEntity; }
    void setWorldEntities(std::vector<std::unique_ptr<Entity>>* world) { CharacterController::setWorldEntities(world); }
    void setAvoidance(float avoidDistance, float avoidRadius) { m_avoidDistance = avoidDistance; m_avoidRadius = avoidRadius; }

protected:
    Intent gatherIntent(Entity& entity, double dt) override;

private:
    glm::vec2 resolveTarget() const;
    void ensurePath(Entity& entity);
    void advanceWaypoint(const glm::vec2& pos);
    glm::vec2 currentWaypoint() const;
    bool hasArrived(const glm::vec2& pos) const;
    void applyAvoidance(Entity& self, const glm::vec2& pos, glm::vec2& desiredDir, Intent& intent);
    bool forwardBlocked(Entity& self, const glm::vec2& pos, const glm::vec2& dir) const;
    glm::vec2 separationForce(Entity& self, const glm::vec2& pos) const;

    std::shared_ptr<INavMesh> m_navMesh;
    glm::vec2 m_targetPosition{0.0f};
    Entity* m_followEntity{nullptr};
    std::vector<glm::vec2> m_path;
    size_t m_pathIndex{0};
    float m_arrivalTolerance{5.0f};
    float m_repathInterval{0.5f};
    float m_repathAccum{0.0f};
    float m_avoidDistance{40.0f};
    float m_avoidRadius{20.0f};
    float m_separationWeight{0.5f};
};

#endif // AI_PLAYER_CONTROLLER_HPP
