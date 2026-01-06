#include "CharacterController.hpp"

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>

#include "GameObjects/Entity.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Components/SpriteComponent.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "GameObjects/Components/GroundSensorComponent.hpp"
#include "GameObjects/Components/LedgeComponent.hpp"
#include "GameObjects/Components/LedgeSensorComponent.hpp"
#include "GameObjects/Sprite.hpp"
#include "Physics/RigidBody.hpp"

#include "Physics/PhysicsUnits.hpp"

namespace {
constexpr float kAxisEpsilon = 0.001f;
const float kHangDetectionVelocity = PhysicsUnits::toUnits(0.2f);
}

void CharacterController::update(Entity &entity, double deltaTime) {
    auto *transformComp = entity.getComponent<TransformComponent>();
    if (!transformComp) {
        return;
    }

    auto *spriteComp = entity.getComponent<SpriteComponent>();
    auto *sprite = spriteComp ? spriteComp->sprite() : nullptr;
    auto &transform = transformComp->getTransform();
    auto *rbComp = entity.getComponent<RigidBodyComponent>();
    RigidBody* body = rbComp ? rbComp->body() : nullptr;

    Intent intent = gatherIntent(entity, deltaTime);
    ensureLedgeSensor(entity);
    if (m_isHanging) {
        if (handleHangInput(intent, transformComp, body, sprite)) {
            return;
        }
    }

    if (!m_groundInitialized) {
        // Assume ground at world y=0 for simple platformer movement; adjust as needed per level.
        m_groundLevel = 0.0f;
        m_groundInitialized = true;
    }

    const float dtf = static_cast<float>(deltaTime);
    const float desiredDir = std::clamp(intent.moveAxis, -1.0f, 1.0f);
    const float axisMag = std::abs(desiredDir);
    if (axisMag > kAxisEpsilon) {
        m_faceDirection = (desiredDir >= 0.0f) ? 1 : -1;
    }

    // Hysteresis: small gap between walk and run thresholds to avoid rapid flipping.
    MoveMode currentMode = m_lastMoveMode;
    if (axisMag <= kAxisEpsilon) {
        currentMode = MoveMode::Idle;
    } else {
        const float walkEnd = m_walkAxisThreshold;
        const float runStart = m_runAxisThreshold;
        switch (m_lastMoveMode) {
            case MoveMode::Idle:
                currentMode = (axisMag <= walkEnd) ? MoveMode::Walk : MoveMode::Run;
                break;
            case MoveMode::Walk:
                currentMode = (axisMag >= runStart) ? MoveMode::Run : MoveMode::Walk;
                break;
            case MoveMode::Run:
                currentMode = (axisMag <= walkEnd) ? MoveMode::Walk : MoveMode::Run;
                break;
        }
    }

    float speedCap = m_moveSpeed;
    if (currentMode == MoveMode::Walk) {
        speedCap *= m_walkSpeedMultiplier;
    }
    const float scaledAxis = std::clamp(axisMag, 0.0f, 1.0f);
    const float targetVelX = scaledAxis > kAxisEpsilon ? desiredDir * (speedCap * scaledAxis) : 0.0f;


    m_lastMoveMode = currentMode;

    // Smoothly approach target velocity using acceleration as the rate.
    const float accelStep = m_acceleration * dtf;
    if (std::abs(targetVelX - m_velocity.x) <= accelStep) {
        m_velocity.x = targetVelX;
    } else {
        m_velocity.x += (targetVelX > m_velocity.x ? 1.0f : -1.0f) * accelStep;
    }

    auto* sensor = entity.getComponent<GroundSensorComponent>();
    if (sensor) {
        if (m_worldEntities) {
            sensor->setWorldEntities(m_worldEntities);
        }
        if (!m_sensorCallbacksBound) {
            sensor->setCallbacks([this](Entity& e) { onLanded(e); },
                                 [this](Entity& e) { onLeftGround(e); });
            m_sensorCallbacksBound = true;
        }
        sensor->refresh(entity); // Avoid ordering issues if controller runs before sensor component.
        m_wallContact = sensor->hasWallContact();
        m_wallNormal = sensor->wallNormal();
    } else {
        m_wallContact = false;
        m_wallNormal = glm::vec2{0.0f};
    }
    const bool groundedBySensor = sensor && sensor->isGrounded();
    const bool platformContact = sensor && sensor->hasPlatformContact();
    glm::vec2 platformVelocity{0.0f};
    if (platformContact) {
        platformVelocity = sensor->platformVelocity();
    }
    const glm::vec2 platformDelta = platformVelocity * dtf;
    const bool wantsClimb = std::abs(intent.climbAxis) >= kAxisEpsilon;
    m_isClimbing = wantsClimb && m_wallContact;
    if (m_isClimbing) {
        m_velocity.x = 0.0f;
    }
    const bool hasHorizontalInput = axisMag > kAxisEpsilon;
    if (!m_isHanging && !m_isClimbing && !m_isGrounded && m_ledgeSensor && hasHorizontalInput) {
        const glm::vec2 currentVel = body ? body->getVelocity() : m_velocity;
        if (currentVel.y < -kHangDetectionVelocity) {
            const glm::vec2 originPos = body ? body->getPosition() : transform.Position;
            const glm::vec2 probeOrigin = originPos + glm::vec2{
                    m_faceDirection * m_movementConfig.ledgeHorizontalOffset,
                    m_movementConfig.ledgeVerticalOffset};
            const auto hit = m_ledgeSensor->detect(probeOrigin,
                                                   m_movementConfig.ledgeProbeDistance,
                                                   entity);
            if (hit.hit) {
                float ledgeOffset = 0.0f;
                if (hit.entity) {
                    if (auto *ledge = hit.entity->getComponent<LedgeComponent>()) {
                        ledgeOffset = ledge->hangOffset();
                    }
                    m_hangEntity = hit.entity;
                }
                startHangState(hit.point, transformComp, body, sprite, ledgeOffset);
                return;
            }
        }
    }
    const bool shouldSnapStop =
        !hasHorizontalInput &&
        ((groundedBySensor) || (!sensor && m_isGrounded));
    if (shouldSnapStop && !m_isClimbing) {
        // Grounded with no input should immediately zero horizontal velocity.
        m_velocity.x = 0.0f;
    }
    const float climbAccelStep = m_climbAcceleration * dtf;
    auto adjustClimbVelocity = [&](float current) {
        if (!m_isClimbing) {
            return current;
        }
        const float target = intent.climbAxis * m_climbSpeed;
        if (std::abs(target - current) <= climbAccelStep) {
            return target;
        }
        return current + (target > current ? 1.0f : -1.0f) * climbAccelStep;
    };

    if (body) {
        auto vel = body->getVelocity();
        vel.x = m_velocity.x;
        if (platformContact && glm::length(platformDelta) > kAxisEpsilon) {
            body->setPosition(body->getPosition() + platformDelta);
        }

        const float groundedVelEps = PhysicsUnits::toUnits(0.015f);
        if (m_isClimbing) {
            m_isGrounded = false;
            vel.y = adjustClimbVelocity(vel.y);
        } else {
            m_isGrounded = groundedBySensor || (std::abs(vel.y) <= groundedVelEps);
            if (intent.jumpPressed && m_isGrounded) {
                vel.y = m_jumpImpulse;
                m_isGrounded = false;
            }
            if (!m_isGrounded) {
                vel.y -= m_gravity * dtf;
            }
        }

        body->setVelocity(vel);
    } else {
        const bool groundedByVel = std::abs(m_velocity.y) <= PhysicsUnits::toUnits(0.015f);
        if (m_isClimbing) {
            m_isGrounded = false;
            m_velocity.y = adjustClimbVelocity(m_velocity.y);
        } else {
            m_isGrounded = groundedBySensor || groundedByVel;
            if (intent.jumpPressed && m_isGrounded) {
                m_velocity.y = m_jumpImpulse;
                m_isGrounded = false;
            }

            if (!m_isGrounded) {
                m_velocity.y -= m_gravity * dtf;
            }
        }

        glm::vec2 position = transform.Position;
        position += m_velocity * dtf;
        if (platformContact) {
            position += platformDelta;
        }

        if (position.y <= m_groundLevel) {
            position.y = m_groundLevel;
            m_velocity.y = 0.0f;
            m_isGrounded = true;
        }

        transform.setPos(position);
        if (sprite) {
            sprite->setPosition(position);
        }
    }
}

void CharacterController::applyFeeling(const FeelingsSystem::FeelingSnapshot &snapshot) {
    resetFeelingOverrides();
    const float speedMul = snapshot.entitySpeedMul.value_or(1.0f);
    const float animMul = snapshot.animationSpeedMul.value_or(1.0f);

    m_moveSpeed = m_baseMoveSpeed * speedMul;
    m_acceleration = m_baseAcceleration * speedMul;
    m_deceleration = m_baseDeceleration * speedMul;
    m_jumpImpulse = m_baseJumpImpulse * speedMul;
    m_gravity = m_baseGravity * animMul;
}

void CharacterController::resetFeelingOverrides() {
    m_moveSpeed = m_baseMoveSpeed;
    m_acceleration = m_baseAcceleration;
    m_deceleration = m_baseDeceleration;
    m_jumpImpulse = m_baseJumpImpulse;
    m_gravity = m_baseGravity;
}

void CharacterController::configureMovement(const MovementConfig &config) {
    m_baseMoveSpeed = config.moveSpeed;
    m_baseAcceleration = config.acceleration;
    m_baseDeceleration = config.deceleration;
    m_baseJumpImpulse = config.jumpImpulse;
    m_baseGravity = config.gravity;
    m_walkSpeedMultiplier = config.walkSpeedMultiplier;
    m_walkAxisThreshold = config.walkAxisThreshold;
    m_runAxisThreshold = config.runAxisThreshold;
    m_climbSpeed = config.climbSpeed;
    m_climbAcceleration = config.climbAcceleration;
    m_movementConfig = config;
    if (m_ledgeSensor) {
        m_ledgeSensor->setProbeDistance(config.ledgeProbeDistance);
    }
    resetFeelingOverrides();
}

void CharacterController::setMaxMoveSpeed(float speed) {
    const float clamped = std::max(0.0f, speed);
    m_baseMoveSpeed = clamped;
    m_moveSpeed = clamped;
}

void CharacterController::resetVelocity() {
    m_velocity = glm::vec2{0.0f};
}

void CharacterController::setVelocity(const glm::vec2& velocity) {
    m_velocity = velocity;
}

void CharacterController::ensureLedgeSensor(Entity& owner) {
    if (!m_ledgeSensor) {
        if (auto *existing = owner.getComponent<LedgeSensorComponent>()) {
            m_ledgeSensor = existing;
        } else {
            m_ledgeSensor = &owner.addComponent<LedgeSensorComponent>();
        }
    }
    if (!m_ledgeSensor) {
        return;
    }
    if (m_worldEntities) {
        m_ledgeSensor->setWorldEntities(m_worldEntities);
    }
    m_ledgeSensor->setProbeDistance(m_movementConfig.ledgeProbeDistance);
}

void CharacterController::startHangState(const glm::vec2& point,
                                        TransformComponent* transform,
                                        RigidBody* body,
                                        GameObjects::Sprite* sprite,
                                        float ledgeHangOffset) {
    m_isHanging = true;
    m_hangPoint = point;
    m_hangEntityOffset = ledgeHangOffset;
    if (body) {
        body->setVelocity(glm::vec2{0.0f});
    }
    m_velocity = glm::vec2{0.0f};
    m_isGrounded = false;
    const glm::vec2 hangPos = hangTransformPosition();
    if (transform) {
        transform->setPosition(hangPos);
    }
    if (body) {
        body->setPosition(hangPos);
    }
    if (sprite) {
        sprite->setPosition(hangPos);
    }
}

bool CharacterController::handleHangInput(const Intent& intent,
                                         TransformComponent* transform,
                                         RigidBody* body,
                                         GameObjects::Sprite* sprite) {
    const glm::vec2 hangPos = hangTransformPosition();
    if (transform) {
        transform->setPosition(hangPos);
    }
    if (body) {
        body->setPosition(hangPos);
        body->setVelocity(glm::vec2{0.0f});
    }
    if (sprite) {
        sprite->setPosition(hangPos);
    }

    if (intent.jumpPressed) {
        releaseHang(true, false, transform, body, sprite);
        return false;
    }

    if (intent.climbAxis > 0.5f) {
        releaseHang(false, true, transform, body, sprite);
        return false;
    }

    if (intent.climbAxis <= -0.5f || intent.moveAxis * m_faceDirection < -0.5f) {
        releaseHang(false, false, transform, body, sprite);
        return false;
    }
    return true;
}

void CharacterController::releaseHang(bool jump,
                                      bool climb,
                                      TransformComponent* transform,
                                      RigidBody* body,
                                      GameObjects::Sprite* sprite) {
    if (!m_isHanging) {
        return;
    }
    m_isHanging = false;
    m_hangEntity = nullptr;
    const glm::vec2 hangPos = hangTransformPosition();
    if (climb) {
        const glm::vec2 climbPos = climbTransformPosition();
        if (transform) {
            transform->setPosition(climbPos);
        }
        if (body) {
            body->setPosition(climbPos);
            body->setVelocity(glm::vec2{0.0f});
        }
        if (sprite) {
            sprite->setPosition(climbPos);
        }
        m_velocity = glm::vec2{0.0f};
        m_isGrounded = true;
    } else {
        if (transform) {
            transform->setPosition(hangPos);
        }
        if (body) {
            body->setPosition(hangPos);
            body->setVelocity(glm::vec2{0.0f, jump ? m_jumpImpulse : 0.0f});
        }
        if (sprite) {
            sprite->setPosition(hangPos);
        }
        m_velocity.x = 0.0f;
        m_velocity.y = jump ? m_jumpImpulse : 0.0f;
        m_isGrounded = false;
    }
    m_hangEntityOffset = 0.0f;
}

glm::vec2 CharacterController::hangTransformPosition() const {
    return m_hangPoint +
           glm::vec2{-m_faceDirection * m_movementConfig.ledgeHorizontalOffset,
                     -m_movementConfig.hangVerticalOffset - m_hangEntityOffset};
}

glm::vec2 CharacterController::climbTransformPosition() const {
    return m_hangPoint +
           glm::vec2{-m_faceDirection * m_movementConfig.ledgeHorizontalOffset,
                     m_movementConfig.hangClimbOffset};
}

void CharacterController::setWorldEntities(std::vector<std::unique_ptr<Entity>> *world) {
    m_worldEntities = world;
    if (m_ledgeSensor) {
        m_ledgeSensor->setWorldEntities(world);
    }
}
