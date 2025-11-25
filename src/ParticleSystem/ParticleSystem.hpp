//
// Created by Mohamad on 25/11/2025.
//

#ifndef GL2D_PARTICLESYSTEM_HPP
#define GL2D_PARTICLESYSTEM_HPP

#include "ParticleEmitter.hpp"

namespace Rendering{
    class ParticleRenderer;
}
class ParticleSystem {
public:
    ParticleSystem();

    virtual ~ParticleSystem();

    ParticleSystem(const ParticleSystem &other) = delete;

    ParticleSystem &operator=(const ParticleSystem &other) = delete;

    ParticleSystem(ParticleSystem &&other) = delete;

    ParticleSystem &operator=(ParticleSystem &&other) = delete;

    ParticleEmitter* createEmitter(std::size_t maxParticles, const ParticleEmitterConfig& cfg);
    void update(float dt);
    void render(Rendering::ParticleRenderer& renderer) const;
private:
    std::vector<std::unique_ptr<ParticleEmitter>> m_emitters;
};


#endif //GL2D_PARTICLESYSTEM_HPP
