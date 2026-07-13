#include "CircleCollider.hpp"
#include "AABB.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

CircleCollider::CircleCollider(float radius) { setRadius(radius); }

float CircleCollider::getRadius() const { return m_radius; }
void CircleCollider::setRadius(float radius) {
  if (!std::isfinite(radius) || radius < 0.0f) {
    throw std::invalid_argument("Circle radius must be finite and non-negative");
  }
  m_radius = radius;
}

void CircleCollider::setLocalOffset(const glm::vec2& offset) {
  if (!std::isfinite(offset.x) || !std::isfinite(offset.y)) {
    throw std::invalid_argument("Circle offset must be finite");
  }
  m_localOffset = offset;
}

ColliderType CircleCollider::getType() const { return ColliderType::CIRCLE; }

glm::vec2 CircleCollider::getWorldCenter() const {
  const Transform *transform = tryGetTransform();
  if (!transform) {
    return m_localOffset;
  }
  const glm::vec4 world = transform->getModelMatrix() *
                          glm::vec4(m_localOffset, 0.0f, 1.0f);
  return {world.x, world.y};
}

float CircleCollider::getWorldRadius() const {
  const Transform* transform = tryGetTransform();
  if (!transform) return m_radius;
  return m_radius * std::max(std::abs(transform->Scale.x),
                             std::abs(transform->Scale.y));
}

AABB CircleCollider::getAABB() const {
  const glm::vec2 center = getWorldCenter();
  const float radius = getWorldRadius();
  return AABB({center.x - radius, center.y - radius},
              {center.x + radius, center.y + radius});
}
