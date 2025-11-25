//
// Created by Mohamad on 25/11/2025.
//

#include "ParticleEmitter.hpp"
#include "RenderingSystem/ParticleRenderer.hpp"
#include <algorithm>

ParticleEmitter::ParticleEmitter(std::size_t maxParticles, const ParticleEmitterConfig &config)
: m_config(config), m_particles(maxParticles), m_rng(std::random_device{}()),m_unitDist(0.0f,1.0f)
{

}

ParticleEmitter::~ParticleEmitter() =default;

void ParticleEmitter::setPosition(const glm::vec2 &pos) {
m_position=pos;
    m_target=pos;
}

const glm::vec2 &ParticleEmitter::getPosition() const {
    return m_position;
}

void ParticleEmitter::setTarget(const glm::vec2 &target) {
    m_target = target;
}

void ParticleEmitter::setConfig(const ParticleEmitterConfig &cfg) {
    m_config=cfg;
}

const ParticleEmitterConfig &ParticleEmitter::getConfig() const {
    return m_config;
}

void ParticleEmitter::update(float dt) {
    if(m_config.spawnRate>0.0f){
        m_spawnAccumulator+=m_config.spawnRate* dt;
        unsigned int toSpawn=static_cast<unsigned int>(m_spawnAccumulator);
        m_spawnAccumulator-=toSpawn;
        for(unsigned int i=0; i<toSpawn; ++i){
            spawnOne();
        }
    }
    for(auto& p: m_particles){
        if(!p.alive) continue;
        p.age+=dt;
        if(p.age>=p.lifeTime){
            p.alive= false;
            continue;
        }
        p.velocity+=p.acceleration*dt;
        if(m_config.drag>0.0f){
            const float factor=std::max(0.0f,1.0f-m_config.drag*dt);
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
                p.velocity += dir * (m_config.spiralStrength * dt);
            }
        }
        p.position+=p.velocity*dt;
        p.rotation+=p.angularVelocity*dt;
        float t= p.age/p.lifeTime;
        p.color=glm::mix(m_config.startColor, m_config.endColor, t);
    }
}

void ParticleEmitter::render(Rendering::ParticleRenderer &renderer) const {
    for(const auto& p: m_particles){
        if(!p.alive) continue;
        renderer.submit({p.position, p.size, p.rotation, p.color});
    }
}

void ParticleEmitter::burst(unsigned int count) {
    for (unsigned int i = 0; i < count; ++i) {
        spawnOne();
    }
}

void ParticleEmitter::spawnOne() {
    auto it=std::find_if(m_particles.begin(),m_particles.end(),[](const Particle& p) {return !p.alive;});
    if(it==m_particles.end()) return;

    Particle& p=*it;
    p.alive=true;
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

    p.rotation=0.0f;
    p.angularVelocity=0.0f;
    p.color=m_config.startColor;
}
