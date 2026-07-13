#include <boost/test/unit_test.hpp>

#include "EcsTestOutput.hpp"

#include "ECS/Components/CharacterMotor.hpp"
#include "ECS/Registry.hpp"
#include "ECS/Systems/CharacterMotorSystem.hpp"

namespace {
struct CharacterFixture {
    ECS::Registry registry;
    ECS::Entity entity{registry.create()};

    CharacterFixture() {
        registry.emplace<ECS::CharacterIntent>(entity);
        registry.emplace<ECS::CharacterMotorConfig>(entity);
        registry.emplace<ECS::CharacterMotorState>(entity);
        registry.emplace<ECS::KinematicBody2D>(entity);
        registry.emplace<ECS::GroundContact2D>(entity);
    }
};
}

BOOST_FIXTURE_TEST_SUITE(CharacterMotorTests, CharacterFixture)

BOOST_AUTO_TEST_CASE(accelerates_and_turns_more_quickly) {
    auto& config = registry.get<ECS::CharacterMotorConfig>(entity);
    config.maxSpeed = 10.0f;
    config.groundAcceleration = 20.0f;
    config.turnAcceleration = 40.0f;
    config.gravity = 0.0f;
    registry.get<ECS::GroundContact2D>(entity).grounded = true;
    registry.get<ECS::CharacterIntent>(entity).moveAxis = 1.0f;

    ECS::CharacterMotorSystem::update(registry, 0.1f);
    BOOST_TEST(registry.get<ECS::KinematicBody2D>(entity).velocity.x == 2.0f);

    registry.get<ECS::CharacterIntent>(entity).moveAxis = -1.0f;
    ECS::CharacterMotorSystem::update(registry, 0.1f);
    BOOST_TEST(registry.get<ECS::KinematicBody2D>(entity).velocity.x == -2.0f);
}

BOOST_AUTO_TEST_CASE(coyote_time_accepts_a_late_jump) {
    auto& config = registry.get<ECS::CharacterMotorConfig>(entity);
    config.gravity = 0.0f;
    config.jumpSpeed = 10.0f;
    config.coyoteTime = 0.1f;
    auto& contact = registry.get<ECS::GroundContact2D>(entity);
    contact.grounded = true;
    ECS::CharacterMotorSystem::update(registry, 0.01f);

    contact.grounded = false;
    ECS::CharacterMotorSystem::update(registry, 0.05f);
    registry.get<ECS::CharacterIntent>(entity).jumpPressed = true;
    ECS::CharacterMotorSystem::update(registry, 0.01f);

    BOOST_TEST(registry.get<ECS::CharacterMotorState>(entity).jumpedThisStep);
    BOOST_TEST(registry.get<ECS::KinematicBody2D>(entity).velocity.y == 10.0f);
}

BOOST_AUTO_TEST_CASE(jump_buffer_fires_on_landing) {
    auto& config = registry.get<ECS::CharacterMotorConfig>(entity);
    config.gravity = 0.0f;
    config.jumpSpeed = 10.0f;
    config.jumpBufferTime = 0.12f;
    auto& contact = registry.get<ECS::GroundContact2D>(entity);
    contact.grounded = false;
    registry.get<ECS::CharacterIntent>(entity).jumpPressed = true;
    ECS::CharacterMotorSystem::update(registry, 0.05f);

    contact.grounded = true;
    ECS::CharacterMotorSystem::update(registry, 0.05f);
    BOOST_TEST(registry.get<ECS::CharacterMotorState>(entity).jumpedThisStep);
    BOOST_TEST(!contact.grounded);
}

BOOST_AUTO_TEST_CASE(releasing_jump_reduces_upward_velocity) {
    auto& config = registry.get<ECS::CharacterMotorConfig>(entity);
    config.gravity = 0.0f;
    config.jumpCutMultiplier = 0.4f;
    auto& body = registry.get<ECS::KinematicBody2D>(entity);
    body.velocity.y = 10.0f;
    registry.get<ECS::CharacterIntent>(entity).jumpReleased = true;

    ECS::CharacterMotorSystem::update(registry, 0.01f);
    BOOST_TEST(body.velocity.y == 4.0f, boost::test_tools::tolerance(1e-5f));
}

BOOST_AUTO_TEST_CASE(feeling_speed_and_acceleration_multipliers_are_independent) {
    auto& config = registry.get<ECS::CharacterMotorConfig>(entity);
    config.maxSpeed = 10.0f;
    config.groundAcceleration = 20.0f;
    config.gravity = 0.0f;
    registry.get<ECS::GroundContact2D>(entity).grounded = true;
    registry.get<ECS::CharacterIntent>(entity).moveAxis = 1.0f;

    ECS::CharacterMotorSystem::update(registry, 0.1f, 0.5f, 2.0f);

    // Target speed is halved while acceleration is doubled.
    BOOST_TEST(registry.get<ECS::KinematicBody2D>(entity).velocity.x == 4.0f,
               boost::test_tools::tolerance(1e-5f));
}

BOOST_AUTO_TEST_SUITE_END()
