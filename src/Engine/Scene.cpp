//
// Created by Mohamad on 21/11/2025.
//

#include "Scene.hpp"
#include "ECS/Components/SmoothedTransform2D.hpp"
#include "ECS/Components/Transform2D.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "GameObjects/Components/HingeComponent.hpp"
#include "Physics/RigidBody.hpp"
#include "GameObjects/Components/TransformFollowerComponent.hpp"
#include "GameObjects/Components/RopeSegmentComponent.hpp"
#include "GameObjects/Components/AnimatorComponent.hpp"
#include "GameObjects/Components/ControllerComponent.hpp"
#include "RenderingSystem/RenderSystem.hpp"
#include "ECS/Systems/CharacterMotorSystem.hpp"
#include "ECS/Systems/ClimbingSystem2D.hpp"
#include "ECS/Systems/KinematicCharacterPhysicsSystem.hpp"
#include "ECS/Systems/AnimationSystem2D.hpp"
#include "ECS/Systems/ParticleSystem2D.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

void Scene::setAmbientLight(glm::vec3 color) {
    if (!std::isfinite(color.x) || !std::isfinite(color.y) ||
        !std::isfinite(color.z) || color.x < 0.0f || color.y < 0.0f ||
        color.z < 0.0f) {
        throw std::invalid_argument(
            "Scene ambient light must contain finite, non-negative values");
    }
    m_ambientLight = color;
}

void Scene::setClearColor(const glm::vec4& color) {
    if (!std::isfinite(color.x) || !std::isfinite(color.y) ||
        !std::isfinite(color.z) || !std::isfinite(color.w) ||
        color.x < 0.0f || color.y < 0.0f || color.z < 0.0f || color.w < 0.0f) {
        throw std::invalid_argument(
            "Scene clear color must contain finite, non-negative values");
    }
    m_clearColor = color;
}

void Scene::snapshotTransformsForInterpolation() {
    // Legacy entities: remember mobile transforms at the start of the step so
    // render extraction can interpolate between steps. Entities that cannot
    // move (no controller, follower, or non-static body) are skipped — they
    // render identically without history.
    for (const auto& entity : m_entities) {
        if (!entity) continue;
        const auto* transform = entity->getComponent<TransformComponent>();
        if (!transform) continue;
        bool mobile = entity->getComponent<ControllerComponent>() != nullptr ||
                      entity->getComponent<TransformFollowerComponent>() != nullptr;
        if (!mobile) {
            if (const auto* rigidBody = entity->getComponent<RigidBodyComponent>()) {
                mobile = rigidBody->body() &&
                         rigidBody->body()->getBodyType() != RigidBodyType::STATIC;
            }
        }
        if (mobile) {
            m_previousPositions[entity->getId()] =
                transform->getTransform().Position;
        }
    }

    // ECS entities opt in through SmoothedTransform2D. History components are
    // added outside the query because structural changes are rejected inside.
    m_smoothedNeedingHistory.clear();
    m_ecsRegistry.each<ECS::Transform2D, ECS::SmoothedTransform2D>(
        [this](ECS::Entity entity, const ECS::Transform2D& transform,
               const ECS::SmoothedTransform2D&) {
            if (auto* previous =
                    m_ecsRegistry.tryGet<ECS::PreviousTransform2D>(entity)) {
                previous->value = transform;
            } else {
                m_smoothedNeedingHistory.push_back(entity);
            }
        });
    for (const ECS::Entity entity : m_smoothedNeedingHistory) {
        m_ecsRegistry.emplace<ECS::PreviousTransform2D>(entity).value =
            m_ecsRegistry.get<ECS::Transform2D>(entity);
    }
}

Entity &Scene::createEntity() {
    return addEntity(std::make_unique<Entity>());
}

Entity &Scene::addEntity(std::unique_ptr<Entity> entity) {
    if (!entity) {
        throw std::invalid_argument("Scene::addEntity requires a non-null entity");
    }

    Entity& result = *entity;
    if (m_updating) {
        m_pendingAdditions.push_back(std::move(entity));
    } else {
        m_entities.push_back(std::move(entity));
    }
    return result;
}

void Scene::destroyEntity(Entity &entity) {
    const auto pendingIt = std::find_if(m_pendingAdditions.begin(), m_pendingAdditions.end(), [&entity](const auto& ptr) {
        return ptr.get() == &entity;
    });
    if (pendingIt != m_pendingAdditions.end()) {
        detachLegacyEntityReferences(&entity);
        m_pendingAdditions.erase(pendingIt);
        return;
    }

    const auto it = std::find_if(m_entities.begin(), m_entities.end(), [&entity](const auto& ptr) {
        return ptr.get() == &entity;
    });
    if (it == m_entities.end()) {
        return;
    }

    if (m_updating) {
        detachLegacyEntityReferences(&entity);
        m_pendingDestructions.insert(entity.getId());
    } else {
        detachLegacyEntityReferences(&entity);
        m_triggerSystem.unregisterEntity(entity.getId());
        m_previousPositions.erase(entity.getId());
        m_entities.erase(it);
    }
}

void Scene::detachLegacyEntityReferences(const Entity* target) {
    const auto detach = [target](auto& entities) {
        for (auto& candidate : entities) {
            if (!candidate) continue;
            for (const auto& component : candidate->components()) {
                if (auto* hinge = dynamic_cast<HingeComponent*>(component.get());
                    hinge && hinge->target() == target) {
                    hinge->setTarget(nullptr);
                }
                if (auto* follower = dynamic_cast<TransformFollowerComponent*>(component.get());
                    follower && follower->target() == target) {
                    follower->setTarget(nullptr);
                }
                if (auto* rope = dynamic_cast<RopeSegmentComponent*>(component.get())) {
                    if (rope->previous() == target) rope->setPrevious(nullptr);
                    if (rope->next() == target) rope->setNext(nullptr);
                }
            }
        }
    };
    detach(m_entities);
    detach(m_pendingAdditions);
}

void Scene::clear() {
    if (m_updating) {
        m_clearPending = true;
        m_pendingAdditions.clear();
        m_pendingDestructions.clear();
        return;
    }
    m_triggerSystem.clear();
    m_entities.clear();
    m_ecsRegistry.clear();
    m_previousPositions.clear();
    m_fixedClock.reset();
}

Engine::FixedStepClock::Result Scene::advance(float frameDeltaTime) {
    ECS::AnimationSystem2D::beginFrame(m_ecsRegistry);
    if (!std::isfinite(frameDeltaTime) || frameDeltaTime < 0.0f) {
        throw std::invalid_argument(
            "Scene::advance requires a finite, non-negative frame delta");
    }
    // Presentation blends use unscaled frame time and advance exactly once per
    // frame. They remain responsive while simulation is paused or slowed.
    m_feelingsSystem.update(frameDeltaTime * 1000.0f);
    if (m_paused) {
        m_fixedClock.reset();
        return {};
    }
    const float timeScale = m_feelingsSystem.getSnapshot().timeScale.value_or(1.0f);
    const float simulationDelta = frameDeltaTime * timeScale;
    if (!std::isfinite(simulationDelta)) {
        throw std::overflow_error("Scene time-scale multiplication overflowed");
    }
    return m_fixedClock.advance(simulationDelta, [this](float fixedDeltaTime) {
        update(fixedDeltaTime);
    });
}

void Scene::flushPendingMutations() {
    if (m_clearPending) {
        m_triggerSystem.clear();
        m_entities.clear();
        m_ecsRegistry.clear();
        m_previousPositions.clear();
        m_pendingDestructions.clear();
        m_clearPending = false;
    } else if (!m_pendingDestructions.empty()) {
        for (const uint64_t id : m_pendingDestructions) {
            m_triggerSystem.unregisterEntity(id);
            m_previousPositions.erase(id);
        }
        std::erase_if(m_entities, [this](const auto& entity) {
            return m_pendingDestructions.contains(entity->getId());
        });
        m_pendingDestructions.clear();
    }

    for (auto& entity : m_pendingAdditions) {
        m_entities.push_back(std::move(entity));
    }
    m_pendingAdditions.clear();
}

void Scene::update(float deltaTime) {
    if (m_paused) {
        return;
    }
    if (!std::isfinite(deltaTime) || deltaTime < 0.0f) {
        throw std::invalid_argument("Scene::update requires a finite, non-negative delta time");
    }
    if (m_updating) {
        throw std::logic_error("Scene::update cannot be called recursively");
    }

    m_updating = true;
    try {
        snapshotTransformsForInterpolation();
        const FeelingsSystem::FeelingSnapshot& feeling =
            m_feelingsSystem.getSnapshot();
        const float entitySpeed = feeling.entitySpeedMul.value_or(1.0f);
        const float accelerationSpeed =
            feeling.accelerationSpeedMul.value_or(entitySpeed);
        const float animationSpeed =
            feeling.animationSpeedMul.value_or(1.0f);
        ECS::ClimbingSystem2D::update(m_ecsRegistry);
        ECS::CharacterMotorSystem::update(
            m_ecsRegistry, deltaTime, entitySpeed, accelerationSpeed);
        ECS::KinematicCharacterPhysicsSystem::update(m_ecsRegistry, deltaTime);
        ECS::CharacterAnimationParameterSystem2D::update(m_ecsRegistry);
        ECS::AnimationSystem2D::update(
            m_ecsRegistry, deltaTime, animationSpeed);
        ECS::ParticleSystem2D::update(m_ecsRegistry, deltaTime);
        for (auto& e : m_entities) {
            if (m_clearPending) {
                break;
            }
            if (m_pendingDestructions.contains(e->getId())) {
                continue;
            }
            if (auto* controller = e->getComponent<ControllerComponent>()) {
                controller->applyFeeling(feeling);
            }
            if (auto* animator = e->getComponent<AnimatorComponent>()) {
                animator->setPlaybackSpeed(animationSpeed);
            }
            e->update(deltaTime);
        }
        flushPendingMutations();
        m_waterSystem.update(deltaTime, m_entities, m_physicsEngine.getGravity());
        m_physicsEngine.step(deltaTime, m_entities);
        m_triggerSystem.update(m_entities);
    } catch (...) {
        m_updating = false;
        flushPendingMutations();
        throw;
    }
    m_updating = false;
    flushPendingMutations();
}

void Scene::updateWorld(float deltaTime, Camera &camera, Rendering::Renderer &renderer) {
    advance(deltaTime);
    camera.applyFeeling(m_feelingsSystem.getSnapshot());
    camera.update(deltaTime);
    RenderSystem::renderScene(*this, camera, renderer);
}

std::vector<std::unique_ptr<Entity>> &Scene::getEntities() {
    return m_entities;
}

const std::vector<std::unique_ptr<Entity>> &Scene::getEntities() const {
    return m_entities;
}
