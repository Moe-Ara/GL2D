#include "ParticleSystem/ParticleEmitterConfig.hpp"

#include "Exceptions/SubsystemExceptions.hpp"

#include <array>
#include <algorithm>
#include <cmath>

namespace {
bool finiteColor(const glm::vec4& color) {
    return std::isfinite(color.x) && std::isfinite(color.y) &&
           std::isfinite(color.z) && std::isfinite(color.w) &&
           color.x >= 0.0f && color.y >= 0.0f && color.z >= 0.0f &&
           color.w >= 0.0f && color.w <= 1.0f;
}
}

void validateParticleEmitterConfig(std::size_t capacity,
                                   const ParticleEmitterConfig& config) {
    constexpr std::size_t maxCapacity = 1'000'000;
    if (capacity == 0 || capacity > maxCapacity) {
        throw Engine::ParticleException(
            "Particle capacity must be between 1 and 1000000");
    }
    const std::array values{
        config.spawnRate, config.minLifeTime, config.maxLifeTime,
        config.minSpeed, config.maxSpeed, config.direction, config.spread,
        config.minSize, config.maxSize, config.endSizeMultiplier,
        config.minAngularVelocity, config.maxAngularVelocity,
        config.gravity.x, config.gravity.y, config.drag,
        config.homingStrength, config.orbitStrength, config.spiralStrength
    };
    if (!std::ranges::all_of(values, [](float value) { return std::isfinite(value); })) {
        throw Engine::ParticleException(
            "Particle emitter numeric fields must be finite");
    }
    if (config.spawnRate < 0.0f) {
        throw Engine::ParticleException("Particle spawnRate cannot be negative");
    }
    if (config.minLifeTime <= 0.0f || config.maxLifeTime < config.minLifeTime) {
        throw Engine::ParticleException(
            "Particle lifetime range must be positive and ordered");
    }
    if (config.minSpeed < 0.0f || config.maxSpeed < config.minSpeed) {
        throw Engine::ParticleException(
            "Particle speed range must be non-negative and ordered");
    }
    if (config.spread < 0.0f) {
        throw Engine::ParticleException("Particle spread cannot be negative");
    }
    if (config.minSize <= 0.0f || config.maxSize < config.minSize ||
        config.endSizeMultiplier < 0.0f) {
        throw Engine::ParticleException(
            "Particle size range must be positive and ordered; endSizeMultiplier cannot be negative");
    }
    if (config.maxAngularVelocity < config.minAngularVelocity) {
        throw Engine::ParticleException(
            "Particle angular velocity range must be ordered");
    }
    if (config.drag < 0.0f || config.homingStrength < 0.0f ||
        config.orbitStrength < 0.0f) {
        throw Engine::ParticleException(
            "Particle drag, homing, and orbit strengths cannot be negative");
    }
    if (!finiteColor(config.startColor) || !finiteColor(config.endColor)) {
        throw Engine::ParticleException(
            "Particle colors require non-negative finite RGB and alpha in [0, 1]");
    }
}
