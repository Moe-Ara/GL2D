#include "AABBCollider.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace {
glm::vec2 normalizedOr(const glm::vec2& value, const glm::vec2& fallback) {
  constexpr float epsilonSquared = 1e-12f;
  const float lengthSquared = glm::dot(value, value);
  return lengthSquared > epsilonSquared
      ? value / std::sqrt(lengthSquared)
      : fallback;
}
} // namespace

AABBCollider::AABBCollider(glm::vec2 min, glm::vec2 max) {
  setLocalBounds(min, max);
}

ColliderType AABBCollider::getType() const { return ColliderType::AABB; }

AABB AABBCollider::getAABB() const {
  const OrientedBounds2D bounds = getOrientedBounds();
  const glm::vec2 worldHalfExtents =
      glm::abs(bounds.axisX) * bounds.halfExtents.x +
      glm::abs(bounds.axisY) * bounds.halfExtents.y;
  return AABB(bounds.center - worldHalfExtents,
              bounds.center + worldHalfExtents);
}

OrientedBounds2D AABBCollider::getOrientedBounds() const {
  const glm::vec2 localCenter = (m_min + m_max) * 0.5f;
  const glm::vec2 localHalfExtents = (m_max - m_min) * 0.5f;
  const Transform* transform = tryGetTransform();
  if (!transform) {
    return {localCenter, {1.0f, 0.0f}, {0.0f, 1.0f}, localHalfExtents};
  }

  const glm::mat4& model = transform->getModelMatrix();
  const glm::vec4 center4 = model * glm::vec4(localCenter, 0.0f, 1.0f);
  const glm::vec2 worldX = glm::vec2(model * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
  const glm::vec2 worldY = glm::vec2(model * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
  const float scaleX = glm::length(worldX);
  const float scaleY = glm::length(worldY);

  // Transform is TRS, so the two axes remain perpendicular. Fall back to the
  // other axis when one scale is zero to keep collision math finite.
  const glm::vec2 axisX = normalizedOr(
      worldX, normalizedOr(glm::vec2{worldY.y, -worldY.x}, {1.0f, 0.0f}));
  const glm::vec2 axisY = normalizedOr(
      worldY, glm::vec2{-axisX.y, axisX.x});
  return {{center4.x, center4.y}, axisX, axisY,
          localHalfExtents * glm::vec2{scaleX, scaleY}};
}


void AABBCollider::setLocalBounds(const glm::vec2 &min, const glm::vec2 &max) {
  if (!std::isfinite(min.x) || !std::isfinite(min.y) ||
      !std::isfinite(max.x) || !std::isfinite(max.y)) {
    throw std::invalid_argument("AABB collider bounds must be finite");
  }
  // Ensure min <= max on both axes even if caller passed inverted values.
  m_min = glm::vec2{std::min(min.x, max.x), std::min(min.y, max.y)};
  m_max = glm::vec2{std::max(min.x, max.x), std::max(min.y, max.y)};
}
