#pragma once

namespace ECS {
class Registry;

class KinematicCharacterPhysicsSystem {
public:
    static void update(Registry& registry, float fixedDeltaTime);
};

} // namespace ECS
