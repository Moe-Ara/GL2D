//
// Created by Mohamad on 25/11/2025.
//

#include "ParticleSystem.hpp"
#include "RenderingSystem/ParticleRenderer.hpp"
#include "Exceptions/SubsystemExceptions.hpp"

#include <cmath>

ParticleEmitter *ParticleSystem::createEmitter(std::size_t maxParticles, const ParticleEmitterConfig &cfg) {
    m_emitters.push_back(std::make_unique<ParticleEmitter>(maxParticles, cfg));
    return m_emitters.back().get();
}

void ParticleSystem::update(float dt) {
    if (!std::isfinite(dt) || dt < 0.0f) {
        throw Engine::ParticleException(
            "ParticleSystem::update requires finite, non-negative delta time");
    }
    for (auto &e: m_emitters) {
        e->update(dt);
    }
}

void ParticleSystem::render(Rendering::ParticleRenderer &renderer) const {
    for(const auto& e: m_emitters){
        for (const Particle& particle : e->getParticles()) {
            if (particle.alive) {
                renderer.submit({particle.position, particle.size,
                                 particle.rotation, particle.color});
            }
        }
    }
}
