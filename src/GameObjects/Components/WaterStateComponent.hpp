#ifndef GL2D_WATERSTATECOMPONENT_HPP
#define GL2D_WATERSTATECOMPONENT_HPP

#include "GameObjects/IComponent.hpp"

#include <algorithm>
#include <glm/vec2.hpp>

// Tracks whether an entity is currently submerged in a water volume and exposes
// fluid properties (flow, depth) for controllers or gameplay to react to.
class WaterStateComponent : public IComponent {
public:
    WaterStateComponent() = default;
    ~WaterStateComponent() override = default;

    void setState(bool submerged,
                  float submersion,
                  const glm::vec2& flowVelocity,
                  float surfaceY,
                  double dt);

    bool isSubmerged() const { return m_isSubmerged; }
    float submersion() const { return m_submersion; }
    const glm::vec2& flowVelocity() const { return m_flowVelocity; }
    float surfaceY() const { return m_surfaceY; }
    float timeInWater() const { return m_timeInWater; }
    float timeSinceExit() const { return m_timeSinceExit; }
    bool justEntered() const { return m_justEntered; }
    bool justExited() const { return m_justExited; }
    void clearTransientFlags() { m_justEntered = m_justExited = false; }

private:
    bool m_isSubmerged{false};
    bool m_justEntered{false};
    bool m_justExited{false};
    float m_submersion{0.0f};
    glm::vec2 m_flowVelocity{0.0f};
    float m_surfaceY{0.0f};
    float m_timeInWater{0.0f};
    float m_timeSinceExit{0.0f};
};

#endif // GL2D_WATERSTATECOMPONENT_HPP
