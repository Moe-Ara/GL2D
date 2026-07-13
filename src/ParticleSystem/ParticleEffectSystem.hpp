//
// Transient particle effects (one-shot bursts that auto-destroy when done).
//

#ifndef GL2D_PARTICLEEFFECTSYSTEM_HPP
#define GL2D_PARTICLEEFFECTSYSTEM_HPP

#include <vector>
#include <memory>
#include <glm/vec2.hpp>
#include "ParticleEmitter.hpp"
#include "ParticleEffectLoader.hpp"

namespace Rendering { class ParticleRenderer; }

class ParticleEffectSystem {
public:
    ParticleEffectSystem() = default;

    // Spawns a one-shot effect: creates an emitter, bursts, and tracks it until all particles die.
    // Returns a non-owning pointer for repositioning. It becomes invalid as soon
    // as the final particle expires and the effect is removed during update().
    ParticleEmitter* spawnOneShot(const glm::vec2& position,
                                  const ParticleEffectDefinition& def,
                                  unsigned int burstOverride = 0);

    // Call once per frame to advance and cull finished one-shots.
    void update(float dt);

    // Render all active effects.
    void render(Rendering::ParticleRenderer& renderer) const;

    // For positioning moving effects (e.g., attach to a moving target).
    void setEffectPosition(ParticleEmitter* emitter, const glm::vec2& pos);

private:
    struct ActiveEffect {
        std::unique_ptr<ParticleEmitter> emitter;
    };

    std::vector<ActiveEffect> m_active;
};

#endif //GL2D_PARTICLEEFFECTSYSTEM_HPP
