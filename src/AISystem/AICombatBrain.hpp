#ifndef AI_COMBAT_BRAIN_HPP
#define AI_COMBAT_BRAIN_HPP

#include <glm/vec2.hpp>

// Deterministic range/cooldown gate for AI or scripted combat. Time progression
// is separate from attack attempts so losing a target does not pause cooldowns.
class AICombatBrain {
public:
    void setAttackRange(float range);
    void setCooldown(float seconds);

    void update(float dt);
    [[nodiscard]] bool tryAttack(const glm::vec2& selfPos, const glm::vec2& targetPos);
    void resetCooldown() noexcept { m_cooldownTimer = 0.0f; }

    [[nodiscard]] bool isOnCooldown() const noexcept { return m_cooldownTimer > 0.0f; }
    [[nodiscard]] float remainingCooldown() const noexcept { return m_cooldownTimer; }
    [[nodiscard]] float attackRange() const noexcept { return m_attackRange; }
    [[nodiscard]] float cooldown() const noexcept { return m_cooldown; }

private:
    float m_attackRange{50.0f};
    float m_cooldown{1.0f};
    float m_cooldownTimer{0.0f};
};

#endif // AI_COMBAT_BRAIN_HPP
