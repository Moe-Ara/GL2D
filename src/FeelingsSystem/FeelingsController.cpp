//
// Central feelings controller: owns feelings state, applies to subsystems, and supports timed resets.
//

#include "FeelingsController.hpp"

#include "Graphics/Camera/Camera.hpp"
#include "RenderingSystem/Renderer.hpp"
#include "RenderingSystem/ParticleRenderer.hpp"
#include "AudioSystem/AudioManager.hpp"
#include "RenderingSystem/RenderSystem.hpp"
#include "FeelingsSystem/FeelingSnapshot.hpp"

namespace FeelingsSystem {

FeelingsController::FeelingsController(FeelingsManager& feelings)
    : m_feelings(feelings) {}

void FeelingsController::setDefinitions(std::unordered_map<std::string, FeelingSnapshot> defs, std::string defaultId) {
    m_defs = std::move(defs);
    if (!defaultId.empty()) {
        m_defaultId = defaultId;
    }
    // Apply default immediately if present.
    revertToDefault();
}

void FeelingsController::setTargets(Camera* camera, Rendering::Renderer* renderer, Rendering::ParticleRenderer* particles, Audio::AudioManager* audio) {
    m_camera = camera;
    m_renderer = renderer;
    m_particles = particles;
    m_audio = audio;
}

void FeelingsController::setFeeling(const std::string& id, std::optional<float> durationMs, std::optional<float> blendMs) {
    auto it = m_defs.find(id);
    if (it == m_defs.end()) {
        return;
    }
    m_feelings.setFeeling(it->second, blendMs);
    if (durationMs.has_value() && *durationMs > 0.0f) {
        m_activeTimerMs = *durationMs;
    } else {
        m_activeTimerMs.reset(); // persist until another feeling is set
    }
}

void FeelingsController::revertToDefault() {
    auto it = m_defs.find(m_defaultId);
    if (it != m_defs.end()) {
        m_feelings.setFeeling(it->second, 0.0f);
    } else if (!m_defs.empty()) {
        m_feelings.setFeeling(m_defs.begin()->second, 0.0f);
    }
    m_activeTimerMs = 0.0f;
}

void FeelingsController::update(float deltaMs) {
    m_feelings.update(deltaMs);

    if (m_activeTimerMs.has_value()) {
        float remaining = *m_activeTimerMs - deltaMs;
        if (remaining <= 0.0f) {
            revertToDefault();
            m_activeTimerMs.reset();
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
    if (m_renderer) {
        // Disable color grading tint; lighting is driven separately.
        m_renderer->applyFeeling(FeelingSnapshot{});
    }
    if (m_particles) {
        m_particles->applyFeeling(snap);
    }
    if (m_audio) {
        m_audio->applyFeeling(snap);
    }
    RenderSystem::applyFeeling(snap);
}

} // namespace FeelingsSystem
