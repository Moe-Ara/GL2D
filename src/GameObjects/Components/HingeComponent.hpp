#ifndef GL2D_HINGECOMPONENT_HPP
#define GL2D_HINGECOMPONENT_HPP

#include "GameObjects/IComponent.hpp"

#include <glm/vec2.hpp>

class Entity;

// Simple hinge constraint between this entity and a target entity.
class HingeComponent : public IUpdatableComponent {
public:
    explicit HingeComponent(Entity* target = nullptr)
        : m_target(target) {}
    ~HingeComponent() override = default;

    void setTarget(Entity* target) { m_target = target; }
    Entity* target() const { return m_target; }

    void setAnchorSelf(const glm::vec2& anchor) { m_anchorSelf = anchor; }
    const glm::vec2& anchorSelf() const { return m_anchorSelf; }

    void setAnchorTarget(const glm::vec2& anchor) { m_anchorTarget = anchor; }
    const glm::vec2& anchorTarget() const { return m_anchorTarget; }

    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    void setReferenceAngle(float angle) { m_referenceAngle = angle; }
    float referenceAngle() const { return m_referenceAngle; }

    void enableLimits(bool enabled) { m_limitsEnabled = enabled; }
    bool limitsEnabled() const { return m_limitsEnabled; }
    void setLimitRange(float lower, float upper) {
        m_lowerLimit = lower;
        m_upperLimit = upper;
    }
    void setLimitParameters(float stiffness, float damping, float maxTorque) {
        m_limitStiffness = stiffness;
        m_limitDamping = damping;
        m_maxLimitTorque = maxTorque;
    }
    float lowerLimit() const { return m_lowerLimit; }
    float upperLimit() const { return m_upperLimit; }
    float limitStiffness() const { return m_limitStiffness; }
    float limitDamping() const { return m_limitDamping; }
    float maxLimitTorque() const { return m_maxLimitTorque; }

    void enableMotor(bool enabled) { m_motorEnabled = enabled; }
    bool motorEnabled() const { return m_motorEnabled; }
    void setMotorSpeed(float speed) { m_motorSpeed = speed; }
    void setMotorParameters(float stiffness, float maxTorque) {
        m_motorStiffness = stiffness;
        m_maxMotorTorque = maxTorque;
    }
    float motorSpeed() const { return m_motorSpeed; }
    float motorStiffness() const { return m_motorStiffness; }
    float maxMotorTorque() const { return m_maxMotorTorque; }

    void update(Entity& /*owner*/, double /*dt*/) override {}

private:
    Entity* m_target{nullptr};
    glm::vec2 m_anchorSelf{0.0f, 0.0f};
    glm::vec2 m_anchorTarget{0.0f, 0.0f};
    bool m_enabled{true};
    float m_referenceAngle{0.0f};

    bool m_limitsEnabled{false};
    float m_lowerLimit{0.0f};
    float m_upperLimit{0.0f};
    float m_limitStiffness{10.0f};
    float m_limitDamping{1.0f};
    float m_maxLimitTorque{10.0f};

    bool m_motorEnabled{false};
    float m_motorSpeed{0.0f};
    float m_motorStiffness{5.0f};
    float m_maxMotorTorque{10.0f};
};

#endif // GL2D_HINGECOMPONENT_HPP
