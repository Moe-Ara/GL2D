//
// Loads particle effect presets from JSON.
//

#ifndef GL2D_PARTICLEEFFECTLOADER_HPP
#define GL2D_PARTICLEEFFECTLOADER_HPP

#include <string>
#include <vector>
#include "ParticleEmitterConfig.hpp"

struct ParticleEffectDefinition {
    std::string name;
    std::size_t maxParticles{128};
    ParticleEmitterConfig config{};
};

class ParticleEffectLoader {
public:
    static std::vector<ParticleEffectDefinition> loadFromFile(const std::string& path);
};

#endif //GL2D_PARTICLEEFFECTLOADER_HPP
