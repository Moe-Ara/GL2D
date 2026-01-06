#ifndef GL2D_GAMECONFIG_HPP
#define GL2D_GAMECONFIG_HPP

#include "Engine/CharacterController.hpp"

namespace Config {

struct CombatConfig {
    float attackRange{80.0f};
    float attackCooldown{0.5f};
    float attackDamage{10.0f};
};

struct HealthConfig {
    float maxHp{120.0f};
    float armor{5.0f};
    float regenRate{0.0f};
};

struct PlayerConfig {
    CharacterController::MovementConfig movement{};
    CombatConfig combat{};
    HealthConfig health{};
};

class GameConfig {
public:
    static const PlayerConfig &player();
};

} // namespace Config

#endif // GL2D_GAMECONFIG_HPP
