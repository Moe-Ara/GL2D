//
// Created by Mohamad on 25/11/2025.
//

#ifndef GL2D_PARTICLE_HPP
#define GL2D_PARTICLE_HPP

#include <glm/glm.hpp>

struct Particle{
    glm::vec2 position{};
    glm::vec2 velocity{};
    glm::vec2 acceleration{0.f,0.f};

    float rotation{0.0f};
    float angularVelocity{0.0f};

    float lifeTime{1.0f};
    float age{0.0f};

    glm::vec2 size{1.0f,1.0f};
    glm::vec4 color{1.0f};
    bool alive{false};
};
#endif //GL2D_PARTICLE_HPP
