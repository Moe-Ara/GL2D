//
// Created by Mohamad on 25/11/2025.
//

#ifndef GL2D_PARTICLEEMITTER_HPP
#define GL2D_PARTICLEEMITTER_HPP
#include <vector>
#include <random>
#include "ParticleEmitterConfig.hpp"
#include "Particle.hpp"

class ParticleEmitter {
public:
    explicit ParticleEmitter(std::size_t maxParticles, const ParticleEmitterConfig& config);

    ~ParticleEmitter() = default;

    ParticleEmitter(const ParticleEmitter &other) = delete;

    ParticleEmitter &operator=(const ParticleEmitter &other) = delete;

    ParticleEmitter(ParticleEmitter &&other) noexcept = default;

    ParticleEmitter &operator=(ParticleEmitter &&other) noexcept = default;

    void setPosition(const glm::vec2& pos);
    const glm::vec2& getPosition()const;
    void setTarget(const glm::vec2& target);
    void setConfig(const ParticleEmitterConfig& cfg);
    const ParticleEmitterConfig& getConfig() const;
    const std::vector<Particle>& getParticles() const { return m_particles; }
    [[nodiscard]] std::size_t liveParticleCount() const noexcept { return m_liveParticleCount; }
    [[nodiscard]] bool isFinished() const noexcept { return m_liveParticleCount == 0; }
    void setEmitting(bool emitting) noexcept { m_emitting = emitting; }
    [[nodiscard]] bool isEmitting() const noexcept { return m_emitting; }
    void update(float dt);
    void burst(unsigned  int count);
private:
    glm::vec2 m_position{0.0f,0.0f};
    ParticleEmitterConfig m_config;
    std::vector<Particle> m_particles;
    double m_spawnAccumulator{0.0};
    std::size_t m_nextFreeIndex{0};
    std::size_t m_liveParticleCount{0};
    bool m_emitting{true};
    glm::vec2 m_target{0.0f,0.0f};

    mutable std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_unitDist;

    bool spawnOne();
};


#endif //GL2D_PARTICLEEMITTER_HPP
