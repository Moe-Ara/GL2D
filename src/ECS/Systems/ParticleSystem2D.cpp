#include "ECS/Systems/ParticleSystem2D.hpp"

#include "ECS/Components/ParticleEmitter2D.hpp"
#include "ECS/Components/Transform2D.hpp"
#include "ECS/Registry.hpp"

#include <cmath>
#include <stdexcept>

namespace ECS {

void ParticleSystem2D::update(Registry& registry, float fixedDeltaTime) {
    if (!std::isfinite(fixedDeltaTime) || fixedDeltaTime <= 0.0f) {
        throw std::invalid_argument(
            "ParticleSystem2D requires a positive finite fixed delta time");
    }

    registry.each<Transform2D, ParticleEmitter2D>(
        [&registry, fixedDeltaTime](Entity entity, const Transform2D& transform,
                                    ParticleEmitter2D& particles) {
            if (particles.paused) {
                return;
            }

            const glm::mat4 world = toMatrix(transform);
            const glm::vec4 emitterPosition =
                world * glm::vec4(particles.localOffset, 0.0f, 1.0f);
            particles.emitter.setPosition({emitterPosition.x, emitterPosition.y});
            if (particles.targetFollowsTransform) {
                const glm::vec4 target =
                    world * glm::vec4(particles.targetOffset, 0.0f, 1.0f);
                particles.emitter.setTarget({target.x, target.y});
            }
            particles.emitter.setEmitting(particles.emitting);
            particles.emitter.update(fixedDeltaTime);
            // Burst after integration so deferred bursts spawn exactly at the
            // synchronized transform; they take their first step next update.
            particles.emitter.burst(particles.takePendingBurst());

            if (particles.autoDestroyWhenFinished && !particles.emitting &&
                particles.emitter.isFinished()) {
                registry.destroy(entity);
            }
        });
}

} // namespace ECS
