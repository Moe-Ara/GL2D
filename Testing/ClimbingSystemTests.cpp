#include <boost/test/unit_test.hpp>

#include "ECS/Components/CharacterMotor.hpp"
#include "ECS/Components/Climbable2D.hpp"
#include "ECS/Components/Collision2D.hpp"
#include "ECS/Components/Transform2D.hpp"
#include "ECS/Registry.hpp"
#include "ECS/Systems/CharacterMotorSystem.hpp"
#include "ECS/Systems/ClimbingSystem2D.hpp"

#include "EcsTestOutput.hpp"

#include <stdexcept>

namespace {
ECS::Entity addLadder(ECS::Registry& registry, glm::vec2 center,
                      glm::vec2 halfExtents = {1.0f, 5.0f}) {
    const ECS::Entity ladder = registry.create();
    registry.emplace<ECS::Transform2D>(ladder).position = center;
    registry.emplace<ECS::AabbCollider2D>(ladder).halfExtents = halfExtents;
    registry.emplace<ECS::Climbable2D>(ladder);
    return ladder;
}

ECS::Entity addCharacter(ECS::Registry& registry, glm::vec2 center) {
    const ECS::Entity character = registry.create();
    registry.emplace<ECS::Transform2D>(character).position = center;
    registry.emplace<ECS::AabbCollider2D>(character).halfExtents = {0.5f, 1.0f};
    registry.emplace<ECS::CharacterIntent>(character);
    registry.emplace<ECS::CharacterMotorConfig>(character);
    registry.emplace<ECS::CharacterMotorState>(character);
    registry.emplace<ECS::KinematicBody2D>(character);
    registry.emplace<ECS::GroundContact2D>(character);
    registry.emplace<ECS::ClimbingState2D>(character);
    return character;
}
}

BOOST_AUTO_TEST_SUITE(ClimbingSystemTests)

BOOST_AUTO_TEST_CASE(vertical_input_enters_ladder_and_suppresses_gravity) {
    ECS::Registry registry;
    const ECS::Entity ladder = addLadder(registry, {0.0f, 4.0f});
    const ECS::Entity character = addCharacter(registry, {0.5f, 2.0f});
    auto& intent = registry.get<ECS::CharacterIntent>(character);
    intent.climbAxis = 1.0f;

    ECS::ClimbingSystem2D::update(registry);
    ECS::CharacterMotorSystem::update(registry, 0.1f);

    const auto& climbing = registry.get<ECS::ClimbingState2D>(character);
    const auto& velocity = registry.get<ECS::KinematicBody2D>(character).velocity;
    BOOST_TEST(climbing.active);
    BOOST_TEST(climbing.climbable == ladder);
    BOOST_TEST(climbing.startedThisStep);
    BOOST_TEST(velocity.y > 0.0f);
    BOOST_TEST(velocity.x < 0.0f);
    BOOST_TEST(registry.get<ECS::CharacterMotorState>(character).locomotion ==
               ECS::LocomotionState::Climbing);
}

BOOST_AUTO_TEST_CASE(jump_detaches_from_ladder_with_upward_velocity) {
    ECS::Registry registry;
    addLadder(registry, {0.0f, 4.0f});
    const ECS::Entity character = addCharacter(registry, {0.0f, 2.0f});
    auto& intent = registry.get<ECS::CharacterIntent>(character);
    intent.climbAxis = 1.0f;
    ECS::ClimbingSystem2D::update(registry);
    ECS::CharacterMotorSystem::update(registry, 0.01f);

    intent.jumpPressed = true;
    ECS::ClimbingSystem2D::update(registry);
    ECS::CharacterMotorSystem::update(registry, 0.01f);

    const auto& climbing = registry.get<ECS::ClimbingState2D>(character);
    BOOST_TEST(!climbing.active);
    BOOST_TEST(climbing.jumpedOffThisStep);
    BOOST_TEST(registry.get<ECS::KinematicBody2D>(character).velocity.y > 0.0f);

    ECS::ClimbingSystem2D::update(registry);
    BOOST_TEST(!registry.get<ECS::ClimbingState2D>(character).active);
    intent.climbAxis = 0.0f;
    ECS::ClimbingSystem2D::update(registry);
    intent.climbAxis = 1.0f;
    ECS::ClimbingSystem2D::update(registry);
    BOOST_TEST(registry.get<ECS::ClimbingState2D>(character).active);
}

BOOST_AUTO_TEST_CASE(no_vertical_input_does_not_auto_grab_a_ladder) {
    ECS::Registry registry;
    addLadder(registry, {0.0f, 4.0f});
    const ECS::Entity character = addCharacter(registry, {0.0f, 2.0f});

    ECS::ClimbingSystem2D::update(registry);

    BOOST_TEST(!registry.get<ECS::ClimbingState2D>(character).active);
}

BOOST_AUTO_TEST_CASE(invalid_climbable_axis_is_rejected) {
    ECS::Registry registry;
    const ECS::Entity ladder = addLadder(registry, {0.0f, 4.0f});
    registry.get<ECS::Climbable2D>(ladder).axis = {0.0f, 0.0f};
    addCharacter(registry, {0.0f, 2.0f});

    BOOST_CHECK_THROW(ECS::ClimbingSystem2D::update(registry),
                      std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()
