//
// Created by Mohamad on 20/11/2025.
//

#ifndef GL2D_SCREENSHAKEEFFECT_HPP
#define GL2D_SCREENSHAKEEFFECT_HPP
#include <cmath>
#include <algorithm>

#include <glm/gtc/constants.hpp>

#include "Graphics/Camera/ICameraEffect.hpp"

class ScreenShakeEffect final : public ICameraEffect {
public:
    ScreenShakeEffect(float magnitude, float duration, float roughness, glm::vec2 seed)
            : m_magnitude(std::max(0.0f, magnitude)),
              m_duration(duration > 0.0f ? duration : 0.15f),
              m_roughness(std::max(0.1f, roughness)),
              m_seed(seed) {}

    bool update(double deltaTime, CameraEffectState &state) override {
        m_elapsed += static_cast<float>(deltaTime);
        if (m_elapsed >= m_duration) {
            return false;
        }
        const float progress = m_elapsed / m_duration;
        const float falloff = (1.0f - progress) * (1.0f - progress);
        const float seedPhase = m_seed.x * 13.37f + m_seed.y * 7.11f;
        const float phase = seedPhase + m_elapsed * m_roughness * glm::two_pi<float>();
        const glm::vec2 noise{
            std::sin(phase) * 0.7f + std::sin(phase * 2.17f + 1.3f) * 0.3f,
            std::cos(phase * 1.31f + 0.7f) * 0.7f +
                std::cos(phase * 2.53f) * 0.3f};
        state.posOffset += noise * (m_magnitude * falloff);
        state.rotOffsetDeg += std::sin(phase * 0.73f) *
                              (m_magnitude * 0.08f * falloff);
        return true;
    }

private:
    float m_magnitude;
    float m_duration;
    float m_roughness;
    float m_elapsed{0.0f};
    glm::vec2 m_seed;
};

#endif //GL2D_SCREENSHAKEEFFECT_HPP
