#include "AABBCollider.hpp"

#include <algorithm>
#include <glm/vec2.hpp>

ColliderType AABBCollider::getType() const { return ColliderType::AABB; }

AABB AABBCollider::getAABB() const {
  // If no transform is attached, treat m_min/m_max as world-space bounds.
  const Transform *transform = tryGetTransform();
  if (!transform) {
    return AABB(m_min, m_max);
  }

  const glm::vec2 position = transform->Position;
  const glm::vec2 scale = transform->Scale;

  // Scale the local min/max and translate; handle negative scale by re-sorting.
  const glm::vec2 p1 = position + m_min * scale;
  const glm::vec2 p2 = position + m_max * scale;
  const glm::vec2 worldMin{std::min(p1.x, p2.x), std::min(p1.y, p2.y)};
  const glm::vec2 worldMax{std::max(p1.x, p2.x), std::max(p1.y, p2.y)};

  return AABB(worldMin, worldMax);
}


void AABBCollider::setLocalBounds(const glm::vec2 &min, const glm::vec2 &max) {
  // Ensure min <= max on both axes even if caller passed inverted values.
  m_min = glm::vec2{std::min(min.x, max.x), std::min(min.y, max.y)};
  m_max = glm::vec2{std::max(min.x, max.x), std::max(min.y, max.y)};
}
