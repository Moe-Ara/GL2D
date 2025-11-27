#include "HealthComponent.hpp"

#include "GameObjects/Entity.hpp"

void HealthComponent::applyDamage(Entity& owner, float amount) {
    if (isDead()) return;
    const float effective = std::max(0.0f, amount - m_armor);
    m_hp = std::max(0.0f, m_hp - effective);
    m_deathHandled = false;
    if (m_onDamage) {
        m_onDamage(owner);
    }
    if (m_hp <= 0.0f && m_onDeath) {
        m_onDeath(owner);
        m_deathHandled = true;
    }
}

void HealthComponent::heal(Entity& owner, float amount) {
    if (isDead()) return;
    const float before = m_hp;
    m_hp = std::min(m_maxHp, m_hp + amount);
    if (m_hp > before && m_onHeal) {
        m_onHeal(owner);
    }
}
