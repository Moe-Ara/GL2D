//
// Created by Mohamad on 25/11/2025.
//

#ifndef GL2D_PARTICLEEMITTERCONFIG_HPP
#define GL2D_PARTICLEEMITTERCONFIG_HPP
#include <glm/glm.hpp>
#include <cstdint>
#include <cstddef>

struct ParticleEmitterConfig{
    float spawnRate{50.0f};
    unsigned int burstCount{0};

    float minLifeTime{0.5f};
    float maxLifeTime{1.5f};

    float minSpeed{10.0f};
    float maxSpeed{50.0f};

    float direction{0.0f};
    float spread{3.14f};

    float minSize{4.0f};
    float maxSize{8.0f};
    float endSizeMultiplier{1.0f};

    float minAngularVelocity{0.0f};
    float maxAngularVelocity{0.0f};

    glm::vec4 startColor{1.0f};
    glm::vec4 endColor{1.0f,1.0f,1.0f,1.0f};

    glm::vec2 gravity{0.0f, -200.0f};
    float drag{0.0f};

    // Optional motion steering
    float homingStrength{0.0f};   // Acceleration toward target point
    float orbitStrength{0.0f};    // Tangential push around target point
    float spiralStrength{0.0f};   // Radial push (outward if positive) from target point

    // Stable by default so fixed-step replays and tests reproduce exactly.
    std::uint32_t randomSeed{0x6d2b79f5u};
};

// Throws ParticleException with a stable diagnostic when authored values cannot
// be simulated safely. Validation occurs before allocating the particle pool.
void validateParticleEmitterConfig(std::size_t capacity,
                                   const ParticleEmitterConfig& config);
#endif //GL2D_PARTICLEEMITTERCONFIG_HPP
