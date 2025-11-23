//
// Created by Mohamad on 23/11/2025.
//
#include "ACollider.hpp"
#include "CollisionDispatcher.hpp"

std::unique_ptr<Hit> ACollider::hit(const ICollider &other) const {
    if (auto otherCollider = dynamic_cast<const ACollider *>(&other)) {
        return CollisionDispatcher::dispatch(*this, *otherCollider);
    }
    return nullptr;
}

void ACollider::setTransform(Transform * transform) {
    m_transform= transform;
}

Transform & ACollider::getTransform() const {
    return *m_transform;
}

void ACollider::setTrigger(bool isTrigger, bool fireOnce) {
    m_isTrigger = isTrigger;
    m_fireOnce = fireOnce;
    if (!isTrigger) {
        m_triggered = false;
    }
}
