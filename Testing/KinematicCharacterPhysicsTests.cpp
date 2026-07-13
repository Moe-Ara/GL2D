#include <boost/test/unit_test.hpp>

#include "EcsTestOutput.hpp"

#include "ECS/Components/CharacterMotor.hpp"
#include "ECS/Components/Collision2D.hpp"
#include "ECS/Components/Transform2D.hpp"
#include "ECS/Registry.hpp"
#include "ECS/Systems/KinematicCharacterPhysicsSystem.hpp"

namespace {
ECS::Entity addStatic(ECS::Registry& registry, glm::vec2 position,
                      glm::vec2 halfExtents, std::uint32_t category = 1u) {
    const ECS::Entity entity = registry.create();
    registry.emplace<ECS::Transform2D>(entity).position = position;
    auto& collider = registry.emplace<ECS::AabbCollider2D>(entity);
    collider.halfExtents = halfExtents;
    collider.categoryBits = category;
    registry.emplace<ECS::StaticCollider2D>(entity);
    return entity;
}

ECS::Entity addCharacter(ECS::Registry& registry, glm::vec2 position,
                         glm::vec2 halfExtents = {0.5f, 1.0f}) {
    const ECS::Entity entity = registry.create();
    registry.emplace<ECS::Transform2D>(entity).position = position;
    registry.emplace<ECS::AabbCollider2D>(entity).halfExtents = halfExtents;
    registry.emplace<ECS::KinematicBody2D>(entity);
    registry.emplace<ECS::GroundContact2D>(entity);
    registry.emplace<ECS::CharacterCollisionState2D>(entity);
    return entity;
}
}

BOOST_AUTO_TEST_SUITE(KinematicCharacterPhysicsTests)

BOOST_AUTO_TEST_CASE(fast_fall_does_not_tunnel_through_ground) {
    ECS::Registry registry;
    const ECS::Entity ground = addStatic(registry, {0.0f, 0.0f}, {10.0f, 0.5f});
    const ECS::Entity character = addCharacter(registry, {0.0f, 5.0f});
    registry.get<ECS::KinematicBody2D>(character).velocity.y = -100.0f;

    ECS::KinematicCharacterPhysicsSystem::update(registry, 0.1f);

    const auto& transform = registry.get<ECS::Transform2D>(character);
    const auto& contact = registry.get<ECS::GroundContact2D>(character);
    BOOST_TEST(transform.position.y == 1.51f, boost::test_tools::tolerance(1e-5f));
    BOOST_TEST(contact.grounded);
    BOOST_TEST(contact.surface == ground);
    BOOST_TEST(registry.get<ECS::KinematicBody2D>(character).velocity.y == 0.0f);
}

BOOST_AUTO_TEST_CASE(horizontal_sweep_stops_at_wall) {
    ECS::Registry registry;
    addStatic(registry, {3.0f, 1.0f}, {0.5f, 3.0f});
    const ECS::Entity character = addCharacter(registry, {0.0f, 1.0f});
    registry.get<ECS::KinematicBody2D>(character).velocity.x = 50.0f;

    ECS::KinematicCharacterPhysicsSystem::update(registry, 0.1f);

    BOOST_TEST(registry.get<ECS::Transform2D>(character).position.x == 1.99f,
               boost::test_tools::tolerance(1e-5f));
    BOOST_TEST(registry.get<ECS::CharacterCollisionState2D>(character).hitWallRight);
    BOOST_TEST(registry.get<ECS::KinematicBody2D>(character).velocity.x == 0.0f);
}

BOOST_AUTO_TEST_CASE(layer_masks_filter_collision_pairs) {
    ECS::Registry registry;
    addStatic(registry, {3.0f, 1.0f}, {0.5f, 3.0f}, 2u);
    const ECS::Entity character = addCharacter(registry, {0.0f, 1.0f});
    auto& collider = registry.get<ECS::AabbCollider2D>(character);
    collider.maskBits = 1u;
    registry.get<ECS::KinematicBody2D>(character).velocity.x = 50.0f;

    ECS::KinematicCharacterPhysicsSystem::update(registry, 0.1f);
    BOOST_TEST(registry.get<ECS::Transform2D>(character).position.x == 5.0f);
}

BOOST_AUTO_TEST_CASE(character_inherits_moving_surface_velocity) {
    ECS::Registry registry;
    const ECS::Entity platform = addStatic(registry, {0.0f, 0.0f}, {4.0f, 0.5f});
    registry.emplace<ECS::SurfaceVelocity2D>(platform).velocity = {2.0f, 0.0f};
    const ECS::Entity character = addCharacter(registry, {0.0f, 1.51f});
    auto& contact = registry.get<ECS::GroundContact2D>(character);
    contact.grounded = true;
    contact.platformVelocity = {2.0f, 0.0f};
    contact.surface = platform;

    ECS::KinematicCharacterPhysicsSystem::update(registry, 0.5f);

    BOOST_TEST(registry.get<ECS::Transform2D>(character).position.x == 1.0f,
               boost::test_tools::tolerance(1e-5f));
    BOOST_TEST(registry.get<ECS::Transform2D>(platform).position.x == 1.0f,
               boost::test_tools::tolerance(1e-5f));
    BOOST_TEST(registry.get<ECS::GroundContact2D>(character).grounded);
}

BOOST_AUTO_TEST_CASE(spawn_overlap_is_depenetrated) {
    ECS::Registry registry;
    addStatic(registry, {0.0f, 0.0f}, {10.0f, 0.5f});
    const ECS::Entity character = addCharacter(registry, {0.0f, 0.25f});

    ECS::KinematicCharacterPhysicsSystem::update(registry, 0.01f);

    BOOST_TEST(registry.get<ECS::Transform2D>(character).position.y > 1.5f);
    BOOST_TEST(registry.get<ECS::GroundContact2D>(character).grounded);
}

BOOST_AUTO_TEST_SUITE_END()
