#include <boost/test/unit_test.hpp>

#include "EcsTestOutput.hpp"

#include "ECS/Components/Collision2D.hpp"
#include "ECS/Components/SmoothedTransform2D.hpp"
#include "ECS/Components/Transform2D.hpp"
#include "ECS/Registry.hpp"
#include "Engine/Scene.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "Physics/RigidBody.hpp"

#include <memory>

BOOST_AUTO_TEST_SUITE(RenderInterpolationTests)

BOOST_AUTO_TEST_CASE(interpolated_transform_blends_position_scale_rotation) {
    ECS::Transform2D previous{};
    previous.position = {0.0f, 10.0f};
    previous.scale = {1.0f, 1.0f};
    previous.rotationDegrees = 0.0f;
    ECS::Transform2D current{};
    current.position = {10.0f, 20.0f};
    current.scale = {3.0f, 1.0f};
    current.rotationDegrees = 90.0f;

    const ECS::Transform2D mid =
        ECS::interpolatedTransform2D(previous, current, 0.5f);
    BOOST_TEST(mid.position.x == 5.0f, boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(mid.position.y == 15.0f, boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(mid.scale.x == 2.0f, boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(mid.rotationDegrees == 45.0f, boost::test_tools::tolerance(0.0001f));

    // Endpoints are exact.
    BOOST_TEST(ECS::interpolatedTransform2D(previous, current, 0.0f).position.x == 0.0f);
    BOOST_TEST(ECS::interpolatedTransform2D(previous, current, 1.0f).position.x == 10.0f);
}

BOOST_AUTO_TEST_CASE(scene_snapshots_smoothed_ecs_entities_one_step_behind) {
    Scene scene;
    scene.configureFixedStep({1.0 / 128.0, 0.25, 16});

    auto& registry = scene.registry();
    const ECS::Entity mover = registry.create();
    registry.emplace<ECS::Transform2D>(mover).position = {0.0f, 5.0f};
    registry.emplace<ECS::SmoothedTransform2D>(mover);
    registry.emplace<ECS::AabbCollider2D>(mover).halfExtents = {0.5f, 0.5f};
    registry.emplace<ECS::KinematicBody2D>(mover).velocity = {128.0f, 0.0f};
    registry.emplace<ECS::GroundContact2D>(mover);
    registry.emplace<ECS::CharacterCollisionState2D>(mover);

    scene.advance(1.0f / 128.0f); // exactly one fixed step

    const auto* previous = registry.tryGet<ECS::PreviousTransform2D>(mover);
    BOOST_REQUIRE(previous != nullptr);
    const auto& transform = registry.get<ECS::Transform2D>(mover);
    // The snapshot holds the pre-step pose; the transform moved one step.
    BOOST_TEST(previous->value.position.x == 0.0f,
               boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(transform.position.x == 1.0f,
               boost::test_tools::tolerance(0.0001f));

    scene.advance(1.0f / 128.0f); // second step: history follows
    BOOST_TEST(registry.tryGet<ECS::PreviousTransform2D>(mover)->value.position.x
                   == 1.0f,
               boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(registry.get<ECS::Transform2D>(mover).position.x == 2.0f,
               boost::test_tools::tolerance(0.0001f));
}

BOOST_AUTO_TEST_CASE(untagged_ecs_entities_get_no_history_component) {
    Scene scene;
    scene.configureFixedStep({1.0 / 128.0, 0.25, 16});
    auto& registry = scene.registry();
    const ECS::Entity plain = registry.create();
    registry.emplace<ECS::Transform2D>(plain);

    scene.advance(1.0f / 128.0f);

    BOOST_TEST(registry.tryGet<ECS::PreviousTransform2D>(plain) == nullptr);
}

BOOST_AUTO_TEST_CASE(scene_tracks_mobile_legacy_entity_previous_positions) {
    Scene scene;
    scene.configureFixedStep({1.0 / 128.0, 0.25, 16});
    Entity& entity = scene.createEntity();
    auto& transform = entity.addComponent<TransformComponent>();
    transform.setPosition({7.0f, 3.0f});
    // Only mobile entities (controller/follower/non-static body) get history.
    auto body = std::make_unique<RigidBody>(1.0f, RigidBodyType::KINEMATIC);
    body->setTransform(&transform.getTransform());
    entity.addComponent<RigidBodyComponent>(std::move(body));

    Entity& staticEntity = scene.createEntity();
    auto& staticTransform = staticEntity.addComponent<TransformComponent>();
    staticTransform.setPosition({1.0f, 1.0f});

    BOOST_TEST(scene.previousPosition(entity.getId()) == nullptr);
    scene.advance(1.0f / 128.0f);

    // Static-only entities are skipped; they render identically either way.
    BOOST_TEST(scene.previousPosition(staticEntity.getId()) == nullptr);

    const glm::vec2* previous = scene.previousPosition(entity.getId());
    BOOST_REQUIRE(previous != nullptr);
    BOOST_TEST(previous->x == 7.0f, boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(previous->y == 3.0f, boost::test_tools::tolerance(0.0001f));

    // Destruction releases the history entry.
    scene.destroyEntity(entity);
    BOOST_TEST(scene.previousPosition(entity.getId()) == nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
