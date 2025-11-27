#ifndef AI_COMBAT_BRAIN_HPP
#define AI_COMBAT_BRAIN_HPP

#include <glm/vec2.hpp>

// Minimal combat brain: tracks cooldown and range for a single attack.
class AICombatBrain {
public:
    void setAttackRange(float range) { m_attackRange = range; }
    void setCooldown(float seconds) { m_cooldown = seconds; }

    // Returns true if an attack can be triggered this frame; updates internal cooldown.
    bool tryAttack(const glm::vec2& selfPos, const glm::vec2& targetPos, float dt);
    bool isOnCooldown() const { return m_cooldownTimer > 0.0f; }

private:
    float m_attackRange{50.0f};
    float m_cooldown{1.0f};
    float m_cooldownTimer{0.0f};
};

#endif // AI_COMBAT_BRAIN_HPP
