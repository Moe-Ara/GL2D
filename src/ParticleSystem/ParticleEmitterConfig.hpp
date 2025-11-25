//
// Created by Mohamad on 25/11/2025.
//

#ifndef GL2D_PARTICLEEMITTERCONFIG_HPP
#define GL2D_PARTICLEEMITTERCONFIG_HPP
#include <glm/glm.hpp>
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

    glm::vec4 startColor{1.0f};
    glm::vec4 endColor{1.0f,1.0f,1.0f,1.0f};

    glm::vec2 gravity{0.0f, -200.0f};
    float drag{0.0f};

    // Optional motion steering
    float homingStrength{0.0f};   // Acceleration toward target point
    float orbitStrength{0.0f};    // Tangential push around target point
    float spiralStrength{0.0f};   // Radial push (outward if positive) from target point
};
#endif //GL2D_PARTICLEEMITTERCONFIG_HPP
