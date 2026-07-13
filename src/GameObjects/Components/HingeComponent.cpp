#include "HingeComponent.hpp"

#include <cmath>
#include <stdexcept>

namespace {
void requireFinite(const glm::vec2& value, const char* message) {
    if (!std::isfinite(value.x) || !std::isfinite(value.y)) {
        throw std::invalid_argument(message);
    }
}

void requireNonNegative(float value, const char* message) {
    if (!std::isfinite(value) || value < 0.0f) {
        throw std::invalid_argument(message);
    }
}
}

void HingeComponent::setAnchorSelf(const glm::vec2& anchor) {
    requireFinite(anchor, "Hinge self anchor must be finite");
    m_anchorSelf = anchor;
}

void HingeComponent::setAnchorTarget(const glm::vec2& anchor) {
    requireFinite(anchor, "Hinge target anchor must be finite");
    m_anchorTarget = anchor;
}

void HingeComponent::setReferenceAngle(float angle) {
    if (!std::isfinite(angle)) {
        throw std::invalid_argument("Hinge reference angle must be finite");
    }
    m_referenceAngle = angle;
}

void HingeComponent::setLimitRange(float lower, float upper) {
    if (!std::isfinite(lower) || !std::isfinite(upper) || lower > upper) {
        throw std::invalid_argument(
            "Hinge limit range must be finite and ordered lower <= upper");
    }
    m_lowerLimit = lower;
    m_upperLimit = upper;
}

void HingeComponent::setLimitParameters(float stiffness, float damping,
                                        float maxTorque) {
    requireNonNegative(stiffness,
                       "Hinge limit stiffness must be finite and non-negative");
    requireNonNegative(damping,
                       "Hinge limit damping must be finite and non-negative");
    requireNonNegative(maxTorque,
                       "Hinge maximum limit torque must be finite and non-negative");
    m_limitStiffness = stiffness;
    m_limitDamping = damping;
    m_maxLimitTorque = maxTorque;
}

void HingeComponent::setMotorSpeed(float speed) {
    if (!std::isfinite(speed)) {
        throw std::invalid_argument("Hinge motor speed must be finite");
    }
    m_motorSpeed = speed;
}

void HingeComponent::setMotorParameters(float stiffness, float maxTorque) {
    requireNonNegative(stiffness,
                       "Hinge motor stiffness must be finite and non-negative");
    requireNonNegative(maxTorque,
                       "Hinge maximum motor torque must be finite and non-negative");
    m_motorStiffness = stiffness;
    m_maxMotorTorque = maxTorque;
}
