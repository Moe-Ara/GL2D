#include "CombatComponent.hpp"

#include "GameObjects/Entity.hpp"
#include "GameObjects/Components/TransformComponent.hpp"

void CombatComponent::update(Entity& owner, double dt) {
    auto* tc = owner.getComponent<TransformComponent>();
    if (!tc) return;
    const glm::vec2 selfPos = tc->getTransform().Position;
    const glm::vec2 targetPos = currentTargetPos();

    if (m_brain.tryAttack(selfPos, targetPos, static_cast<float>(dt))) {
        if (m_onAttack) {
            m_onAttack(owner);
        }
    }
}

glm::vec2 CombatComponent::currentTargetPos() const {
    if (m_useEntityTarget && m_targetEntity) {
        if (auto* tc = m_targetEntity->getComponent<TransformComponent>()) {
            return tc->getTransform().Position;
        }
    }
    return m_targetPos;
}
