#include "ECS/Systems/CharacterMotorSystem.hpp"

#include "ECS/Components/CharacterMotor.hpp"
#include "ECS/Components/Climbable2D.hpp"
#include "ECS/Registry.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <glm/geometric.hpp>

namespace ECS {
namespace {
float moveTowards(float current, float target, float maxDelta) {
    if (std::abs(target - current) <= maxDelta) {
        return target;
    }
    return current + std::copysign(maxDelta, target - current);
}

void validate(const CharacterMotorConfig& config) {
    const std::array values{
        config.maxSpeed, config.groundAcceleration, config.groundDeceleration,
        config.turnAcceleration, config.airControlMultiplier, config.jumpSpeed,
        config.gravity, config.fallGravityMultiplier, config.maxFallSpeed,
        config.coyoteTime, config.jumpBufferTime, config.jumpCutMultiplier,
        config.axisDeadzone, config.climbSpeed, config.climbAcceleration,
        config.climbSnapSpeed
    };
    const bool allFinite = std::ranges::all_of(values, [](float value) {
        return std::isfinite(value);
    });
    if (!allFinite || config.maxSpeed < 0.0f || config.groundAcceleration < 0.0f ||
        config.groundDeceleration < 0.0f || config.turnAcceleration < 0.0f ||
        config.airControlMultiplier < 0.0f || config.jumpSpeed < 0.0f ||
        config.gravity < 0.0f || config.fallGravityMultiplier < 0.0f ||
        config.maxFallSpeed < 0.0f || config.coyoteTime < 0.0f ||
        config.jumpBufferTime < 0.0f || config.jumpCutMultiplier < 0.0f ||
        config.jumpCutMultiplier > 1.0f || config.axisDeadzone < 0.0f ||
        config.axisDeadzone >= 1.0f || config.climbSpeed < 0.0f ||
        config.climbAcceleration < 0.0f || config.climbSnapSpeed < 0.0f) {
        throw std::invalid_argument("CharacterMotorConfig contains invalid values");
    }
}
}

void CharacterMotorSystem::update(Registry& registry, float fixedDeltaTime,
                                  float speedMultiplier,
                                  float accelerationMultiplier) {
    if (!std::isfinite(fixedDeltaTime) || fixedDeltaTime <= 0.0f ||
        !std::isfinite(speedMultiplier) || speedMultiplier < 0.0f ||
        !std::isfinite(accelerationMultiplier) || accelerationMultiplier < 0.0f) {
        throw std::invalid_argument(
            "CharacterMotorSystem delta and feeling multipliers must be positive/finite and non-negative respectively");
    }

    registry.each<CharacterIntent, CharacterMotorConfig, CharacterMotorState,
                  KinematicBody2D, GroundContact2D>(
        [&registry, fixedDeltaTime, speedMultiplier,
         accelerationMultiplier](Entity entity, CharacterIntent& intent,
                         const CharacterMotorConfig& config,
                         CharacterMotorState& state, KinematicBody2D& body,
                         GroundContact2D& contact) {
            validate(config);
            if (!std::isfinite(intent.moveAxis) ||
                !std::isfinite(intent.climbAxis)) {
                throw std::invalid_argument(
                    "Character intent axes must be finite");
            }
            state.jumpedThisStep = false;
            state.landedThisStep = contact.grounded && !state.wasGrounded;
            auto* climbing = registry.tryGet<ClimbingState2D>(entity);

            if (intent.jumpPressed) {
                state.jumpBufferRemaining = config.jumpBufferTime;
            } else {
                state.jumpBufferRemaining = std::max(
                    0.0f, state.jumpBufferRemaining - fixedDeltaTime);
            }

            if (contact.grounded) {
                state.coyoteRemaining = config.coyoteTime;
            } else {
                state.coyoteRemaining = std::max(
                    0.0f, state.coyoteRemaining - fixedDeltaTime);
            }

            float axis = std::clamp(intent.moveAxis, -1.0f, 1.0f);
            if (std::abs(axis) < config.axisDeadzone) {
                axis = 0.0f;
            }
            if (axis != 0.0f) {
                state.facingDirection = axis > 0.0f ? 1.0f : -1.0f;
            }

            if (climbing && climbing->active) {
                float climbAxis = std::clamp(intent.climbAxis, -1.0f, 1.0f);
                if (std::abs(climbAxis) < config.axisDeadzone) {
                    climbAxis = 0.0f;
                }
                const glm::vec2 climbDirection = climbing->axis;
                const glm::vec2 perpendicular{-climbDirection.y,
                                               climbDirection.x};
                const float currentClimbVelocity =
                    glm::dot(body.velocity, climbDirection);
                const float climbVelocity = moveTowards(
                    currentClimbVelocity,
                    climbAxis * config.climbSpeed * speedMultiplier,
                    config.climbAcceleration * accelerationMultiplier *
                        fixedDeltaTime);
                const float snapVelocity = std::clamp(
                    climbing->lateralError / fixedDeltaTime,
                    -config.climbSnapSpeed * speedMultiplier,
                    config.climbSnapSpeed * speedMultiplier);
                body.velocity = climbDirection * climbVelocity +
                                perpendicular * snapVelocity;
                contact = {};
                state.landedThisStep = false;
                state.coyoteRemaining = 0.0f;
                state.jumpBufferRemaining = 0.0f;
                state.locomotion = LocomotionState::Climbing;
                state.wasGrounded = false;
                intent.jumpPressed = false;
                intent.jumpReleased = false;
                return;
            }

            const float targetVelocity =
                axis * config.maxSpeed * speedMultiplier;
            float acceleration = axis == 0.0f
                ? config.groundDeceleration
                : config.groundAcceleration;
            if (axis != 0.0f && body.velocity.x != 0.0f &&
                std::signbit(axis) != std::signbit(body.velocity.x)) {
                acceleration = config.turnAcceleration;
            }
            if (!contact.grounded) {
                acceleration *= config.airControlMultiplier;
            }
            body.velocity.x = moveTowards(
                body.velocity.x, targetVelocity,
                acceleration * accelerationMultiplier * fixedDeltaTime);

            if (climbing && climbing->jumpedOffThisStep) {
                body.velocity.y = config.jumpSpeed * speedMultiplier;
                contact.grounded = false;
                state.jumpBufferRemaining = 0.0f;
                state.coyoteRemaining = 0.0f;
                state.jumpedThisStep = true;
            } else if (state.jumpBufferRemaining > 0.0f && state.coyoteRemaining > 0.0f) {
                body.velocity.y = config.jumpSpeed * speedMultiplier;
                contact.grounded = false;
                state.jumpBufferRemaining = 0.0f;
                state.coyoteRemaining = 0.0f;
                state.jumpedThisStep = true;
            } else if (intent.jumpReleased && body.velocity.y > 0.0f) {
                body.velocity.y *= config.jumpCutMultiplier;
            }

            if (contact.grounded) {
                if (body.velocity.y < 0.0f) {
                    body.velocity.y = 0.0f;
                }
            } else {
                const float gravityMultiplier = body.velocity.y < 0.0f
                    ? config.fallGravityMultiplier : 1.0f;
                body.velocity.y -= config.gravity * gravityMultiplier * fixedDeltaTime;
                body.velocity.y = std::max(
                    body.velocity.y, -config.maxFallSpeed * speedMultiplier);
            }

            if (!contact.grounded && body.velocity.y > 0.0f) {
                state.locomotion = LocomotionState::Rising;
            } else if (!contact.grounded) {
                state.locomotion = LocomotionState::Falling;
            } else if (std::abs(body.velocity.x) > 0.01f) {
                state.locomotion = LocomotionState::Running;
            } else {
                state.locomotion = LocomotionState::Idle;
            }

            state.wasGrounded = contact.grounded;
            intent.jumpPressed = false;
            intent.jumpReleased = false;
        });
}

} // namespace ECS
