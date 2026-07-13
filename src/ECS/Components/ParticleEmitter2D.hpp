#pragma once

#include "ParticleSystem/ParticleEmitter.hpp"

#include <cstddef>
#include <limits>

namespace ECS {

// Fixed-step particle simulation owned by an ECS entity. Transform2D supplies
// the emitter anchor; already-spawned particles remain in world space.
struct ParticleEmitter2D {
    ParticleEmitter emitter;
    glm::vec2 localOffset{0.0f};
    glm::vec2 targetOffset{0.0f};
    bool emitting{true};
    bool paused{false};
    bool targetFollowsTransform{true};
    bool autoDestroyWhenFinished{false};

    // Deferred until the fixed-step system has synchronized Transform2D.
    void requestBurst(unsigned int count) noexcept {
        const unsigned int available =
            std::numeric_limits<unsigned int>::max() - m_pendingBurst;
        m_pendingBurst += count > available ? available : count;
    }

    [[nodiscard]] unsigned int takePendingBurst() noexcept {
        const unsigned int count = m_pendingBurst;
        m_pendingBurst = 0;
        return count;
    }

    explicit ParticleEmitter2D(
        std::size_t capacity, const ParticleEmitterConfig& config)
        : emitter(capacity, config) {}

private:
    unsigned int m_pendingBurst{0};
};

} // namespace ECS
