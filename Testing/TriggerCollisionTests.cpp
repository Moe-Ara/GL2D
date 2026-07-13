#include <boost/test/unit_test.hpp>

#include "Physics/Collision/CircleCollider.hpp"
#include "Physics/Collision/AABBCollider.hpp"
#include "Physics/Collision/CollisionDispatcher.hpp"
#include "Physics/TriggerSystem.hpp"
#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Entity.hpp"
#include "Utils/Transform.hpp"

#include <memory>
#include <vector>

BOOST_AUTO_TEST_SUITE(TriggerCollisionTests)

BOOST_AUTO_TEST_CASE(trigger_overlap_returns_collision_with_zero_penetration) {
    CircleCollider a(1.0f);
    CircleCollider b(1.0f);
    a.setTrigger(true);
    b.setTrigger(true);
    Transform ta{};
    Transform tb{};
    ta.setPos({0.0f, 0.0f});
    tb.setPos({0.5f, 0.0f});
    a.setTransform(&ta);
    b.setTransform(&tb);

    auto hit = CollisionDispatcher::dispatch(a, b);
    BOOST_REQUIRE(hit);
    BOOST_TEST(hit->collided);
    BOOST_TEST(hit->penetration == 0.0f, boost::test_tools::tolerance(1e-5f));
}

BOOST_AUTO_TEST_CASE(collision_queries_do_not_consume_one_shot_triggers) {
    CircleCollider trigger(1.0f);
    CircleCollider other(1.0f);
    trigger.setTrigger(true, true);
    Transform ta{};
    Transform tb{};
    ta.setPos({0.0f, 0.0f});
    tb.setPos({0.25f, 0.0f});
    trigger.setTransform(&ta);
    other.setTransform(&tb);

    auto first = CollisionDispatcher::dispatch(trigger, other);
    BOOST_REQUIRE(first);
    BOOST_TEST(first->collided);

    auto second = CollisionDispatcher::dispatch(trigger, other);
    BOOST_REQUIRE(second);
    BOOST_TEST(second->collided);
    BOOST_TEST(!trigger.hasTriggered());
}

BOOST_AUTO_TEST_CASE(trigger_system_owns_one_shot_lifetime) {
    std::vector<std::unique_ptr<Entity>> entities;
    entities.push_back(std::make_unique<Entity>());
    entities.push_back(std::make_unique<Entity>());
    Entity& triggerEntity = *entities[0];
    Entity& otherEntity = *entities[1];
    triggerEntity.addComponent<TransformComponent>();
    auto& triggerComponent = triggerEntity.addComponent<ColliderComponent>(
        std::make_unique<CircleCollider>(1.0f));
    triggerComponent.setTrigger(true, true);
    auto& otherTransform = otherEntity.addComponent<TransformComponent>();
    otherTransform.setPosition({0.25f, 0.0f});
    otherEntity.addComponent<ColliderComponent>(
        std::make_unique<CircleCollider>(1.0f));
    int enterCount = 0;
    triggerComponent.setOnTriggerEnter(
        [&](Entity&, Entity&) { ++enterCount; });

    TriggerSystem triggers;
    triggers.update(entities);
    triggers.update(entities);
    BOOST_TEST(enterCount == 1);
    BOOST_REQUIRE(triggerComponent.collider());
    BOOST_TEST(triggerComponent.collider()->hasTriggered());

    otherTransform.setPosition({10.0f, 0.0f});
    triggers.update(entities);
    otherTransform.setPosition({0.25f, 0.0f});
    triggers.update(entities);
    BOOST_TEST(enterCount == 1);
}

BOOST_AUTO_TEST_CASE(rotated_trigger_exits_inside_empty_broadphase_corner) {
    std::vector<std::unique_ptr<Entity>> entities;
    entities.push_back(std::make_unique<Entity>());
    entities.push_back(std::make_unique<Entity>());
    Entity& triggerEntity = *entities[0];
    Entity& otherEntity = *entities[1];

    auto& triggerTransform = triggerEntity.addComponent<TransformComponent>();
    triggerTransform.setRotation(45.0f);
    auto& triggerComponent = triggerEntity.addComponent<ColliderComponent>(
        std::make_unique<AABBCollider>(
            glm::vec2{-2.0f, -0.1f}, glm::vec2{2.0f, 0.1f}));
    triggerComponent.setTrigger(true);
    auto& otherTransform = otherEntity.addComponent<TransformComponent>();
    otherEntity.addComponent<ColliderComponent>(
        std::make_unique<CircleCollider>(0.1f));

    int enterCount = 0;
    int exitCount = 0;
    triggerComponent.setOnTriggerEnter(
        [&](Entity&, Entity&) { ++enterCount; });
    triggerComponent.setOnTriggerExit(
        [&](Entity&, Entity&) { ++exitCount; });

    TriggerSystem triggers;
    triggers.update(entities);
    BOOST_TEST(enterCount == 1);
    otherTransform.setPosition({1.3f, -1.3f});
    triggers.update(entities);
    BOOST_TEST(exitCount == 1);
}

BOOST_AUTO_TEST_SUITE_END()
