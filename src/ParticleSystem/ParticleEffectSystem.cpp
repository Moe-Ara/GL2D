//
// Transient particle effects (one-shot bursts that auto-destroy when done).
//

#include "ParticleEffectSystem.hpp"
#include <algorithm>
#include "Particle.hpp"
#include "RenderingSystem/ParticleRenderer.hpp"
#include "Exceptions/SubsystemExceptions.hpp"

#include <cmath>

ParticleEmitter* ParticleEffectSystem::spawnOneShot(const glm::vec2 &position,
                                                    const ParticleEffectDefinition &def,
                                                    unsigned int burstOverride) {
    auto emitter = std::make_unique<ParticleEmitter>(def.maxParticles, def.config);
    emitter->setPosition(position);
    emitter->setTarget(position);
    const unsigned int burstCount = burstOverride > 0 ? burstOverride : def.config.burstCount;
    if (burstCount > 0) {
        emitter->burst(burstCount);
    }
    emitter->setEmitting(false);
    ParticleEmitter* raw = emitter.get();
    m_active.push_back(ActiveEffect{std::move(emitter)});
    return raw;
}

void ParticleEffectSystem::update(float dt) {
    if (!std::isfinite(dt) || dt < 0.0f) {
        throw Engine::ParticleException(
            "ParticleEffectSystem::update requires finite, non-negative delta time");
    }
    auto it = m_active.begin();
    while (it != m_active.end()) {
        auto &em = it->emitter;
        em->update(dt);
        const auto &particles = em->getParticles();
        const bool anyAlive = std::any_of(particles.begin(), particles.end(),
                                          [](const Particle &p) { return p.alive; });
        if (!anyAlive) {
            it = m_active.erase(it);
        } else {
            ++it;
        }
    }
}

void ParticleEffectSystem::render(Rendering::ParticleRenderer &renderer) const {
    for (const auto& fx : m_active) {
        for (const Particle& particle : fx.emitter->getParticles()) {
            if (particle.alive) {
                renderer.submit({particle.position, particle.size,
                                 particle.rotation, particle.color});
            }
        }
    }
}

void ParticleEffectSystem::setEffectPosition(ParticleEmitter *emitter, const glm::vec2 &pos) {
    if (!emitter) return;
    emitter->setPosition(pos);
    emitter->setTarget(pos);
}
