//
// Created by Mohamad on 26/11/2025.
//

#ifndef GL2D_FEELINGSMANAGER_HPP
#define GL2D_FEELINGSMANAGER_HPP


#include "FeelingSnapshot.hpp"
#include <optional>
namespace FeelingsSystem{

class FeelingsManager {
public:
    FeelingsManager();

    virtual ~FeelingsManager();

    FeelingsManager(const FeelingsManager &other) = delete;

    FeelingsManager &operator=(const FeelingsManager &other) = delete;

    FeelingsManager(FeelingsManager &&other) = delete;

    FeelingsManager &operator=(FeelingsManager &&other) = delete;

    // Advance any active blend and expose the current snapshot.
    void update(float deltaMs);

    // Immediately set or blend into a new feeling.
    void setFeeling(const FeelingSnapshot& feeling, std::optional<float> blendMs = std::nullopt);

    // Read-only access for consumers (render/audio/camera/etc.).
    const FeelingSnapshot& getSnapshot() const { return m_snapshot; }

    bool isBlending() const { return m_isBlending; }
private:
    FeelingSnapshot m_snapshot{};
    FeelingSnapshot m_start{};
    FeelingSnapshot m_target{};
    float m_blendDurationMs{0.0f};
    float m_elapsedMs{0.0f};
    bool m_isBlending{false};

    template <typename T>
    static std::optional<T> lerpOptional(const std::optional<T>& from,
                                         const std::optional<T>& to,
                                         float t);
    
};


// Template definitions
template <typename T>
inline std::optional<T> FeelingsManager::lerpOptional(const std::optional<T>& from,
                                                      const std::optional<T>& to,
                                                      float t) {
    if (!to.has_value()) {
        return (t < 1.0f) ? from : std::optional<T>{};
    }
    if (!from.has_value()) {
        return to;
    }
    return from.value() + (to.value() - from.value()) * t;
}
}

#endif //GL2D_FEELINGSMANAGER_HPP
