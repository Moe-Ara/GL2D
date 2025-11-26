//
// Created by Mohamad on 23/11/2025.
//
#include "ACollider.hpp"
#include "CollisionDispatcher.hpp"

#include <algorithm>

std::unique_ptr<Hit> ACollider::hit(const ICollider &other) const {
    if (auto otherCollider = dynamic_cast<const ACollider *>(&other)) {
        return CollisionDispatcher::dispatch(*this, *otherCollider);
    }
    return nullptr;
}

void ACollider::setTransform(Transform *transform) {
  m_transform = transform;
}

Transform &ACollider::getTransform() const {
  static Transform identity{};
  return m_transform ? *m_transform : identity;
}

void ACollider::setTrigger(bool isTrigger, bool fireOnce) {
  m_isTrigger = isTrigger;
  m_fireOnce = fireOnce;
  if (!isTrigger) {
    m_triggered = false;
  }
}

void ACollider::setLayer(uint32_t layer) {
  // Clamp to 0-31 so it fits in a 32-bit mask.
  m_layer = std::min<uint32_t>(layer, 31u);
}

bool ACollider::allowsCollisionWith(const ACollider &other) const {
  const uint32_t otherBit = 1u << other.getLayer();
  return (m_collisionMask & otherBit) != 0u;
}
