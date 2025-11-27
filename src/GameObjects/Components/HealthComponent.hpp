#ifndef GL2D_HEALTH_COMPONENT_HPP
#define GL2D_HEALTH_COMPONENT_HPP

#include <functional>

#include "GameObjects/IComponent.hpp"

class Entity;

class HealthComponent : public IUpdatableComponent {
public:
    using Callback = std::function<void(Entity&)>;

    explicit HealthComponent(float maxHp = 100.0f, float armor = 0.0f)
        : m_maxHp(maxHp), m_hp(maxHp), m_armor(armor) {}

    float hp() const { return m_hp; }
    float maxHp() const { return m_maxHp; }
    float armor() const { return m_armor; }
    bool isDead() const { return m_hp <= 0.0f; }

    void setOnDeath(Callback cb) { m_onDeath = std::move(cb); }
    void setOnDamage(Callback cb) { m_onDamage = std::move(cb); }
    void setOnHeal(Callback cb) { m_onHeal = std::move(cb); }

    void setArmor(float armor) { m_armor = armor; }
    void setMaxHp(float maxHp) { m_maxHp = maxHp; m_hp = std::min(m_hp, m_maxHp); }

    void applyDamage(Entity& owner, float amount);
    void heal(Entity& owner, float amount);

    void update(Entity& owner, double /*dt*/) override {
        if (m_hp <= 0.0f && !m_deathHandled) {
            m_deathHandled = true;
            if (m_onDeath) m_onDeath(owner);
        }
    }

private:
    float m_maxHp{100.0f};
    float m_hp{100.0f};
    float m_armor{0.0f};
    bool m_deathHandled{false};
    Callback m_onDeath{};
    Callback m_onDamage{};
    Callback m_onHeal{};
};

#endif // GL2D_HEALTH_COMPONENT_HPP
