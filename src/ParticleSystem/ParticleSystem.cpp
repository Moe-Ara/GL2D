//
// Created by Mohamad on 25/11/2025.
//

#include "ParticleSystem.hpp"

ParticleSystem::ParticleSystem() {

}

ParticleSystem::~ParticleSystem() {

}

ParticleEmitter *ParticleSystem::createEmitter(std::size_t maxParticles, const ParticleEmitterConfig &cfg) {
    m_emitters.push_back(std::make_unique<ParticleEmitter>(maxParticles, cfg));
    return m_emitters.back().get();
}

void ParticleSystem::update(float dt) {
    for (auto &e: m_emitters) {
        e->update(dt);
    }
}

void ParticleSystem::render(Rendering::ParticleRenderer &renderer) const {
    for(const auto& e: m_emitters){
        e->render(renderer);
    }
}
