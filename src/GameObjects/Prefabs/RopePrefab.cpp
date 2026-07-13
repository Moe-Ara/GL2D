//
// Created by Codex on 27/11/2025.
//

#include "RopePrefab.hpp"

#include "Engine/Scene.hpp"
#include "GameObjects/Components/HingeComponent.hpp"
#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Components/TransformFollowerComponent.hpp"
#include "GameObjects/Components/RopeSegmentComponent.hpp"
#include "GameObjects/Prefabs/PrefabFactory.hpp"
#include "GameObjects/Prefabs/PrefabCatalouge.hpp"

#include "Physics/RigidBody.hpp"

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <stdexcept>

namespace Prefabs {

namespace {
bool finite(const glm::vec2& value) {
    return std::isfinite(value.x) && std::isfinite(value.y);
}

glm::vec2 normalizedDirection(glm::vec2 dir) {
    const float len = glm::length(dir);
    if (!finite(dir) || len < 1e-4f) {
        throw std::invalid_argument("Rope direction must be finite and non-zero");
    }
    return dir / len;
}

float rotationRadians(const Entity& entity) {
    if (const auto* transform = entity.getComponent<TransformComponent>()) {
        return glm::radians(transform->getTransform().Rotation);
    }
    return 0.0f;
}

void validate(const RopePrefabConfig& config) {
    if (!finite(config.anchorPosition) || !finite(config.startAnchorOffset) ||
        !finite(config.endAnchorOffset) || config.segmentPrefabId.empty() ||
        config.segmentCount <= 0 || !std::isfinite(config.segmentLength) ||
        config.segmentLength <= 0.0f || !std::isfinite(config.segmentThickness) ||
        config.segmentThickness <= 0.0f || !std::isfinite(config.segmentSpacing) ||
        config.segmentLength + config.segmentSpacing <= 0.0f ||
        !std::isfinite(config.segmentMass) || config.segmentMass <= 0.0f ||
        !std::isfinite(config.segmentLinearDamping) ||
        config.segmentLinearDamping < 0.0f ||
        !std::isfinite(config.segmentAngularDamping) ||
        config.segmentAngularDamping < 0.0f || config.collisionLayer > 31u ||
        !std::isfinite(config.lowerLimit) || !std::isfinite(config.upperLimit) ||
        config.lowerLimit > config.upperLimit ||
        !std::isfinite(config.limitStiffness) || config.limitStiffness < 0.0f ||
        !std::isfinite(config.limitDamping) || config.limitDamping < 0.0f ||
        !std::isfinite(config.maxLimitTorque) || config.maxLimitTorque < 0.0f ||
        !std::isfinite(config.maxDrop) || config.maxDrop < 0.0f) {
        throw std::invalid_argument("RopePrefabConfig contains invalid values");
    }
}

bool sceneContains(const Scene& scene, const Entity* entity) {
    if (!entity) return false;
    return std::ranges::any_of(scene.getEntities(), [entity](const auto& candidate) {
        return candidate.get() == entity;
    });
}

void validateAnchor(const Scene& scene, const Entity* entity,
                    bool requiresRigidBody, const char* label) {
    if (!entity) return;
    if (!sceneContains(scene, entity)) {
        throw std::invalid_argument(std::string(label) +
                                    " must belong to the target Scene");
    }
    if (!entity->getComponent<TransformComponent>() ||
        (requiresRigidBody && !entity->getComponent<RigidBodyComponent>())) {
        throw std::invalid_argument(std::string(label) +
            (requiresRigidBody
                ? " requires Transform and RigidBody components"
                : " requires a Transform component"));
    }
}
} // namespace

RopePrefabResult RopePrefab::instantiate(Scene &scene, const RopePrefabConfig &config) {
    validate(config);
    if (!PrefabCatalouge::contains(config.segmentPrefabId)) {
        throw std::invalid_argument(
            "Rope segment prefab is not registered: " + config.segmentPrefabId);
    }
    const Prefab& prefab = PrefabCatalouge::get(config.segmentPrefabId);
    if (!prefab.hasCollider && !prefab.components.contains("Collider")) {
        throw std::invalid_argument(
            "Rope segment prefab requires a Collider component: " +
            config.segmentPrefabId);
    }
    validateAnchor(scene, config.startAnchor, !config.useAnchorEntity,
                   "Rope start anchor");
    validateAnchor(scene, config.endAnchor, true, "Rope end anchor");
    RopePrefabResult result{};
    const glm::vec2 direction = normalizedDirection(config.direction);

    Entity* anchorEntity = nullptr;
    if (config.useAnchorEntity || !config.startAnchor) {
        auto anchorPtr = std::make_unique<Entity>();
        auto &anchorTransform = anchorPtr->addComponent<TransformComponent>();
        anchorTransform.setPosition(config.anchorPosition);
        if (config.startAnchor) {
            auto &followerComp = anchorPtr->addComponent<TransformFollowerComponent>();
            followerComp.setTarget(config.startAnchor);
            followerComp.setOffset(config.startAnchorOffset);
            followerComp.setCopyRotation(true);
        }
        auto anchorBody = std::make_unique<RigidBody>(0.0f, RigidBodyType::STATIC);
        anchorBody->setTransform(&anchorTransform.getTransform());
        anchorPtr->addComponent<RigidBodyComponent>(std::move(anchorBody));
        anchorEntity = &scene.addEntity(std::move(anchorPtr));
    } else {
        anchorEntity = config.startAnchor;
    }
    result.anchor = anchorEntity;

    std::unordered_map<std::string, ComponentSpec> prefabOverrides;
    ComponentSpec rigidOverride{};
    rigidOverride.type = "RigidBody";
    rigidOverride.numbers["mass"] = config.segmentMass;
    rigidOverride.numbers["linearDamping"] = config.segmentLinearDamping;
    rigidOverride.numbers["angularDamping"] = config.segmentAngularDamping;
    rigidOverride.numbers["inertia"] = config.segmentMass *
        (config.segmentLength * config.segmentLength +
         config.segmentThickness * config.segmentThickness) / 12.0f;
    prefabOverrides["RigidBody"] = rigidOverride;

    const float angleRad = std::atan2(direction.y, direction.x);
    const float rotationDeg = glm::degrees(angleRad);
    const float stepLength = config.segmentLength + config.segmentSpacing;
    glm::vec2 position = config.anchorPosition + direction * (config.segmentLength * 0.5f);
    float allowedDrop = std::numeric_limits<float>::infinity();
    if (config.clampDrop && config.maxDrop > 0.0f) {
        allowedDrop = std::max(0.0f, config.maxDrop - config.segmentLength * 0.5f);
    }

    for (int i = 0; i < config.segmentCount; ++i) {
        const float dropSoFar = glm::dot(position - config.anchorPosition, direction);
        if (dropSoFar > allowedDrop) {
            break;
        }
        auto segment = PrefabFactory::instantiate(config.segmentPrefabId,
                                                  position,
                                                  glm::vec2{config.segmentLength, config.segmentThickness},
                                                  rotationDeg,
                                                  prefabOverrides);
        if (!segment) {
            throw std::runtime_error(
                "Rope segment prefab is not registered: " +
                config.segmentPrefabId);
        }
        auto* collider = segment->getComponent<ColliderComponent>();
        if (!collider) {
            throw std::runtime_error(
                "Rope segment prefab requires a Collider component: " +
                config.segmentPrefabId);
        }
        collider->setLayer(config.collisionLayer);
        collider->setCollisionMask(
            config.collisionMask & ~(1u << config.collisionLayer));
        Entity &segmentRef = scene.addEntity(std::move(segment));
        result.segments.push_back(&segmentRef);
        position += direction * stepLength;
    }

    if (result.segments.empty()) {
        return result;
    }

    const glm::vec2 halfOffset{config.segmentLength * 0.5f, 0.0f};
    const glm::vec2 startOffset{-halfOffset.x, 0.0f};
    const glm::vec2 connectedHalfOffset{
        (config.segmentLength + config.segmentSpacing) * 0.5f, 0.0f};

    auto applyLimits = [&](HingeComponent *hinge) {
        hinge->enableLimits(config.limitEnabled);
        hinge->setLimitRange(config.lowerLimit, config.upperLimit);
        hinge->setLimitParameters(config.limitStiffness, config.limitDamping, config.maxLimitTorque);
        hinge->enableMotor(false);
    };

    for (int i = 0; i < static_cast<int>(result.segments.size()); ++i) {
        Entity *segment = result.segments[i];
        HingeComponent *hinge = segment->getComponent<HingeComponent>();
        if (!hinge) {
            hinge = &segment->addComponent<HingeComponent>();
        }
        hinge->setEnabled(true);
        hinge->setAnchorSelf(i == 0 ? startOffset : -connectedHalfOffset);
        applyLimits(hinge);

        if (i == 0) {
            hinge->setAnchorTarget(config.useAnchorEntity || !config.startAnchor
                                       ? glm::vec2{0.0f, 0.0f}
                                       : config.startAnchorOffset);
            hinge->setTarget(anchorEntity);
        } else {
            hinge->setAnchorTarget(connectedHalfOffset);
            hinge->setTarget(result.segments[i - 1]);
        }
        hinge->setReferenceAngle(
            rotationRadians(*hinge->target()) - rotationRadians(*segment));
    }

    if (config.endAnchor) {
        Entity *tail = result.segments.back();
        auto &endHingeRef = tail->addComponent<HingeComponent>();
        HingeComponent *endHinge = &endHingeRef;
        endHinge->setEnabled(true);
        endHinge->setReferenceAngle(
            rotationRadians(*config.endAnchor) - rotationRadians(*tail));
        endHinge->setAnchorSelf(halfOffset);
        endHinge->setAnchorTarget(config.endAnchorOffset);
        endHinge->setTarget(config.endAnchor);
        applyLimits(endHinge);
    }

    if (!result.segments.empty()) {
        glm::vec2 ropeTop = config.anchorPosition;
        glm::vec2 ropeBottom = ropeTop;
        if (auto* bottomTransform = result.segments.back()->getComponent<TransformComponent>()) {
            ropeBottom = bottomTransform->getTransform().Position;
        }
        ropeBottom += direction * (config.segmentLength * 0.5f);
        for (std::size_t i = 0; i < result.segments.size(); ++i) {
            Entity* segment = result.segments[i];
            auto& rope = segment->addComponent<RopeSegmentComponent>(
                direction, ropeTop, ropeBottom);
            rope.setSegmentLength(config.segmentLength);
            rope.setPrevious(i > 0 ? result.segments[i - 1] : nullptr);
            rope.setNext(i + 1 < result.segments.size()
                             ? result.segments[i + 1] : nullptr);
        }
    }

    return result;
}

} // namespace Prefabs
