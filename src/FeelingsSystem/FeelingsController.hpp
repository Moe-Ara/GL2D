//
// Central feelings controller: owns feelings state, applies to subsystems, and supports timed resets.
//

#pragma once

#include <string>
#include <unordered_map>
#include <optional>

#include "FeelingsManager.hpp"

class Camera;
class RenderSystem;
namespace Rendering { class Renderer; class ParticleRenderer; }
namespace Audio { class AudioManager; }

namespace FeelingsSystem {

class FeelingsController {
public:
    explicit FeelingsController(FeelingsManager& feelings);

    void setDefinitions(std::unordered_map<std::string, FeelingSnapshot> defs, std::string defaultId = "default");
    void setTargets(Camera* camera, Rendering::ParticleRenderer* particles,
                    Audio::AudioManager* audio);
    [[deprecated("Renderer reads feelings directly from its Scene")]]
    void setTargets(Camera* camera, Rendering::Renderer* renderer, Rendering::ParticleRenderer* particles, Audio::AudioManager* audio);

    // Set a feeling by id. Returns false when the id is not defined. If
    // durationMs has a value > 0, it auto-reverts after that duration.
    // If durationMs is std::nullopt, the feeling persists until another feeling is set.
    [[nodiscard]] bool setFeeling(
        const std::string& id,
        std::optional<float> durationMs = std::nullopt,
        std::optional<float> blendMs = std::nullopt);

    // Tick real-time duration timers and apply the manager's current snapshot
    // to external targets. The Scene advances its FeelingsManager once per
    // frame, independently of simulation time scale.
    void update(float deltaMs);

    const FeelingSnapshot& snapshot() const { return m_feelings.getSnapshot(); }
    float timeScale() const { return m_feelings.getSnapshot().timeScale.value_or(1.0f); }

private:
    void applyToTargets(const FeelingSnapshot& snap);
    void revertToDefault(float blendMs = 0.0f);

    FeelingsManager& m_feelings;
    std::unordered_map<std::string, FeelingSnapshot> m_defs;
    std::string m_defaultId{"default"};
    std::optional<float> m_activeTimerMs{};
    float m_activeBlendOutMs{0.0f};

    Camera* m_camera{nullptr};
    Rendering::ParticleRenderer* m_particles{nullptr};
    Audio::AudioManager* m_audio{nullptr};
};

} // namespace FeelingsSystem
