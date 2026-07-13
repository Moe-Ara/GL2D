//
// Central feelings controller: owns feelings state, applies to subsystems, and supports timed resets.
//

#include "FeelingsController.hpp"

#include "Graphics/Camera/Camera.hpp"
#include "RenderingSystem/ParticleRenderer.hpp"
#include "AudioSystem/AudioManager.hpp"
#include "FeelingsSystem/FeelingSnapshot.hpp"

#include <cmath>
#include <stdexcept>

namespace FeelingsSystem {

FeelingsController::FeelingsController(FeelingsManager& feelings)
    : m_feelings(feelings) {}

void FeelingsController::setDefinitions(std::unordered_map<std::string, FeelingSnapshot> defs, std::string defaultId) {
    const std::string resolvedDefault = defaultId.empty() ? m_defaultId : defaultId;
    for (const auto& [key, snapshot] : defs) {
        if (key.empty() || snapshot.id != key) {
            throw std::invalid_argument(
                "Feeling definition keys must be non-empty and match snapshot ids");
        }
        if (const auto error = validationError(snapshot)) {
            throw std::invalid_argument("Invalid feeling '" + key + "': " + *error);
        }
    }
    if (!defs.empty() && !defs.contains(resolvedDefault)) {
        throw std::invalid_argument("Default feeling '" + resolvedDefault +
                                    "' is not defined");
    }
    m_defs = std::move(defs);
    m_defaultId = resolvedDefault;
    revertToDefault();
}

void FeelingsController::setTargets(Camera* camera,
                                    Rendering::ParticleRenderer* particles,
                                    Audio::AudioManager* audio) {
    m_camera = camera;
    m_particles = particles;
    m_audio = audio;
    applyToTargets(m_feelings.getSnapshot());
}

void FeelingsController::setTargets(Camera* camera, Rendering::Renderer*,
                                    Rendering::ParticleRenderer* particles,
                                    Audio::AudioManager* audio) {
    setTargets(camera, particles, audio);
}

bool FeelingsController::setFeeling(const std::string& id, std::optional<float> durationMs, std::optional<float> blendMs) {
    if ((durationMs && (!std::isfinite(*durationMs) || *durationMs < 0.0f)) ||
        (blendMs && (!std::isfinite(*blendMs) || *blendMs < 0.0f))) {
        throw std::invalid_argument(
            "Feeling duration and blend time must be finite and non-negative");
    }
    auto it = m_defs.find(id);
    if (it == m_defs.end()) {
        return false;
    }
    m_feelings.setFeeling(it->second, blendMs);
    if (durationMs.has_value() && *durationMs > 0.0f) {
        m_activeTimerMs = *durationMs;
    } else {
        m_activeTimerMs.reset(); // persist until another feeling is set
    }
    m_activeBlendOutMs = it->second.blendOutMs;
    return true;
}

void FeelingsController::revertToDefault(float blendMs) {
    auto it = m_defs.find(m_defaultId);
    if (it != m_defs.end()) {
        m_feelings.setFeeling(it->second, blendMs);
    } else {
        m_feelings.setFeeling(FeelingSnapshot{}, 0.0f);
    }
    m_activeTimerMs.reset();
    m_activeBlendOutMs = 0.0f;
}

void FeelingsController::update(float deltaMs) {
    if (!std::isfinite(deltaMs) || deltaMs < 0.0f) {
        throw std::invalid_argument(
            "FeelingsController delta must be finite and non-negative");
    }
    if (m_activeTimerMs.has_value()) {
        float remaining = *m_activeTimerMs - deltaMs;
        if (remaining <= 0.0f) {
            revertToDefault(m_activeBlendOutMs);
        } else {
            m_activeTimerMs = remaining;
        }
    }

    applyToTargets(m_feelings.getSnapshot());
}

void FeelingsController::applyToTargets(const FeelingSnapshot& snap) {
    if (m_camera) {
        m_camera->applyFeeling(snap);
    }
    if (m_particles) {
        m_particles->applyFeeling(snap);
    }
    if (m_audio) {
        m_audio->applyFeeling(snap);
    }
}

} // namespace FeelingsSystem
