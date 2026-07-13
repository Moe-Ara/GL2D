#include <boost/test/unit_test.hpp>

#include "ECS/Components/CharacterMotor.hpp"
#include "ECS/Components/Collision2D.hpp"
#include "ECS/Components/Transform2D.hpp"
#include "ECS/Registry.hpp"
#include "ECS/Systems/CharacterMotorSystem.hpp"
#include "ECS/Systems/KinematicCharacterPhysicsSystem.hpp"

#include "EcsTestOutput.hpp"

#include <algorithm>

BOOST_AUTO_TEST_SUITE(ECSPlatformerIntegrationTests)

BOOST_AUTO_TEST_CASE(character_runs_jumps_and_lands_through_full_ecs_pipeline) {
    ECS::Registry registry;
    const ECS::Entity ground = registry.create();
    registry.emplace<ECS::Transform2D>(ground).position = {0.0f, 0.0f};
    registry.emplace<ECS::AabbCollider2D>(ground).halfExtents = {100.0f, 0.5f};
    registry.emplace<ECS::StaticCollider2D>(ground);

    const ECS::Entity player = registry.create();
    registry.emplace<ECS::Transform2D>(player).position = {0.0f, 1.51f};
    registry.emplace<ECS::AabbCollider2D>(player).halfExtents = {0.5f, 1.0f};
    auto& intent = registry.emplace<ECS::CharacterIntent>(player);
    auto& config = registry.emplace<ECS::CharacterMotorConfig>(player);
    registry.emplace<ECS::CharacterMotorState>(player);
    registry.emplace<ECS::KinematicBody2D>(player);
    registry.emplace<ECS::GroundContact2D>(player).grounded = true;
    registry.emplace<ECS::CharacterCollisionState2D>(player);

    config.maxSpeed = 8.0f;
    config.groundAcceleration = 30.0f;
    config.groundDeceleration = 30.0f;
    config.turnAcceleration = 40.0f;
    config.jumpSpeed = 10.0f;
    config.gravity = 30.0f;
    config.fallGravityMultiplier = 1.4f;
    config.maxFallSpeed = 20.0f;
    constexpr float dt = 1.0f / 120.0f;

    intent.moveAxis = 1.0f;
    float maximumHeight = registry.get<ECS::Transform2D>(player).position.y;
    bool landedAfterJump = false;
    for (int step = 0; step < 360; ++step) {
        if (step == 30) {
            intent.jumpPressed = true;
            intent.jumpHeld = true;
        }
        if (step == 55) {
            intent.jumpReleased = true;
            intent.jumpHeld = false;
        }
        ECS::CharacterMotorSystem::update(registry, dt);
        ECS::KinematicCharacterPhysicsSystem::update(registry, dt);
        maximumHeight = std::max(
            maximumHeight, registry.get<ECS::Transform2D>(player).position.y);
        if (step > 55 && registry.get<ECS::GroundContact2D>(player).grounded) {
            landedAfterJump = true;
        }
    }

    BOOST_TEST(registry.get<ECS::Transform2D>(player).position.x > 15.0f);
    BOOST_TEST(maximumHeight > 2.5f);
    BOOST_TEST(landedAfterJump);
    BOOST_TEST(registry.get<ECS::GroundContact2D>(player).surface == ground);
    BOOST_TEST(registry.get<ECS::CharacterMotorState>(player).facingDirection == 1.0f);
}

BOOST_AUTO_TEST_SUITE_END()
