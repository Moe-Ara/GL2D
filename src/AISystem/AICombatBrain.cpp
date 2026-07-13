#include "AICombatBrain.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <glm/geometric.hpp>

namespace {
bool finite(const glm::vec2& value) {
    return std::isfinite(value.x) && std::isfinite(value.y);
}
}

void AICombatBrain::setAttackRange(float range) {
    if (!std::isfinite(range) || range < 0.0f) {
        throw std::invalid_argument("AI attack range must be finite and non-negative");
    }
    m_attackRange = range;
}

void AICombatBrain::setCooldown(float seconds) {
    if (!std::isfinite(seconds) || seconds < 0.0f) {
        throw std::invalid_argument("AI attack cooldown must be finite and non-negative");
    }
    m_cooldown = seconds;
    m_cooldownTimer = std::min(m_cooldownTimer, m_cooldown);
}

void AICombatBrain::update(float dt) {
    if (!std::isfinite(dt) || dt < 0.0f) {
        throw std::invalid_argument("AI combat delta must be finite and non-negative");
    }
    m_cooldownTimer = std::max(0.0f, m_cooldownTimer - dt);
}

bool AICombatBrain::tryAttack(const glm::vec2& selfPos, const glm::vec2& targetPos) {
    if (!finite(selfPos) || !finite(targetPos)) {
        throw std::invalid_argument("AI combat positions must be finite");
    }
    if (isOnCooldown()) return false;

    const glm::vec2 delta = targetPos - selfPos;
    if (glm::dot(delta, delta) > m_attackRange * m_attackRange) {
        return false;
    }
    m_cooldownTimer = m_cooldown;
    return true;
}
