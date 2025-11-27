#include "AIPlayerController.hpp"

#include "AISystem/NavMesh/INavMesh.hpp"
#include "GameObjects/Entity.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "Physics/PhysicsCasts.hpp"

AIPlayerController::AIPlayerController(std::shared_ptr<INavMesh> navMesh)
    : m_navMesh(std::move(navMesh)) {}

void AIPlayerController::setTargetPosition(const glm::vec2& targetPos) {
    m_targetPosition = targetPos;
    m_repathAccum = 0.0f;
}

AIPlayerController::Intent AIPlayerController::gatherIntent(Entity& entity, double dt) {
    Intent intent{};

    auto* transformComp = entity.getComponent<TransformComponent>();
    if (!transformComp || !m_navMesh) {
        return intent;
    }

    const glm::vec2 pos = transformComp->getTransform().Position;
    m_repathAccum += static_cast<float>(dt);

    if (m_path.empty() || m_repathAccum >= m_repathInterval) {
        ensurePath(entity);
        m_repathAccum = 0.0f;
    }

    if (m_path.empty()) {
        return intent;
    }

    advanceWaypoint(pos);
    if (m_pathIndex >= m_path.size()) {
        return intent;
    }

    const glm::vec2 wp = m_path[m_pathIndex];
    glm::vec2 toWp = wp - pos;
    const float len = glm::length(toWp);
    if (hasArrived(pos)) {
        m_path.clear();
        m_pathIndex = 0;
        return intent;
    }
    if (len > 1e-4f) {
        glm::vec2 dir = toWp / len;
        applyAvoidance(entity, pos, dir, intent);
        if (intent.moveAxis == 0.0f) {
            intent.moveAxis = (dir.x > 0.0f) ? 1.0f : -1.0f;
        }
    }

    return intent;
}

glm::vec2 AIPlayerController::resolveTarget() const {
    if (m_followEntity) {
        if (auto* tc = m_followEntity->getComponent<TransformComponent>()) {
            return tc->getTransform().Position;
        }
    }
    return m_targetPosition;
}

void AIPlayerController::ensurePath(Entity& entity) {
    m_path.clear();
    m_pathIndex = 0;
    if (!m_navMesh) return;

    auto* transformComp = entity.getComponent<TransformComponent>();
    if (!transformComp) return;

    const glm::vec2 startPos = transformComp->getTransform().Position;
    const glm::vec2 target = resolveTarget();

    NavPath nav = m_navMesh->findPath(startPos, target);
    if (!nav.valid()) {
        return;
    }
    m_path = nav.points;
    m_pathIndex = 0;
}

void AIPlayerController::advanceWaypoint(const glm::vec2& pos) {
    while (m_pathIndex < m_path.size()) {
        if (glm::length(m_path[m_pathIndex] - pos) <= m_arrivalTolerance) {
            ++m_pathIndex;
        } else {
            break;
        }
    }
}

glm::vec2 AIPlayerController::currentWaypoint() const {
    if (m_pathIndex < m_path.size()) {
        return m_path[m_pathIndex];
    }
    return resolveTarget();
}

bool AIPlayerController::hasArrived(const glm::vec2& pos) const {
    return glm::length(pos - resolveTarget()) <= m_arrivalTolerance;
}

void AIPlayerController::applyAvoidance(Entity& self, const glm::vec2& pos, glm::vec2& desiredDir, Intent& intent) {
    if (!m_worldEntities || m_avoidDistance <= 0.0f) {
        intent.moveAxis = (desiredDir.x > 0.0f) ? 1.0f : -1.0f;
        return;
    }
    // If forward ray hits something, slow/stop.
    if (forwardBlocked(self, pos, desiredDir)) {
        intent.moveAxis = 0.0f;
        return;
    }

    // Separation steering from nearby agents/obstacles.
    glm::vec2 sep = separationForce(self, pos);
    if (glm::length(sep) > 1e-4f) {
        desiredDir = glm::normalize(desiredDir + sep * m_separationWeight);
    }

    intent.moveAxis = (desiredDir.x > 0.0f) ? 1.0f : -1.0f;
}

bool AIPlayerController::forwardBlocked(Entity& self, const glm::vec2& pos, const glm::vec2& dir) const {
    if (!m_worldEntities) return false;
    auto hit = PhysicsCasts::rayCast(pos, dir, m_avoidDistance, *m_worldEntities,
                                     PhysicsCasts::CastFilter{.ignore = &self, .includeTriggers = false});
    return hit.hit;
}

glm::vec2 AIPlayerController::separationForce(Entity& self, const glm::vec2& pos) const {
    if (!m_worldEntities || m_avoidRadius <= 0.0f) return glm::vec2{0.0f};
    auto hits = PhysicsCasts::overlapCircle(pos, m_avoidRadius, *m_worldEntities,
                                            PhysicsCasts::CastFilter{.ignore = &self, .includeTriggers = false});
    glm::vec2 force{0.0f};
    for (const auto& h : hits) {
        if (!h.entity) continue;
        auto* tc = h.entity->getComponent<TransformComponent>();
        if (!tc) continue;
        glm::vec2 otherPos = tc->getTransform().Position;
        glm::vec2 away = pos - otherPos;
        float dist = glm::length(away);
        if (dist < 1e-4f) continue;
        float strength = (m_avoidRadius - dist) / m_avoidRadius;
        force += (away / dist) * strength;
    }
    return force;
}
