//
// Created by Codex on 27/11/2025.
//

#include "RopePrefab.hpp"

#include "Engine/Scene.hpp"
#include "GameObjects/Components/HingeComponent.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Components/TransformFollowerComponent.hpp"
#include "GameObjects/Components/RopeSegmentComponent.hpp"
#include "GameObjects/Prefabs/PrefabFactory.hpp"

#include "Physics/RigidBody.hpp"

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace Prefabs {

namespace {
glm::vec2 safeDirection(glm::vec2 dir) {
    const float len = glm::length(dir);
    if (len < 1e-4f) {
        return glm::vec2{0.0f, -1.0f};
    }
    return dir / len;
}
} // namespace

RopePrefabResult RopePrefab::instantiate(Scene &scene, const RopePrefabConfig &config) {
    RopePrefabResult result{};
    if (config.segmentCount <= 0) {
        return result;
    }

    Entity* anchorEntity = nullptr;
    if (config.startAnchor && config.useAnchorEntity) {
        auto anchorPtr = std::make_unique<Entity>();
        auto &anchorTransform = anchorPtr->addComponent<TransformComponent>();
        anchorTransform.setPosition(config.anchorPosition);
        auto &followerComp = anchorPtr->addComponent<TransformFollowerComponent>();
        followerComp.setTarget(config.startAnchor);
        followerComp.setOffset(config.startAnchorOffset);
        followerComp.setCopyRotation(true);
        auto anchorBody = std::make_unique<RigidBody>(0.0f, RigidBodyType::STATIC);
        anchorBody->setTransform(&anchorTransform.getTransform());
        anchorPtr->addComponent<RigidBodyComponent>(std::move(anchorBody));
        anchorEntity = &scene.addEntity(std::move(anchorPtr));
    }

    std::unordered_map<std::string, ComponentSpec> prefabOverrides;
    ComponentSpec rigidOverride{};
    rigidOverride.type = "RigidBody";
    rigidOverride.numbers["mass"] = config.segmentMass;
    rigidOverride.numbers["linearDamping"] = config.segmentLinearDamping;
    rigidOverride.numbers["angularDamping"] = config.segmentAngularDamping;
    prefabOverrides["RigidBody"] = rigidOverride;

    const glm::vec2 direction = safeDirection(config.direction);
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
        Entity &segmentRef = scene.addEntity(std::move(segment));
        result.segments.push_back(&segmentRef);
        position += direction * stepLength;
    }

    if (result.segments.empty()) {
        return result;
    }

    const glm::vec2 halfOffset{config.segmentLength * 0.5f, 0.0f};
    const glm::vec2 startOffset{-halfOffset.x, 0.0f};

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
        hinge->setReferenceAngle(0.0f);
        hinge->setAnchorSelf(startOffset);
        applyLimits(hinge);

        if (i == 0) {
            if (anchorEntity) {
                hinge->setAnchorTarget(glm::vec2{0.0f, 0.0f});
                hinge->setTarget(anchorEntity);
            } else if (config.startAnchor) {
                hinge->setAnchorTarget(config.startAnchorOffset);
                hinge->setTarget(config.startAnchor);
            }
        } else {
            hinge->setAnchorTarget(halfOffset);
            hinge->setTarget(result.segments[i - 1]);
        }
    }

    if (config.endAnchor) {
        Entity *tail = result.segments.back();
        auto &endHingeRef = tail->addComponent<HingeComponent>();
        HingeComponent *endHinge = &endHingeRef;
        endHinge->setEnabled(true);
        endHinge->setReferenceAngle(0.0f);
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
        const glm::vec2 ropeDirection = safeDirection(config.direction);
        for (Entity* segment : result.segments) {
            segment->addComponent<RopeSegmentComponent>(ropeDirection, ropeTop, ropeBottom);
        }
    }

    return result;
}

} // namespace Prefabs
