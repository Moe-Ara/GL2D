#include "CircleCollider.hpp"
#include "AABB.hpp"

#include <algorithm>


#include <glm/vec2.hpp>

float CircleCollider::getRadius() const { return m_radius; }
void CircleCollider::setRadius(float radius) { m_radius = radius; }
ColliderType CircleCollider::getType() const { return ColliderType::CIRCLE; }
AABB CircleCollider::getAABB() const {
  glm::vec2 center = m_localOffset;
  float radius = m_radius;

  const Transform *transform = tryGetTransform();
  if (transform) {
    // Apply transform: center offset + scale. Rotation ignored (circle is symmetric).
    center = transform->Position + m_localOffset * transform->Scale;
    const glm::vec2 scale = transform->Scale;
    // Uniformly scale radius using max axis to stay conservative when non-uniform.
    radius *= std::max(scale.x, scale.y);
  }

  return AABB({center.x - radius, center.y - radius},
              {center.x + radius, center.y + radius});
}
