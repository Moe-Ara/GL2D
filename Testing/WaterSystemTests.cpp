#include <boost/test/unit_test.hpp>

#include <glm/vec2.hpp>
#include <memory>
#include <vector>

#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Components/WaterStateComponent.hpp"
#include "GameObjects/Components/WaterVolumeComponent.hpp"
#include "GameObjects/Entity.hpp"
#include "Physics/Collision/AABBCollider.hpp"
#include "Physics/WaterSystem.hpp"
#include "Physics/PhysicsUnits.hpp"

namespace {
using EntityPtr = std::unique_ptr<Entity>;

// Helper to create a water volume entity with a given world-space AABB.
EntityPtr makeWaterVolume(const glm::vec2& min, const glm::vec2& max, const glm::vec2& flow = {0.0f, 0.0f}) {
    auto e = std::make_unique<Entity>();
    auto& t = e->addComponent<TransformComponent>();
    t.setPosition(glm::vec2{0.0f});
    auto collider = std::make_unique<AABBCollider>(min, max);
    auto& c = e->addComponent<ColliderComponent>(std::move(collider), ColliderType::AABB, 0.0f);
    c.setTrigger(true);
    auto& volume = e->addComponent<WaterVolumeComponent>();
    volume.setFlowVelocity(flow);
    return e;
}

// Helper to create a dynamic body with an AABB of given size placed at position.
EntityPtr makeBody(const glm::vec2& position, const glm::vec2& halfExtents, float mass = 1.0f, const glm::vec2& velocity = {0.0f, 0.0f}) {
    auto e = std::make_unique<Entity>();
    auto& t = e->addComponent<TransformComponent>();
    t.setPosition(position);
    const glm::vec2 min = -halfExtents;
    const glm::vec2 max = halfExtents;
    auto collider = std::make_unique<AABBCollider>(min, max);
    auto& c = e->addComponent<ColliderComponent>(std::move(collider), ColliderType::AABB, 0.0f);
    c.setTrigger(false);

    auto body = std::make_unique<RigidBody>(mass, RigidBodyType::DYNAMIC);
    body->setVelocity(velocity);
    body->setTransform(&t.getTransform());
    e->addComponent<RigidBodyComponent>(std::move(body));
    e->addComponent<WaterStateComponent>();
    return e;
}
} // namespace

BOOST_AUTO_TEST_SUITE(WaterSystemTests)

BOOST_AUTO_TEST_CASE(applies_buoyancy_upwards_when_submerged) {
    std::vector<std::unique_ptr<Entity>> entities;
    entities.push_back(makeWaterVolume(glm::vec2{-5.0f, 0.0f}, glm::vec2{5.0f, 10.0f}));
    // Body height = 2; placed fully inside the volume.
    const glm::vec2 halfSize{1.0f, 1.0f};
    entities.push_back(makeBody(glm::vec2{0.0f, 2.0f}, halfSize));

    WaterSystem system;
    const glm::vec2 gravity = PhysicsUnits::toUnits(glm::vec2{0.0f, -9.81f});
    const float dt = 0.1f;
    system.update(dt, entities, gravity);

    auto* bodyComp = entities[1]->getComponent<RigidBodyComponent>();
    auto* body = bodyComp ? bodyComp->body() : nullptr;
    auto* waterState = entities[1]->getComponent<WaterStateComponent>();
    BOOST_REQUIRE(body != nullptr);
    BOOST_REQUIRE(waterState != nullptr);

    body->integrate(dt);

    BOOST_TEST(waterState->isSubmerged());
    BOOST_TEST(waterState->submersion() == 1.0f, boost::test_tools::tolerance(1e-3f));
    BOOST_TEST(body->getVelocity().y > 0.0f); // should accelerate upward
}

BOOST_AUTO_TEST_CASE(applies_drag_and_flow_follow) {
    std::vector<std::unique_ptr<Entity>> entities;
    const glm::vec2 flow = glm::vec2{PhysicsUnits::toUnits(2.0f), 0.0f};
    entities.push_back(makeWaterVolume(glm::vec2{-10.0f, -5.0f}, glm::vec2{10.0f, 5.0f}, flow));

    // Body starts moving faster than the flow; expect drag+flow forces to reduce speed toward flow.
    const glm::vec2 initialVel{PhysicsUnits::toUnits(4.0f), 0.0f};
    entities.push_back(makeBody(glm::vec2{0.0f, 0.0f}, glm::vec2{1.0f, 1.0f}, 1.0f, initialVel));

    WaterSystem system;
    const glm::vec2 gravity = PhysicsUnits::toUnits(glm::vec2{0.0f, -9.81f});
    const float dt = 0.1f;
    system.update(dt, entities, gravity);

    auto* bodyComp = entities[1]->getComponent<RigidBodyComponent>();
    auto* body = bodyComp ? bodyComp->body() : nullptr;
    auto* waterState = entities[1]->getComponent<WaterStateComponent>();
    BOOST_REQUIRE(body != nullptr);
    BOOST_REQUIRE(waterState != nullptr);

    body->integrate(dt);

    const float vx = body->getVelocity().x;
    BOOST_TEST(waterState->isSubmerged());
    BOOST_TEST(vx < initialVel.x);
    BOOST_TEST(vx > 0.0f);
}

BOOST_AUTO_TEST_SUITE_END()
