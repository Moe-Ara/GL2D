#pragma once

#include "ECS/Entity.hpp"
#include "Physics/PhysicsUnits.hpp"

#include <glm/vec2.hpp>

namespace ECS {

struct CharacterIntent {
    float moveAxis{0.0f};
    float climbAxis{0.0f};
    bool jumpPressed{false};
    bool jumpReleased{false};
    bool jumpHeld{false};
};

struct KinematicBody2D {
    glm::vec2 velocity{0.0f};
};

struct GroundContact2D {
    bool grounded{false};
    glm::vec2 normal{0.0f, 1.0f};
    glm::vec2 platformVelocity{0.0f};
    Entity surface{};
};

struct CharacterMotorConfig {
    float maxSpeed{PhysicsUnits::toUnits(2.0f)};
    float groundAcceleration{PhysicsUnits::toUnits(24.0f)};
    float groundDeceleration{PhysicsUnits::toUnits(30.0f)};
    float turnAcceleration{PhysicsUnits::toUnits(38.0f)};
    float airControlMultiplier{0.65f};
    float jumpSpeed{PhysicsUnits::toUnits(4.25f)};
    float gravity{PhysicsUnits::toUnits(9.81f)};
    float fallGravityMultiplier{1.35f};
    float maxFallSpeed{PhysicsUnits::toUnits(12.0f)};
    float coyoteTime{0.1f};
    float jumpBufferTime{0.12f};
    float jumpCutMultiplier{0.45f};
    float axisDeadzone{0.08f};
    float climbSpeed{PhysicsUnits::toUnits(2.0f)};
    float climbAcceleration{PhysicsUnits::toUnits(24.0f)};
    float climbSnapSpeed{PhysicsUnits::toUnits(4.0f)};
};

enum class LocomotionState {
    Idle,
    Running,
    Rising,
    Falling,
    Climbing
};

struct CharacterMotorState {
    float coyoteRemaining{0.0f};
    float jumpBufferRemaining{0.0f};
    float facingDirection{1.0f};
    LocomotionState locomotion{LocomotionState::Idle};
    bool jumpedThisStep{false};
    bool landedThisStep{false};
    bool wasGrounded{false};
};

} // namespace ECS
