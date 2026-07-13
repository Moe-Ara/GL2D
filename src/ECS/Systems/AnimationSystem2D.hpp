#pragma once

namespace ECS {
class Registry;

class CharacterAnimationParameterSystem2D {
public:
    static void update(Registry& registry);
};

class AnimationSystem2D {
public:
    // Clears per-render-frame events before one or more fixed simulation steps.
    static void beginFrame(Registry& registry);
    static void update(Registry& registry, float fixedDeltaTime,
                       float feelingSpeedMultiplier = 1.0f);
};

} // namespace ECS
