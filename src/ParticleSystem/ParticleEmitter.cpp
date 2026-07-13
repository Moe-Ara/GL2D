//
// Created by Mohamad on 25/11/2025.
//

#include "ParticleEmitter.hpp"
#include "Exceptions/SubsystemExceptions.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
std::size_t validatedCapacity(std::size_t capacity,
                              const ParticleEmitterConfig& config) {
    validateParticleEmitterConfig(capacity, config);
    return capacity;
}
}

ParticleEmitter::ParticleEmitter(std::size_t maxParticles, const ParticleEmitterConfig &config)
: m_config(config), m_particles(validatedCapacity(maxParticles, config)),
  m_rng(config.randomSeed),
  m_unitDist(0.0f,1.0f)
{
}

void ParticleEmitter::setPosition(const glm::vec2 &pos) {
    if (!std::isfinite(pos.x) || !std::isfinite(pos.y)) {
        throw Engine::ParticleException("Emitter position must be finite");
    }
    m_position=pos;
}

const glm::vec2 &ParticleEmitter::getPosition() const {
    return m_position;
}

void ParticleEmitter::setTarget(const glm::vec2 &target) {
    if (!std::isfinite(target.x) || !std::isfinite(target.y)) {
        throw Engine::ParticleException("Emitter target must be finite");
    }
    m_target = target;
}

void ParticleEmitter::setConfig(const ParticleEmitterConfig &cfg) {
    validateParticleEmitterConfig(m_particles.size(), cfg);
    m_config=cfg;
    m_rng.seed(cfg.randomSeed);
}

const ParticleEmitterConfig &ParticleEmitter::getConfig() const {
    return m_config;
}

void ParticleEmitter::update(float dt) {
    if (!std::isfinite(dt) || dt < 0.0f) {
        throw Engine::ParticleException(
            "ParticleEmitter::update requires finite, non-negative delta time");
    }
    if (dt == 0.0f) {
        return;
    }
    if(m_emitting && m_config.spawnRate>0.0f){
        const std::size_t available = m_particles.size() - m_liveParticleCount;
        const double produced = std::min(
            m_spawnAccumulator + static_cast<double>(m_config.spawnRate) * dt,
            static_cast<double>(available) + 1.0);
        const double wholeParticles = std::floor(produced);
        m_spawnAccumulator = produced - wholeParticles;
        const std::size_t toSpawn = std::min(
            static_cast<std::size_t>(wholeParticles), available);
        for(std::size_t i=0; i<toSpawn; ++i){
            if (!spawnOne()) break;
        }
    }
    for(auto& p: m_particles){
        if(!p.alive) continue;
        p.age+=dt;
        if(p.age>=p.lifeTime){
            p.alive= false;
            --m_liveParticleCount;
            continue;
        }
        p.velocity+=p.acceleration*dt;
        if(m_config.drag>0.0f){
            const float factor=std::exp(-m_config.drag*dt);
            p.velocity*=factor;
        }
        // Steering behaviors
        const glm::vec2 toTarget = m_target - p.position;
        const float dist2 = glm::dot(toTarget, toTarget);
        if(dist2 > 0.0001f){
            const float invLen = 1.0f / std::sqrt(dist2);
            const glm::vec2 dir = toTarget * invLen;
            if(m_config.homingStrength>0.0f){
                p.velocity += dir * (m_config.homingStrength * dt);
            }
            if(m_config.orbitStrength>0.0f){
                const glm::vec2 tangential{-dir.y, dir.x};
                p.velocity += tangential * (m_config.orbitStrength * dt);
            }
            if(m_config.spiralStrength!=0.0f){
                p.velocity -= dir * (m_config.spiralStrength * dt);
            }
        }
        p.position+=p.velocity*dt;
        p.rotation+=p.angularVelocity*dt;
        float t= p.age/p.lifeTime;
        p.color=glm::mix(m_config.startColor, m_config.endColor, t);
        p.size = p.initialSize * glm::mix(1.0f, m_config.endSizeMultiplier, t);
    }
}

void ParticleEmitter::burst(unsigned int count) {
    const std::size_t available = m_particles.size() - m_liveParticleCount;
    const std::size_t toSpawn = std::min<std::size_t>(count, available);
    for (std::size_t i = 0; i < toSpawn; ++i) {
        if (!spawnOne()) break;
    }
}

bool ParticleEmitter::spawnOne() {
    if (m_liveParticleCount >= m_particles.size()) {
        return false;
    }
    std::size_t index = m_nextFreeIndex;
    while (m_particles[index].alive) {
        index = (index + 1) % m_particles.size();
    }
    m_nextFreeIndex = (index + 1) % m_particles.size();
    Particle& p=m_particles[index];
    p.alive=true;
    ++m_liveParticleCount;
    p.age=0.0f;

    float lifeRange=m_config.maxLifeTime-m_config.minLifeTime;
    p.lifeTime=m_config.minLifeTime+m_unitDist(m_rng)*lifeRange;
    float baseDir=m_config.direction;
    float halfSpread=m_config.spread*0.5f;
    float angle=baseDir+(m_unitDist(m_rng)* 2.0f-1.0f)*halfSpread;
    float speedRange=m_config.maxSpeed-m_config.minSpeed;
    float speed=m_config.minSpeed+m_unitDist(m_rng)*speedRange;

    glm::vec2 dir=glm::normalize(glm::vec2(glm::cos(angle),glm::sin(angle)));
    p.position=m_position;
    p.velocity=dir*speed;
    p.acceleration=m_config.gravity;

    float sizeRange=m_config.maxSize-m_config.minSize;
    float size=m_config.minSize+m_unitDist(m_rng)*sizeRange;
    p.size=glm::vec2(size);
    p.initialSize = p.size;

    p.rotation=0.0f;
    const float angularRange = m_config.maxAngularVelocity -
                               m_config.minAngularVelocity;
    p.angularVelocity = m_config.minAngularVelocity +
                        m_unitDist(m_rng) * angularRange;
    p.color=m_config.startColor;
    return true;
}
