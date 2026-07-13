#pragma once

namespace ECS {
class Registry;

class CharacterMotorSystem {
public:
    static void update(Registry& registry, float fixedDeltaTime,
                       float speedMultiplier = 1.0f,
                       float accelerationMultiplier = 1.0f);
};

} // namespace ECS
