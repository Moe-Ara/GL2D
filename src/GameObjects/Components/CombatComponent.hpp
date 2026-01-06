#ifndef GL2D_COMBAT_COMPONENT_HPP
#define GL2D_COMBAT_COMPONENT_HPP

#include <functional>
#include <glm/vec2.hpp>

#include "GameObjects/IComponent.hpp"
#include "AISystem/AICombatBrain.hpp"

class Entity;

// Generic combat component usable by any entity (AI or player driven).
class CombatComponent : public IUpdatableComponent {
public:
    using AttackCallback = std::function<void(Entity& self)>;

    CombatComponent() = default;

    void setRange(float r) { m_brain.setAttackRange(r); }
    void setCooldown(float seconds) { m_brain.setCooldown(seconds); }
    void setOnAttack(AttackCallback cb) { m_onAttack = std::move(cb); }
    void setDamage(float damage) { m_damage = damage; }
    float damage() const { return m_damage; }

    void setTargetPosition(const glm::vec2& pos) { m_targetPos = pos; m_useEntityTarget = false; }
    void setTargetEntity(Entity* e) { m_targetEntity = e; m_useEntityTarget = true; }

    void update(Entity& owner, double dt) override;

private:
    glm::vec2 currentTargetPos() const;

    AICombatBrain m_brain{};
    AttackCallback m_onAttack{};
    glm::vec2 m_targetPos{0.0f};
    Entity* m_targetEntity{nullptr};
    bool m_useEntityTarget{false};
    float m_damage{10.0f};
};

#endif // GL2D_COMBAT_COMPONENT_HPP
