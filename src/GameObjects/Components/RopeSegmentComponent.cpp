#include "RopeSegmentComponent.hpp"

#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Entity.hpp"

#include <cmath>
#include <stdexcept>

void RopeSegmentComponent::setSegmentLength(float length) {
    if (!std::isfinite(length) || length <= 0.0f) {
        throw std::invalid_argument(
            "Rope segment length must be finite and positive");
    }
    m_segmentLength = length;
}

std::pair<glm::vec2, glm::vec2> RopeSegmentComponent::worldEndpoints(
        const Entity& owner) const {
    const auto* transform = owner.getComponent<TransformComponent>();
    if (!transform) {
        throw std::logic_error(
            "Rope segment requires a TransformComponent for world geometry");
    }
    if (m_segmentLength <= 0.0f) {
        throw std::logic_error("Rope segment length has not been configured");
    }

    const Transform& value = transform->getTransform();
    const float angle = glm::radians(value.Rotation);
    const glm::vec2 axis{std::cos(angle), std::sin(angle)};
    const glm::vec2 half = axis * (m_segmentLength * 0.5f);
    return {value.Position - half, value.Position + half};
}
