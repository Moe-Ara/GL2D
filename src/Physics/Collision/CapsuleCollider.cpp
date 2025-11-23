#include "CapsuleCollider.hpp"

#include <algorithm>
#include <glm/vec2.hpp>

#include "AABB.hpp"
#include "CollisionDispatcher.hpp"

CapsuleCollider::CapsuleCollider(const glm::vec2 &localA, const glm::vec2 &localB,
                                 float radius, const glm::vec2 &localOffset)
    : m_localA(localA), m_localB(localB), m_radius(radius),
      m_localOffset(localOffset) {}

ColliderType CapsuleCollider::getType() const { return ColliderType::CAPSULE; }
float CapsuleCollider::getRadius() const { return m_radius; }
void CapsuleCollider::setRadius(float radius) { m_radius = radius; }

const glm::vec2 &CapsuleCollider::getLocalA() const { return m_localA; }
const glm::vec2 &CapsuleCollider::getLocalB() const { return m_localB; }
void CapsuleCollider::setLocalA(const glm::vec2 &pointA) { m_localA = pointA; }
void CapsuleCollider::setLocalB(const glm::vec2 &pointB) { m_localB = pointB; }

const glm::vec2 &CapsuleCollider::getLocalOffset() const { return m_localOffset; }
void CapsuleCollider::setLocalOffset(const glm::vec2 &localOffset) { m_localOffset = localOffset; }

glm::vec2 CapsuleCollider::getWorldA() const {
  glm::vec2 scale{1.0f, 1.0f};
  glm::vec2 pos{0.0f, 0.0f};
  if (auto transform = tryGetTransform()) {
    scale = transform->Scale;
    pos = transform->Position;
  }
  return pos + (m_localOffset + m_localA) * scale;
}

glm::vec2 CapsuleCollider::getWorldB() const {
  glm::vec2 scale{1.0f, 1.0f};
  glm::vec2 pos{0.0f, 0.0f};
  if (auto transform = tryGetTransform()) {
    scale = transform->Scale;
    pos = transform->Position;
  }
  return pos + (m_localOffset + m_localB) * scale;
}

AABB CapsuleCollider::getAABB() const {
  glm::vec2 A = getWorldA();
  glm::vec2 B = getWorldB();

  float radius = m_radius;
  if (auto transform = tryGetTransform()) {
    // Conservative scale on radius using max axis
    const glm::vec2 scale = transform->Scale;
    radius *= std::max(scale.x, scale.y);
  }

  glm::vec2 minPt{std::min(A.x, B.x) - radius, std::min(A.y, B.y) - radius};
  glm::vec2 maxPt{std::max(A.x, B.x) + radius, std::max(A.y, B.y) + radius};
  return AABB{minPt, maxPt};
}

std::unique_ptr<Hit> CapsuleCollider::hit(const ICollider &other) const {
  if (auto otherCollider = dynamic_cast<const ACollider*>(&other)) {
    return CollisionDispatcher::dispatch(*this, *otherCollider);
  }
  return nullptr;
}
