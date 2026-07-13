#pragma once

namespace ECS {
class Registry;

class ClimbingSystem2D final {
public:
    static void update(Registry& registry);
};
} // namespace ECS
