#ifndef GL2D_WATERVOLUMECOMPONENT_HPP
#define GL2D_WATERVOLUMECOMPONENT_HPP

#include "GameObjects/IComponent.hpp"

#include <glm/vec2.hpp>

class Entity;

// Defines a region of water with simple physical properties (density, drag,
// flow). Requires a ColliderComponent on the same entity; the collider is
// automatically marked as a trigger.
class WaterVolumeComponent : public IUpdatableComponent {
public:
    WaterVolumeComponent() = default;
    ~WaterVolumeComponent() override = default;

    void update(Entity& owner, double dt) override;

    void setDensity(float density) { m_density = density; }
    float density() const { return m_density; }

    void setLinearDrag(float drag) { m_linearDrag = drag; }
    float linearDrag() const { return m_linearDrag; }

    void setFlowFollowStrength(float strength) { m_flowFollowStrength = strength; }
    float flowFollowStrength() const { return m_flowFollowStrength; }

    void setFlowVelocity(const glm::vec2& flow) { m_flowVelocity = flow; }
    glm::vec2 flowVelocity() const { return m_flowVelocity; }

    void setMinSubmersionForDrag(float value) { m_minSubmersionForDrag = value; }
    float minSubmersionForDrag() const { return m_minSubmersionForDrag; }

private:
    float m_density{1.05f};
    float m_linearDrag{6.0f};
    float m_flowFollowStrength{1.5f};
    glm::vec2 m_flowVelocity{0.0f};
    float m_minSubmersionForDrag{0.05f};
};

#endif // GL2D_WATERVOLUMECOMPONENT_HPP
