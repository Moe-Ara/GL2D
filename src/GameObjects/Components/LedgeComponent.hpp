#ifndef GL2D_LEDGE_COMPONENT_HPP
#define GL2D_LEDGE_COMPONENT_HPP

#include "GameObjects/IComponent.hpp"
#include "Physics/PhysicsUnits.hpp"

class LedgeComponent : public IComponent {
public:
    explicit LedgeComponent(float hangOffset = PhysicsUnits::toUnits(0.1f))
        : m_hangOffset(hangOffset) {}

    void setHangOffset(float offset) { m_hangOffset = offset; }
    float hangOffset() const { return m_hangOffset; }

private:
    float m_hangOffset{PhysicsUnits::toUnits(0.1f)};
};

#endif // GL2D_LEDGE_COMPONENT_HPP
