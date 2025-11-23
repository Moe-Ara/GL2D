//
// TriggerSystem.cpp
//

#include "TriggerSystem.hpp"

#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Entity.hpp"
#include "Physics/Collision/ACollider.hpp"
#include "Physics/Collision/CollisionDispatcher.hpp"

namespace {
bool isTrigger(const ACollider* c) {
    return c && c->isTrigger();
}

bool hasTriggerOnceFired(const ACollider* c) {
    return c && c->isTrigger() && c->triggersOnce() && c->hasTriggered();
}
} // namespace

TriggerSystem::PairKey TriggerSystem::makeKey(uint64_t a, uint64_t b) {
    if (a < b) return PairKey{a, b};
    return PairKey{b, a};
}

bool TriggerSystem::overlapsAABB(const ACollider& a, const ACollider& b) {
    return a.getAABB().overlaps(b.getAABB());
}

void TriggerSystem::clear() {
    m_activeOverlaps.clear();
}

void TriggerSystem::unregisterEntity(uint64_t entityId) {
    for (auto it = m_activeOverlaps.begin(); it != m_activeOverlaps.end();) {
        if (it->a == entityId || it->b == entityId) {
            it = m_activeOverlaps.erase(it);
        } else {
            ++it;
        }
    }
}

void TriggerSystem::handleEnter(const ColliderEntry& a, const ColliderEntry& b, bool aWasTriggered, bool bWasTriggered) {
    if (a.component && (!a.collider || !a.collider->triggersOnce() || !aWasTriggered)) {
        a.component->invokeTriggerEnter(*a.entity, *b.entity);
    }
    if (b.component && (!b.collider || !b.collider->triggersOnce() || !bWasTriggered)) {
        b.component->invokeTriggerEnter(*b.entity, *a.entity);
    }
}

void TriggerSystem::handleExit(const ColliderEntry& a, const ColliderEntry& b) {
    if (a.component) {
        a.component->invokeTriggerExit(*a.entity, *b.entity);
    }
    if (b.component) {
        b.component->invokeTriggerExit(*b.entity, *a.entity);
    }
}

void TriggerSystem::update(const std::vector<std::unique_ptr<Entity>>& entities) {
    std::vector<ColliderEntry> entries;
    entries.reserve(entities.size());

    for (const auto& ePtr : entities) {
        if (!ePtr) continue;
        auto* colliderComp = ePtr->getComponent<ColliderComponent>();
        if (!colliderComp) continue;
        colliderComp->ensureCollider(*ePtr);
        auto* collider = colliderComp->collider();
        if (!collider) continue;
        entries.push_back(ColliderEntry{ePtr.get(), colliderComp, collider});
    }

    for (size_t i = 0; i < entries.size(); ++i) {
        for (size_t j = i + 1; j < entries.size(); ++j) {
            const auto& a = entries[i];
            const auto& b = entries[j];
            const auto key = makeKey(a.entity->getId(), b.entity->getId());
            const bool wasActive = m_activeOverlaps.contains(key);

            const bool anyTrigger = isTrigger(a.collider) || isTrigger(b.collider);
            if (!anyTrigger) {
                if (wasActive) {
                    m_activeOverlaps.erase(key);
                }
                continue;
            }

            const bool aTriggeredBefore = hasTriggerOnceFired(a.collider);
            const bool bTriggeredBefore = hasTriggerOnceFired(b.collider);
            auto hit = CollisionDispatcher::dispatch(*a.collider, *b.collider);
            bool overlapping = static_cast<bool>(hit);

            if (!overlapping && wasActive) {
                overlapping = overlapsAABB(*a.collider, *b.collider);
            }

            if (overlapping) {
                const bool inserted = m_activeOverlaps.insert(key).second;
                if (inserted) {
                    handleEnter(a, b, aTriggeredBefore, bTriggeredBefore);
                }
            } else if (wasActive) {
                m_activeOverlaps.erase(key);
                handleExit(a, b);
            }
        }
    }
}
