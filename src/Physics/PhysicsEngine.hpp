//
// PhysicsEngine.hpp
//

#ifndef GL2D_PHYSICSENGINE_HPP
#define GL2D_PHYSICSENGINE_HPP

#include <glm/vec2.hpp>
#include <memory>
#include <vector>

#include "Physics/PhysicsUnits.hpp"

class Entity;
class RigidBody;
class ACollider;
class RigidBodyComponent;
class ColliderComponent;

class PhysicsEngine {
public:
    explicit PhysicsEngine(glm::vec2 gravity = PhysicsUnits::toUnits(glm::vec2{0.0f, -9.81f}));
    ~PhysicsEngine() = default;

    PhysicsEngine(const PhysicsEngine&) = delete;
    PhysicsEngine& operator=(const PhysicsEngine&) = delete;
    PhysicsEngine(PhysicsEngine&&) = delete;
    PhysicsEngine& operator=(PhysicsEngine&&) = delete;

    void setGravity(const glm::vec2& g) { m_gravity = g; }
    glm::vec2 getGravity() const { return m_gravity; }

    void step(float dt, const std::vector<std::unique_ptr<Entity>>& entities);

private:
    struct BodyEntry {
        Entity* entity{nullptr};
        RigidBodyComponent* rbComp{nullptr};
        ColliderComponent* colliderComp{nullptr};
        RigidBody* body{nullptr};
        ACollider* collider{nullptr};
    };

    struct HingeEntry {
        RigidBody* bodyA{nullptr};
        RigidBody* bodyB{nullptr};
        glm::vec2 anchorA{0.0f};
        glm::vec2 anchorB{0.0f};
        float referenceAngle{0.0f};
        bool limitsEnabled{false};
        float lowerLimit{0.0f};
        float upperLimit{0.0f};
        float limitStiffness{10.0f};
        float limitDamping{1.0f};
        float maxLimitTorque{10.0f};
        bool motorEnabled{false};
        float motorSpeed{0.0f};
        float motorStiffness{5.0f};
        float maxMotorTorque{10.0f};
    };

    void gather(const std::vector<std::unique_ptr<Entity>>& entities);
    void integrateBodies(float dt);
    void resolveCollisions();
    void resolveHinges();

    glm::vec2 m_gravity;
    std::vector<BodyEntry> m_entries;
    std::vector<HingeEntry> m_hingeEntries;
};

#endif // GL2D_PHYSICSENGINE_HPP
