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
    void setTargets(Camera* camera, Rendering::Renderer* renderer, Rendering::ParticleRenderer* particles, Audio::AudioManager* audio);

    // Set a feeling by id. If durationMs has a value > 0, it auto-reverts after that duration.
    // If durationMs is std::nullopt, the feeling persists until another feeling is set.
    void setFeeling(const std::string& id, std::optional<float> durationMs = std::nullopt, std::optional<float> blendMs = std::nullopt);

    // Tick blends, timers, and apply to targets. deltaMs in milliseconds.
    void update(float deltaMs);

    const FeelingSnapshot& snapshot() const { return m_feelings.getSnapshot(); }
    float timeScale() const { return m_feelings.getSnapshot().timeScale.value_or(1.0f); }

private:
    void applyToTargets(const FeelingSnapshot& snap);
    void revertToDefault();

    FeelingsManager& m_feelings;
    std::unordered_map<std::string, FeelingSnapshot> m_defs;
    std::string m_defaultId{"default"};
    std::optional<float> m_activeTimerMs{};

    Camera* m_camera{nullptr};
    Rendering::Renderer* m_renderer{nullptr};
    Rendering::ParticleRenderer* m_particles{nullptr};
    Audio::AudioManager* m_audio{nullptr};
};

} // namespace FeelingsSystem
