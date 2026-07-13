#include "CapsuleCollider.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "AABB.hpp"
#include "CollisionDispatcher.hpp"

CapsuleCollider::CapsuleCollider(const glm::vec2 &localA, const glm::vec2 &localB,
                                 float radius, const glm::vec2 &localOffset)
    : m_localA(localA), m_localB(localB), m_localOffset(localOffset) {
  setLocalA(localA);
  setLocalB(localB);
  setLocalOffset(localOffset);
  setRadius(radius);
}

ColliderType CapsuleCollider::getType() const { return ColliderType::CAPSULE; }
float CapsuleCollider::getRadius() const { return m_radius; }
void CapsuleCollider::setRadius(float radius) {
  if (!std::isfinite(radius) || radius < 0.0f) {
    throw std::invalid_argument("Capsule radius must be finite and non-negative");
  }
  m_radius = radius;
}

const glm::vec2 &CapsuleCollider::getLocalA() const { return m_localA; }
const glm::vec2 &CapsuleCollider::getLocalB() const { return m_localB; }
void CapsuleCollider::setLocalA(const glm::vec2 &pointA) {
  if (!std::isfinite(pointA.x) || !std::isfinite(pointA.y)) {
    throw std::invalid_argument("Capsule endpoint A must be finite");
  }
  m_localA = pointA;
}
void CapsuleCollider::setLocalB(const glm::vec2 &pointB) {
  if (!std::isfinite(pointB.x) || !std::isfinite(pointB.y)) {
    throw std::invalid_argument("Capsule endpoint B must be finite");
  }
  m_localB = pointB;
}

const glm::vec2 &CapsuleCollider::getLocalOffset() const { return m_localOffset; }
void CapsuleCollider::setLocalOffset(const glm::vec2 &localOffset) {
  if (!std::isfinite(localOffset.x) || !std::isfinite(localOffset.y)) {
    throw std::invalid_argument("Capsule offset must be finite");
  }
  m_localOffset = localOffset;
}

glm::vec2 CapsuleCollider::getWorldA() const {
  if (const auto* transform = tryGetTransform()) {
    const glm::vec4 world = transform->getModelMatrix() *
        glm::vec4(m_localOffset + m_localA, 0.0f, 1.0f);
    return {world.x, world.y};
  }
  return m_localOffset + m_localA;
}

glm::vec2 CapsuleCollider::getWorldB() const {
  if (const auto* transform = tryGetTransform()) {
    const glm::vec4 world = transform->getModelMatrix() *
        glm::vec4(m_localOffset + m_localB, 0.0f, 1.0f);
    return {world.x, world.y};
  }
  return m_localOffset + m_localB;
}

float CapsuleCollider::getWorldRadius() const {
  const Transform* transform = tryGetTransform();
  if (!transform) return m_radius;
  return m_radius * std::max(std::abs(transform->Scale.x),
                             std::abs(transform->Scale.y));
}

AABB CapsuleCollider::getAABB() const {
  glm::vec2 A = getWorldA();
  glm::vec2 B = getWorldB();

  const float radius = getWorldRadius();

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
