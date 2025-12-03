#ifndef GL2D_WATERSYSTEM_HPP
#define GL2D_WATERSYSTEM_HPP

#include <glm/vec2.hpp>
#include <memory>
#include <vector>

#include "Physics/Collision/AABB.hpp"

class Entity;
class ACollider;
class ColliderComponent;
class RigidBodyComponent;
class WaterVolumeComponent;
class WaterStateComponent;

// Applies buoyancy, drag, and flow forces to dynamic bodies that overlap
// WaterVolumeComponent colliders. Also updates WaterStateComponent so gameplay
// can react to being submerged.
class WaterSystem {
public:
    WaterSystem() = default;
    ~WaterSystem() = default;

    WaterSystem(const WaterSystem&) = delete;
    WaterSystem& operator=(const WaterSystem&) = delete;
    WaterSystem(WaterSystem&&) = delete;
    WaterSystem& operator=(WaterSystem&&) = delete;

    void update(float dt,
                const std::vector<std::unique_ptr<Entity>>& entities,
                const glm::vec2& gravity);

private:
    struct VolumeEntry {
        Entity* entity{nullptr};
        WaterVolumeComponent* volume{nullptr};
        ColliderComponent* colliderComp{nullptr};
        ACollider* collider{nullptr};
        AABB bounds{};
    };

    struct BodyEntry {
        Entity* entity{nullptr};
        RigidBodyComponent* rbComp{nullptr};
        WaterStateComponent* waterState{nullptr};
        ACollider* collider{nullptr};
        AABB bounds{};
    };

    static float computeSubmersion(const AABB& volume, const AABB& body);
    void applyForces(const VolumeEntry& volume,
                     BodyEntry& body,
                     float submersion,
                     float dt,
                     const glm::vec2& gravity) const;
};

#endif // GL2D_WATERSYSTEM_HPP
