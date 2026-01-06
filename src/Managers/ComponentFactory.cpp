//
// Created by Mohamad on 21/11/2025.
//

#include "ComponentFactory.hpp"
#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "GameObjects/Components/HingeComponent.hpp"
#include "GameObjects/Sprite.hpp"
#include "Graphics/Animation/AnimationStateMachine.hpp"
#include "Graphics/Animation/Animator.hpp"
#include "SpriteManager.hpp"
#include "AnimatorManager.hpp"
#include "AnimationStateMachineManager.hpp"
#include "TilemapManager.hpp"
#include "Physics/Collision/AABBCollider.hpp"
#include "Physics/Collision/CircleCollider.hpp"
#include "Physics/Collision/CapsuleCollider.hpp"
#include "Physics/RigidBody.hpp"
#include <glm/vec2.hpp>
#include <algorithm>
#include <cctype>
#include <memory>
#include <string>

namespace {
float readNumber(const ComponentSpec &specs, const std::string &key, float fallback) {
    auto it = specs.numbers.find(key);
    return it != specs.numbers.end() ? it->second : fallback;
}

glm::vec2 readVec2(const ComponentSpec &specs,
                   const std::string &xKey,
                   const std::string &yKey,
                   const glm::vec2 &fallback) {
    glm::vec2 result = fallback;
    auto xIt = specs.numbers.find(xKey);
    if (xIt != specs.numbers.end()) {
        result.x = xIt->second;
    }
    auto yIt = specs.numbers.find(yKey);
    if (yIt != specs.numbers.end()) {
        result.y = yIt->second;
    }
    return result;
}

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

ColliderType parseColliderType(const std::string &shapeName) {
    if (shapeName == "circle") {
        return ColliderType::CIRCLE;
    }
    if (shapeName == "capsule") {
        return ColliderType::CAPSULE;
    }
    if (shapeName == "composite") {
        return ColliderType::COMPOSITE;
    }
    if (shapeName == "polygon") {
        return ColliderType::POLYGON;
    }
    return ColliderType::AABB;
}

RigidBodyType parseRigidBodyType(const std::string &typeName) {
    if (typeName == "static") {
        return RigidBodyType::STATIC;
    }
    if (typeName == "kinematic") {
        return RigidBodyType::KINEMATIC;
    }
    return RigidBodyType::DYNAMIC;
}
} // namespace

std::unique_ptr<IComponent> ComponentFactory::create(const ComponentSpec &specs) {
    if (specs.type == "Transform") {
        auto comp = std::make_unique<TransformComponent>();
        return comp;
    }

    if (specs.type == "Sprite") {
        auto it = specs.strings.find("spriteId");
        GameObjects::Sprite *sprite = nullptr;
        if (it != specs.strings.end()) {
            sprite = SpriteManager::get(it->second);
        }
        if (sprite) {
            auto owned = sprite->clone();
            return std::make_unique<SpriteComponent>(owned);
        }
        return std::make_unique<SpriteComponent>(sprite);
    }

    if (specs.type == "Animator") {
        auto it = specs.strings.find("animatorId");
        Graphics::Animator *anim = nullptr;
        if (it != specs.strings.end()) {
            anim = AnimatorManager::get(it->second);
        }
        return std::make_unique<AnimatorComponent>(anim);
    }

    if (specs.type == "AnimStateMachine") {
        auto it = specs.strings.find("stateMachineId");
        Graphics::AnimationStateMachine *sm = nullptr;
        if (it != specs.strings.end()) {
            sm = AnimationStateMachineManager::get(it->second);
        }
        return std::make_unique<AnimationStateMachineComponent>(sm);
    }

    if (specs.type == "Tilemap") {
        std::shared_ptr<TilemapData> data;
        auto it = specs.strings.find("tilemapId");
        if (it != specs.strings.end()) {
            data = TilemapManager::get(it->second);
        }

        // Allow inline definitions via numbers if provided.
        if (!data) {
            auto wIt = specs.numbers.find("width");
            auto hIt = specs.numbers.find("height");
            auto tsXIt = specs.numbers.find("tileSizeX");
            auto tsYIt = specs.numbers.find("tileSizeY");
            if (wIt != specs.numbers.end() && hIt != specs.numbers.end()) {
                data = std::make_shared<TilemapData>();
                data->width = static_cast<int>(wIt->second);
                data->height = static_cast<int>(hIt->second);
                if (tsXIt != specs.numbers.end() && tsYIt != specs.numbers.end()) {
                    data->tileSize = {tsXIt->second, tsYIt->second};
                }
                data->tiles.resize(data->width * data->height, 0);
            }
        }

        return std::make_unique<TilemapComponent>(data);
    }

    if (specs.type == "Trigger") {
        auto comp = std::make_unique<TriggerComponent>();
        auto posX = specs.numbers.find("posX");
        auto posY = specs.numbers.find("posY");
        auto sizeX = specs.numbers.find("sizeX");
        auto sizeY = specs.numbers.find("sizeY");
        if (posX != specs.numbers.end() && posY != specs.numbers.end()) {
            comp->position = {posX->second, posY->second};
        }
        if (sizeX != specs.numbers.end() && sizeY != specs.numbers.end()) {
            comp->size = {sizeX->second, sizeY->second};
        }
        auto evIt = specs.strings.find("eventId");
        if (evIt != specs.strings.end()) {
            comp->eventId = evIt->second;
        }
        return comp;
    }

    if (specs.type == "Collider") {
        const auto shapeIt = specs.strings.find("shape");
        const std::string shapeName = shapeIt != specs.strings.end() ? toLower(shapeIt->second) : std::string{};
        const ColliderType shape = parseColliderType(shapeName);
        std::unique_ptr<ACollider> collider;
        if (shape == ColliderType::CIRCLE) {
            const float radius = std::max(0.0f, readNumber(specs, "radius", 0.5f));
            auto circle = std::make_unique<CircleCollider>(radius);
            circle->setLocalOffset(readVec2(specs, "offsetX", "offsetY", glm::vec2{0.0f, 0.0f}));
            collider = std::move(circle);
        } else if (shape == ColliderType::CAPSULE) {
            glm::vec2 localA = readVec2(specs, "localAX", "localAY", glm::vec2{0.5f, 0.0f});
            glm::vec2 localB = readVec2(specs, "localBX", "localBY", glm::vec2{-0.5f, 0.0f});
            const float radius = std::max(0.0f, readNumber(specs, "radius", 0.25f));
            auto capsule = std::make_unique<CapsuleCollider>(localA, localB, radius);
            capsule->setLocalOffset(readVec2(specs, "offsetX", "offsetY", glm::vec2{0.0f, 0.0f}));
            collider = std::move(capsule);
        } else {
            glm::vec2 min = readVec2(specs, "minX", "minY", glm::vec2{-0.5f, -0.5f});
            glm::vec2 max = readVec2(specs, "maxX", "maxY", glm::vec2{0.5f, 0.5f});
            collider = std::make_unique<AABBCollider>(min, max);
        }
        const float padding = readNumber(specs, "padding", 0.0f);
        auto comp = std::make_unique<ColliderComponent>(std::move(collider), shape, padding);
        if (auto layerIt = specs.numbers.find("layer"); layerIt != specs.numbers.end()) {
            comp->setLayer(static_cast<uint32_t>(std::clamp(layerIt->second, 0.0f, 31.0f)));
        }
        if (auto maskIt = specs.numbers.find("mask"); maskIt != specs.numbers.end()) {
            comp->setCollisionMask(static_cast<uint32_t>(maskIt->second));
        }
        if (auto triggerIt = specs.numbers.find("trigger"); triggerIt != specs.numbers.end()) {
            bool triggerOnce = false;
            if (auto onceIt = specs.numbers.find("triggerOnce"); onceIt != specs.numbers.end()) {
                triggerOnce = onceIt->second != 0.0f;
            }
            comp->setTrigger(triggerIt->second != 0.0f, triggerOnce);
        }
        return comp;
    }

    if (specs.type == "RigidBody") {
        float mass = std::max(0.0f, readNumber(specs, "mass", 1.0f));
        auto bodyType = RigidBodyType::DYNAMIC;
        if (auto typeIt = specs.strings.find("bodyType"); typeIt != specs.strings.end()) {
            bodyType = parseRigidBodyType(toLower(typeIt->second));
        }
        auto body = std::make_unique<RigidBody>(mass, bodyType);
        if (auto linearIt = specs.numbers.find("linearDamping"); linearIt != specs.numbers.end()) {
            body->setLinearDamping(linearIt->second);
        }
        if (auto angularIt = specs.numbers.find("angularDamping"); angularIt != specs.numbers.end()) {
            body->setAngularDamping(angularIt->second);
        }
        if (auto inertiaIt = specs.numbers.find("inertia"); inertiaIt != specs.numbers.end()) {
            body->setInertia(std::max(0.0f, inertiaIt->second));
        }
        return std::make_unique<RigidBodyComponent>(std::move(body));
    }

    if (specs.type == "Hinge") {
        auto hinge = std::make_unique<HingeComponent>();
        hinge->setAnchorSelf(readVec2(specs, "anchorSelfX", "anchorSelfY", glm::vec2{0.0f, 0.0f}));
        hinge->setAnchorTarget(readVec2(specs, "anchorTargetX", "anchorTargetY", glm::vec2{0.0f, 0.0f}));
        hinge->setReferenceAngle(readNumber(specs, "referenceAngle", 0.0f));
        const bool limitsEnabled = readNumber(specs, "limitsEnabled", 0.0f) != 0.0f;
        hinge->enableLimits(limitsEnabled);
        hinge->setLimitRange(readNumber(specs, "lowerLimit", 0.0f), readNumber(specs, "upperLimit", 0.0f));
        hinge->setLimitParameters(readNumber(specs, "limitStiffness", 10.0f),
                                  readNumber(specs, "limitDamping", 1.0f),
                                  readNumber(specs, "maxLimitTorque", 10.0f));
        const bool motorEnabled = readNumber(specs, "motorEnabled", 0.0f) != 0.0f;
        hinge->enableMotor(motorEnabled);
        hinge->setMotorSpeed(readNumber(specs, "motorSpeed", 0.0f));
        hinge->setMotorParameters(readNumber(specs, "motorStiffness", 5.0f),
                                  readNumber(specs, "maxMotorTorque", 10.0f));
        return hinge;
    }

    // Unknown type -> return nullptr for now (or throw/assert)
    return nullptr;
}
