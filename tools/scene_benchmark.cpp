// Headless full-scene stepping benchmark: legacy physics entities, triggers,
// ECS kinematic movers, smoothed transforms, and particles advanced through
// Scene::advance. No GL context required. Used to validate the frame budget
// for a LOWTIDE-scale scene and to measure engine optimizations.

#include "ECS/Components/CharacterMotor.hpp"
#include "ECS/Components/Collision2D.hpp"
#include "ECS/Components/ParticleEmitter2D.hpp"
#include "ECS/Components/SmoothedTransform2D.hpp"
#include "ECS/Components/Transform2D.hpp"
#include "ECS/Registry.hpp"
#include "Engine/Scene.hpp"
#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Entity.hpp"
#include "ParticleSystem/ParticleEmitterConfig.hpp"
#include "Physics/Collision/AABBCollider.hpp"
#include "Physics/Collision/CircleCollider.hpp"
#include "Physics/RigidBody.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

namespace {

void buildLegacyWorld(Scene& scene, std::size_t dynamicBodies,
                      std::size_t triggerVolumes) {
    // Ground strip.
    for (int i = 0; i < 10; ++i) {
        Entity& ground = scene.createEntity();
        auto& transform = ground.addComponent<TransformComponent>();
        transform.setPosition({static_cast<float>(i) * 2000.0f - 10000.0f, 0.0f});
        ground.addComponent<ColliderComponent>(
            std::make_unique<AABBCollider>(glm::vec2{0.0f, 0.0f},
                                           glm::vec2{2000.0f, 80.0f}));
        auto body = std::make_unique<RigidBody>(0.0f, RigidBodyType::STATIC);
        body->setTransform(&transform.getTransform());
        ground.addComponent<RigidBodyComponent>(std::move(body));
    }

    // Falling/rolling dynamic props.
    for (std::size_t i = 0; i < dynamicBodies; ++i) {
        Entity& prop = scene.createEntity();
        auto& transform = prop.addComponent<TransformComponent>();
        const float x = static_cast<float>(i % 100) * 45.0f - 2200.0f;
        const float y = 200.0f + static_cast<float>(i / 100) * 90.0f;
        transform.setPosition({x, y});
        prop.addComponent<ColliderComponent>(
            std::make_unique<CircleCollider>(20.0f));
        auto body = std::make_unique<RigidBody>(1.0f, RigidBodyType::DYNAMIC);
        body->setTransform(&transform.getTransform());
        body->setFriction(0.3f);
        body->setRestitution(0.1f);
        prop.addComponent<RigidBodyComponent>(std::move(body));
    }

    // Trigger gates.
    for (std::size_t i = 0; i < triggerVolumes; ++i) {
        Entity& gate = scene.createEntity();
        auto& transform = gate.addComponent<TransformComponent>();
        transform.setPosition({static_cast<float>(i) * 400.0f - 9000.0f, 0.0f});
        auto& collider = gate.addComponent<ColliderComponent>(
            std::make_unique<AABBCollider>(glm::vec2{-40.0f, 0.0f},
                                           glm::vec2{40.0f, 1000.0f}));
        collider.setTrigger(true);
    }
}

void buildEcsWorld(Scene& scene, std::size_t staticSprites,
                   std::size_t kinematicMovers, std::size_t particleEmitters) {
    auto& registry = scene.registry();

    for (std::size_t i = 0; i < staticSprites; ++i) {
        const ECS::Entity entity = registry.create();
        auto& transform = registry.emplace<ECS::Transform2D>(entity);
        transform.position = {static_cast<float>(i % 200) * 60.0f,
                              static_cast<float>(i / 200) * 40.0f};
    }

    for (std::size_t i = 0; i < kinematicMovers; ++i) {
        const ECS::Entity entity = registry.create();
        auto& transform = registry.emplace<ECS::Transform2D>(entity);
        transform.position = {static_cast<float>(i % 50) * 120.0f - 3000.0f,
                              120.0f + static_cast<float>(i / 50) * 60.0f};
        registry.emplace<ECS::SmoothedTransform2D>(entity);
        registry.emplace<ECS::AabbCollider2D>(entity).halfExtents = {20.0f, 30.0f};
        registry.emplace<ECS::KinematicBody2D>(entity).velocity =
            {(i % 2 == 0) ? 80.0f : -80.0f, 0.0f};
        registry.emplace<ECS::GroundContact2D>(entity);
        registry.emplace<ECS::CharacterCollisionState2D>(entity);
    }

    ParticleEmitterConfig config{};
    config.spawnRate = 120.0f;
    config.minLifeTime = 0.8f;
    config.maxLifeTime = 1.6f;
    config.minSpeed = 20.0f;
    config.maxSpeed = 60.0f;
    config.minSize = 2.0f;
    config.maxSize = 6.0f;
    config.randomSeed = 7;
    for (std::size_t i = 0; i < particleEmitters; ++i) {
        const ECS::Entity entity = registry.create();
        registry.emplace<ECS::Transform2D>(entity).position =
            {static_cast<float>(i) * 500.0f, 300.0f};
        auto& emitter =
            registry.emplace<ECS::ParticleEmitter2D>(entity, 256, config);
        emitter.emitting = true;
    }
}

} // namespace

int main(int argc, char** argv) {
    const int frames = argc > 1 ? std::atoi(argv[1]) : 600;
    if (frames <= 0) {
        std::cerr << "Usage: GL2D_SCENE_BENCHMARK [positive frame count]\n";
        return 2;
    }

    Scene scene;
    buildLegacyWorld(scene, /*dynamicBodies=*/150, /*triggerVolumes=*/50);
    buildEcsWorld(scene, /*staticSprites=*/2000, /*kinematicMovers=*/500,
                  /*particleEmitters=*/6);

    // Warm-up so props settle and pools fill before measurement.
    for (int i = 0; i < 120; ++i) {
        scene.advance(1.0f / 60.0f);
    }

    std::vector<double> frameMs;
    frameMs.reserve(static_cast<std::size_t>(frames));
    for (int i = 0; i < frames; ++i) {
        const auto start = std::chrono::steady_clock::now();
        scene.advance(1.0f / 60.0f);
        frameMs.push_back(std::chrono::duration<double, std::milli>(
                              std::chrono::steady_clock::now() - start)
                              .count());
    }

    double total = 0.0;
    for (const double ms : frameMs) total += ms;
    std::vector<double> sorted = frameMs;
    std::sort(sorted.begin(), sorted.end());
    const double average = total / static_cast<double>(frameMs.size());
    const double p99 = sorted[static_cast<std::size_t>(
        static_cast<double>(sorted.size() - 1) * 0.99)];

    std::cout << "frames=" << frames
              << " avg_ms=" << average
              << " p99_ms=" << p99
              << " max_ms=" << sorted.back() << "\n";
    return 0;
}
