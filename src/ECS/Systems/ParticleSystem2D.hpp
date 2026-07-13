#pragma once

namespace ECS {

class Registry;

class ParticleSystem2D {
public:
    static void update(Registry& registry, float fixedDeltaTime);
};

} // namespace ECS
