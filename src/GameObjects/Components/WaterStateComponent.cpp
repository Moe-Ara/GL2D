#include "WaterStateComponent.hpp"

#include <glm/glm.hpp>

void WaterStateComponent::setState(bool submerged,
                                   float submersion,
                                   const glm::vec2& flowVelocity,
                                   float surfaceY,
                                   double dt) {
    const float clampedSubmersion = std::clamp(submersion, 0.0f, 1.0f);
    m_justEntered = (!m_isSubmerged && submerged);
    m_justExited = (m_isSubmerged && !submerged);
    m_isSubmerged = submerged;

    if (m_isSubmerged) {
        m_submersion = clampedSubmersion;
        m_flowVelocity = flowVelocity;
        m_surfaceY = surfaceY;
        m_timeInWater += static_cast<float>(dt);
        m_timeSinceExit = 0.0f;
    } else {
        m_submersion = 0.0f;
        m_flowVelocity = glm::vec2{0.0f};
        m_surfaceY = surfaceY;
        m_timeSinceExit += static_cast<float>(dt);
        m_timeInWater = 0.0f;
    }
}
