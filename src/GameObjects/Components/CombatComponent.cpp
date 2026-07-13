#include "CombatComponent.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

#include "GameObjects/Entity.hpp"
#include "GameObjects/Components/TransformComponent.hpp"

void CombatComponent::setDamage(float damage) {
    if (!std::isfinite(damage) || damage < 0.0f) {
        throw std::invalid_argument("Combat damage must be finite and non-negative");
    }
    m_damage = damage;
}

void CombatComponent::setTargetPosition(const glm::vec2& pos) {
    if (!std::isfinite(pos.x) || !std::isfinite(pos.y)) {
        throw std::invalid_argument("Combat target position must be finite");
    }
    m_targetPos = pos;
    m_hasTarget = true;
}

void CombatComponent::update(Entity& owner, double dt) {
    if (!std::isfinite(dt) || dt < 0.0 ||
        dt > static_cast<double>(std::numeric_limits<float>::max())) {
        throw std::invalid_argument("Combat update delta must be finite, non-negative, and representable as float");
    }
    m_brain.update(static_cast<float>(dt));
    if (!m_hasTarget) return;

    auto* tc = owner.getComponent<TransformComponent>();
    if (!tc) return;
    const glm::vec2 selfPos = tc->getTransform().Position;

    if (m_brain.tryAttack(selfPos, m_targetPos)) {
        if (m_onAttack) {
            m_onAttack(owner);
        }
    }
}
