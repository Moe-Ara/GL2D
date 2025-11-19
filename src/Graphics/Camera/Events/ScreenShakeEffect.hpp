//
// Created by Mohamad on 20/11/2025.
//

#ifndef GL2D_SCREENSHAKEEFFECT_HPP
#define GL2D_SCREENSHAKEEFFECT_HPP
#include <cmath>
#include <algorithm>

#include "Graphics/Camera/ICameraEffect.hpp"

class ScreenShakeEffect final : public ICameraEffect {
public:
    ScreenShakeEffect(float magnitude, float duration, glm::vec2 seed)
            : m_magnitude(std::max(0.0f, magnitude)),
              m_duration(duration > 0.0f ? duration : 0.15f),
              m_seed(seed) {}

    bool update(double deltaTime, CameraEffectState &state) override {
        m_elapsed += static_cast<float>(deltaTime);
        if (m_elapsed >= m_duration) {
            return false;
        }
        const float progress = m_elapsed / m_duration;
        const float falloff = 1.0f - progress;
        const float angle = m_seed.x * 13.37f + m_seed.y * 7.11f + m_elapsed * 96.0f;
        glm::vec2 offset(std::sin(angle), std::cos(angle));
        state.posOffset += offset * (m_magnitude * falloff);
        state.rotOffsetDeg += std::sin(angle * 0.5f) * (m_magnitude * 0.25f * falloff);
        return true;
    }

private:
    float m_magnitude;
    float m_duration;
    float m_elapsed{0.0f};
    glm::vec2 m_seed;
};

#endif //GL2D_SCREENSHAKEEFFECT_HPP
