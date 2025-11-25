//
// Created by Mohamad on 25/11/2025.
//

#ifndef GL2D_PARTICLEEMITTER_HPP
#define GL2D_PARTICLEEMITTER_HPP
#include <vector>
#include <random>
#include <cstdlib>
#include "ParticleEmitterConfig.hpp"
#include "Particle.hpp"

namespace Rendering{class Renderer; class ParticleRenderer;}
class ParticleEmitter {
public:
    explicit ParticleEmitter(std::size_t maxParticles, const ParticleEmitterConfig& config);

    virtual ~ParticleEmitter();

    ParticleEmitter(const ParticleEmitter &other) = delete;

    ParticleEmitter &operator=(const ParticleEmitter &other) = delete;

    ParticleEmitter(ParticleEmitter &&other) = delete;

    ParticleEmitter &operator=(ParticleEmitter &&other) = delete;

    void setPosition(const glm::vec2& pos);
    const glm::vec2& getPosition()const;
    void setTarget(const glm::vec2& target);
    void setConfig(const ParticleEmitterConfig& cfg);
    const ParticleEmitterConfig& getConfig() const;
    const std::vector<Particle>& getParticles() const { return m_particles; }
    void update(float dt);
    void render(Rendering::ParticleRenderer& renderer) const;
    void burst(unsigned  int count);
private:
    glm::vec2 m_position{0.0f,0.0f};
    ParticleEmitterConfig m_config;
    std::vector<Particle> m_particles;
    float m_spawnAccumulator{0.0f};
    glm::vec2 m_target{0.0f,0.0f};

    mutable std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_unitDist;

    void spawnOne();
};


#endif //GL2D_PARTICLEEMITTER_HPP
