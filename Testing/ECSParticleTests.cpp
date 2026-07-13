#include <boost/test/unit_test.hpp>

#include "EcsTestOutput.hpp"

#include "ECS/Components/ParticleEmitter2D.hpp"
#include "ECS/Components/ParticleRender2D.hpp"
#include "ECS/Components/Transform2D.hpp"
#include "ECS/Registry.hpp"
#include "ECS/Systems/ParticleSystem2D.hpp"
#include "Exceptions/SubsystemExceptions.hpp"

#include <cmath>
#include <limits>

namespace {
ParticleEmitterConfig fixedConfig() {
    ParticleEmitterConfig config{};
    config.spawnRate = 0.0f;
    config.minLifeTime = 2.0f;
    config.maxLifeTime = 2.0f;
    config.minSpeed = 10.0f;
    config.maxSpeed = 10.0f;
    config.direction = 0.0f;
    config.spread = 0.0f;
    config.minSize = 10.0f;
    config.maxSize = 10.0f;
    config.endSizeMultiplier = 0.0f;
    config.gravity = {0.0f, 0.0f};
    config.drag = std::log(2.0f);
    config.randomSeed = 42;
    return config;
}
}

BOOST_AUTO_TEST_SUITE(ECSParticleTests)

BOOST_AUTO_TEST_CASE(equal_seeds_produce_identical_particles) {
    ParticleEmitter first{8, fixedConfig()};
    ParticleEmitter second{8, fixedConfig()};
    first.setPosition({3.0f, 4.0f});
    second.setPosition({3.0f, 4.0f});
    first.burst(4);
    second.burst(4);

    for (std::size_t index = 0; index < 4; ++index) {
        const Particle& a = first.getParticles()[index];
        const Particle& b = second.getParticles()[index];
        BOOST_TEST(a.position.x == b.position.x);
        BOOST_TEST(a.velocity.x == b.velocity.x);
        BOOST_TEST(a.lifeTime == b.lifeTime);
        BOOST_TEST(a.size.x == b.size.x);
    }
}

BOOST_AUTO_TEST_CASE(drag_and_size_curves_follow_elapsed_time) {
    ParticleEmitter emitter{1, fixedConfig()};
    emitter.burst(1);
    emitter.update(1.0f);

    const Particle& particle = emitter.getParticles().front();
    BOOST_TEST(particle.velocity.x == 5.0f,
               boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(particle.size.x == 5.0f,
               boost::test_tools::tolerance(0.0001f));
}

BOOST_AUTO_TEST_CASE(system_synchronizes_transform_before_deferred_burst) {
    ECS::Registry registry;
    const ECS::Entity entity = registry.create();
    auto& transform = registry.emplace<ECS::Transform2D>(entity);
    transform.position = {10.0f, 20.0f};
    transform.rotationDegrees = 90.0f;
    auto& component = registry.emplace<ECS::ParticleEmitter2D>(
        entity, 4, fixedConfig());
    component.localOffset = {2.0f, 0.0f};
    component.emitting = false;
    component.requestBurst(1);

    ECS::ParticleSystem2D::update(registry, 0.01f);

    const Particle& particle = component.emitter.getParticles().front();
    BOOST_TEST(particle.position.x == 10.0f,
               boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(particle.position.y == 22.0f,
               boost::test_tools::tolerance(0.0001f));
}

BOOST_AUTO_TEST_CASE(transient_emitter_destroys_entity_after_last_particle) {
    ECS::Registry registry;
    const ECS::Entity entity = registry.create();
    registry.emplace<ECS::Transform2D>(entity);
    ParticleEmitterConfig config = fixedConfig();
    config.minLifeTime = 0.05f;
    config.maxLifeTime = 0.05f;
    auto& component = registry.emplace<ECS::ParticleEmitter2D>(entity, 2, config);
    component.emitting = false;
    component.autoDestroyWhenFinished = true;
    component.requestBurst(1);

    // Spawn tick: the deferred burst is applied after integration, so the
    // particle ages on the following two ticks and expires on the second.
    ECS::ParticleSystem2D::update(registry, 0.025f);
    BOOST_TEST(registry.alive(entity));
    ECS::ParticleSystem2D::update(registry, 0.025f);
    BOOST_TEST(registry.alive(entity));
    ECS::ParticleSystem2D::update(registry, 0.025f);
    BOOST_TEST(!registry.alive(entity));
}

BOOST_AUTO_TEST_CASE(invalid_capacity_and_non_finite_config_are_rejected) {
    BOOST_CHECK_THROW(ParticleEmitter(0, fixedConfig()),
                      Engine::ParticleException);
    auto config = fixedConfig();
    config.spawnRate = std::numeric_limits<float>::quiet_NaN();
    BOOST_CHECK_THROW(ParticleEmitter(8, config), Engine::ParticleException);
}

BOOST_AUTO_TEST_SUITE_END()
