#include "AICombatBrain.hpp"

#include <algorithm>
#include <glm/glm.hpp>

bool AICombatBrain::tryAttack(const glm::vec2& selfPos, const glm::vec2& targetPos, float dt) {
    if (m_cooldownTimer > 0.0f) {
        m_cooldownTimer = std::max(0.0f, m_cooldownTimer - dt);
        return false;
    }
    const float dist = glm::length(targetPos - selfPos);
    if (dist > m_attackRange) {
        return false;
    }
    m_cooldownTimer = m_cooldown;
    return true;
}
