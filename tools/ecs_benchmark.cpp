#include "ECS/Registry.hpp"
#include "GameObjects/Entity.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

namespace {
struct LegacyPosition : IComponent { float x{0.0f}; float y{0.0f}; };
struct LegacyVelocity : IComponent { float x{1.0f}; float y{2.0f}; };
struct Position { float x{0.0f}; float y{0.0f}; };
struct Velocity { float x{1.0f}; float y{2.0f}; };

template<typename Function>
double measureMilliseconds(Function&& function) {
    const auto start = std::chrono::steady_clock::now();
    function();
    return std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - start).count();
}
}

int main(int argc, char** argv) {
    const std::size_t entityCount = argc > 1 ? std::strtoull(argv[1], nullptr, 10) : 50'000;
    const int frameCount = argc > 2 ? std::atoi(argv[2]) : 200;
    if (entityCount == 0 || frameCount <= 0) {
        std::cerr << "Usage: GL2D_ECS_BENCHMARK [positive entity count] [positive frame count]\n";
        return 2;
    }

    std::vector<std::unique_ptr<Entity>> legacy;
    legacy.reserve(entityCount);
    for (std::size_t i = 0; i < entityCount; ++i) {
        auto entity = std::make_unique<Entity>();
        entity->addComponent<LegacyPosition>();
        entity->addComponent<LegacyVelocity>();
        legacy.push_back(std::move(entity));
    }
    const double legacyMs = measureMilliseconds([&] {
        for (int frame = 0; frame < frameCount; ++frame) {
            for (const auto& entity : legacy) {
                auto& position = *entity->getComponent<LegacyPosition>();
                const auto& velocity = *entity->getComponent<LegacyVelocity>();
                position.x += velocity.x;
                position.y += velocity.y;
            }
        }
    });

    ECS::Registry registry;
    ECS::Entity firstEcsEntity;
    for (std::size_t i = 0; i < entityCount; ++i) {
        const ECS::Entity entity = registry.create();
        if (i == 0) firstEcsEntity = entity;
        registry.emplace<Position>(entity);
        registry.emplace<Velocity>(entity);
    }
    const double ecsMs = measureMilliseconds([&] {
        for (int frame = 0; frame < frameCount; ++frame) {
            registry.each<Position, Velocity>(
                [](ECS::Entity, Position& position, const Velocity& velocity) {
                    position.x += velocity.x;
                    position.y += velocity.y;
                });
        }
    });

    const float legacyResult = legacy.front()->getComponent<LegacyPosition>()->x;
    const float ecsResult = registry.get<Position>(firstEcsEntity).x;
    if (legacyResult != ecsResult) {
        std::cerr << "Benchmark implementations produced different results\n";
        return 1;
    }

    std::cout << "entities=" << entityCount << " frames=" << frameCount << '\n'
              << "legacy_ms=" << legacyMs << '\n'
              << "ecs_ms=" << ecsMs << '\n'
              << "speedup=" << legacyMs / ecsMs << "x\n";
}
